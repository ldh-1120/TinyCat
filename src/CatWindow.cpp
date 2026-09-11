#include "CatWindow.h"
#include "CatConfig.h"
#include "resource.h"
#include <windowsx.h>

using namespace CatConfig;

namespace {
	constexpr wchar_t WindowClassName[] = L"TinyCatWindow";
}

ATOM CatWindow::RegisterWindowClass(HINSTANCE instance) {
	WNDCLASSW definition { };
	definition.lpfnWndProc = WindowProc;
	definition.hInstance = instance;
	definition.lpszClassName = WindowClassName;
	definition.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_CAT));
	return RegisterClassW(&definition);
}

void CatWindow::UnregisterWindowClass(HINSTANCE instance) {
	UnregisterClassW(WindowClassName, instance);
}

CatWindow::CatWindow(const CatSprites& sprites, HCURSOR grabCursor, HCURSOR grabbingCursor)
	: sprites(sprites), grabCursor(grabCursor), grabbingCursor(grabbingCursor), simulation(cat, platforms) {}

CatWindow::~CatWindow() {
	if (window)
		DestroyWindow(window);
}

bool CatWindow::Create(HINSTANCE instance, POINT position, std::wstring& error) {
	if (window) {
		error = L"The cat window is already open.";
		return false;
	}

	if (!renderer.Initialize(RenderWidth, RenderHeight)) {
		error = L"Failed to initialize the cat renderer.";
		return false;
	}

	cat.x = static_cast<float>(position.x);
	cat.y = static_cast<float>(position.y);
	window = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
		WindowClassName, L"Tiny Cat", WS_POPUP,
		static_cast<int>(cat.x) - static_cast<int>(RenderPadding),
		static_cast<int>(cat.y) - static_cast<int>(RenderPadding),
		RenderWidth, RenderHeight, nullptr, nullptr, instance, this);

	if (!window) {
		error = L"Failed to create the cat window.";
		return false;
	}

	platforms.SetPetWindow(window);
	if (!Render()) {
		error = L"Failed to display the cat's first frame.";
		DestroyWindow(window);
		return false;
	}

	ShowWindow(window, SW_SHOWNOACTIVATE);
	return true;
}

bool CatWindow::Update(float deltaTime, const POINT* cursor) {
	if (!IsOpen())
		return true;

	simulation.Update(deltaTime, cursor);
	sprites.UpdateAnimation(cat, deltaTime);
	return Render();
}

bool CatWindow::Render() {
	const Image& frame = sprites.CurrentFrame(cat);
	const Image rendered = RotateImageNearest(frame, cat.rotation, ScruffX, ScruffY);
	return renderer.Render(window, rendered,
		static_cast<int>(cat.x + cat.impactOffsetX) - static_cast<int>(RenderPadding),
		static_cast<int>(cat.y + cat.impactOffsetY) - static_cast<int>(RenderPadding));
}

LRESULT CALLBACK CatWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	auto* catWindow = reinterpret_cast<CatWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if (message == WM_NCCREATE) {
		const auto* creation = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		catWindow = static_cast<CatWindow*>(creation->lpCreateParams);
		if (!catWindow)
			return FALSE;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(catWindow));
		catWindow->window = hwnd;
	}

	if (!catWindow)
		return DefWindowProcW(hwnd, message, wParam, lParam);

	if (message == WM_NCDESTROY) {
		catWindow->window = nullptr;
		catWindow->platforms.SetPetWindow(nullptr);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return catWindow->HandleMessage(hwnd, message, wParam, lParam);
}

LRESULT CatWindow::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
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
		case WM_DESTROY:
			simulation.EndDrag(false);
			if (GetCapture() == hwnd)
				ReleaseCapture();
			return 0;

		case WM_CAPTURECHANGED:
			if (reinterpret_cast<HWND>(lParam) != hwnd)
				simulation.EndDrag(false);
			return 0;

		case WM_SETCURSOR:
			if (LOWORD(lParam) != HTCLIENT)
				return DefWindowProcW(hwnd, message, wParam, lParam);
			SetCursor(cat.dragging ? grabbingCursor : grabCursor);
			return TRUE;

		case WM_NCHITTEST: {
			RECT bounds { };
			if (!GetWindowRect(hwnd, &bounds))
				return HTCLIENT;
			return sprites.HitTest(cat, GET_X_LPARAM(lParam) - bounds.left,
				GET_Y_LPARAM(lParam) - bounds.top) ? HTCLIENT : HTTRANSPARENT;
		}

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
	}
}
