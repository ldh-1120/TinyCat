#include "Application.h"
#include "CatConfig.h"
#include <windowsx.h>
#include <algorithm>

using namespace CatConfig;

namespace {
	constexpr wchar_t WindowClassName[] = L"DesktopCatWindow";

	std::wstring ExecutableDirectory() {
		std::wstring path(MAX_PATH, L'\0');
		for (;;) {
			const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
			if (length == 0)
				return { };

			if (length < path.size()) {
				path.resize(length);
				const auto separator = path.find_last_of(L"\\/");
				return separator == std::wstring::npos ? std::wstring { } : path.substr(0, separator);
			}

			if (path.size() >= 32768)
				return { };

			path.resize(path.size() * 2);
		}
	}
}

Application::Application(HINSTANCE instance) : instance(instance), simulation(cat, platforms) {}

Application::~Application() {
	if (window)
		DestroyWindow(window);

	if (windowClass)
		UnregisterClassW(WindowClassName, instance);

	if (grabCursor)
		DestroyCursor(grabCursor);

	if (grabbingCursor)
		DestroyCursor(grabbingCursor);
}

bool Application::Initialize(std::wstring& error) {
	const auto directory = ExecutableDirectory();
	if (directory.empty()) {
		error = L"실행 파일 경로를 확인하지 못했습니다.";
		return false;
	}

	if (!sprites.Load(directory, error))
		return false;

	grabCursor = LoadCursorFromFileW((directory + L"\\grab.cur").c_str());
	grabbingCursor = LoadCursorFromFileW((directory + L"\\grabbing.cur").c_str());
	if (!grabCursor || !grabbingCursor) {
		error = L"grab.cur 또는 grabbing.cur 파일을 불러오지 못했습니다.";
		return false;
	}

	if (!renderer.Initialize(RenderWidth, RenderHeight)) {
		error = L"렌더러를 초기화하지 못했습니다.";
		return false;
	}

	WNDCLASSW definition { };
	definition.lpfnWndProc = WindowProc;
	definition.hInstance = instance;
	definition.lpszClassName = WindowClassName;

	windowClass = RegisterClassW(&definition);
	if (!windowClass) {
		error = L"창 클래스를 등록하지 못했습니다.";
		return false;
	}

	window = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
		WindowClassName, L"Desktop Cat", WS_POPUP,
		static_cast<int>(cat.x) - static_cast<int>(RenderPadding),
		static_cast<int>(cat.y) - static_cast<int>(RenderPadding),
		RenderWidth, RenderHeight, nullptr, nullptr, instance, this);

	if (!window) {
		error = L"고양이 창을 만들지 못했습니다.";
		return false;
	}

	platforms.SetPetWindow(window);
	if (!Render()) {
		error = L"첫 프레임을 표시하지 못했습니다.";
		return false;
	}

	ShowWindow(window, SW_SHOWNOACTIVATE);
	return true;
}

bool Application::Render() {
	const Image& frame = sprites.CurrentFrame(cat);
	const Image rendered = RotateImageNearest(frame, cat.rotation, ScruffX, ScruffY);
	return renderer.Render(window, rendered, static_cast<int>(cat.x + cat.impactOffsetX) - static_cast<int>(RenderPadding), static_cast<int>(cat.y + cat.impactOffsetY) - static_cast<int>(RenderPadding));
}

int Application::Run() {
	LARGE_INTEGER frequency { };
	LARGE_INTEGER previousTime { };
	if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
		!QueryPerformanceCounter(&previousTime))
		return 1;

	for (;;) {
		MSG message { };
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
			if (message.message == WM_QUIT)
				return static_cast<int>(message.wParam);
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		LARGE_INTEGER currentTime { };
		if (!QueryPerformanceCounter(&currentTime))
			return 1;

		const double elapsed = static_cast<double>(currentTime.QuadPart - previousTime.QuadPart) / static_cast<double>(frequency.QuadPart);
		previousTime = currentTime;
		const float deltaTime = std::clamp(static_cast<float>(elapsed), 0.0f, 0.05f);

		POINT cursor { };
		const bool hasCursor = GetCursorPos(&cursor) != FALSE;
		simulation.Update(deltaTime, hasCursor ? &cursor : nullptr);
		sprites.UpdateAnimation(cat, deltaTime);

		if (!Render()) {
			MessageBoxW(window, L"고양이 프레임을 표시하지 못했습니다.", L"Desktop Cat", MB_OK | MB_ICONERROR);
			return 1;
		}

		Sleep(1);
	}
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	auto* application = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if (message == WM_NCCREATE) {
		const auto* creation = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		application = static_cast<Application*>(creation->lpCreateParams);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
		application->window = hwnd;
	}

	if (!application)
		return DefWindowProcW(hwnd, message, wParam, lParam);

	if (message == WM_NCDESTROY) {
		application->window = nullptr;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return application->HandleMessage(hwnd, message, wParam, lParam);
}

LRESULT Application::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_LBUTTONDOWN: {
			POINT cursor { };
			if (GetCursorPos(&cursor)) {
				simulation.BeginDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), cursor);
				SetCapture(hwnd);
				SetCursor(grabbingCursor);
			}
			return 0;
		}

		case WM_LBUTTONUP:
			simulation.EndDrag();
			if (GetCapture() == hwnd)
				ReleaseCapture();
			SetCursor(grabCursor);
			return 0;

		case WM_CANCELMODE:
			simulation.EndDrag(false);
			if (GetCapture() == hwnd)
				ReleaseCapture();
			return 0;

		case WM_CAPTURECHANGED:
			if (reinterpret_cast<HWND>(lParam) != hwnd)
				simulation.EndDrag(false);
			return 0;

		case WM_SETCURSOR:
			SetCursor(cat.dragging ? grabbingCursor : grabCursor);
			return TRUE;

		case WM_NCHITTEST: {
			RECT bounds { };
			if (!GetWindowRect(hwnd, &bounds))
				return HTCLIENT;
			return sprites.HitTest(cat, GET_X_LPARAM(lParam) - bounds.left,
				GET_Y_LPARAM(lParam) - bounds.top) ? HTCLIENT : HTTRANSPARENT;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
	}
}
