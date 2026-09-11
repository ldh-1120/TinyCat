#include "Application.h"
#include "CatConfig.h"
#include "resource.h"

#include <algorithm>
#include <exception>

using namespace CatConfig;

namespace {
	constexpr wchar_t WindowClassName[] = L"TinyCatTrayWindow";

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

Application::Application(HINSTANCE instance) : instance(instance) {}

Application::~Application() {
	shuttingDown = true;
	cats.clear();
	trayIcon.Reset();
	if (window)
		DestroyWindow(window);
	if (catWindowClass)
		CatWindow::UnregisterWindowClass(instance);
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
		error = L"Unable to locate the executable directory.";
		return false;
	}
	if (!sprites.Load(directory, error))
		return false;

	grabCursor = LoadCursorFromFileW((directory + L"\\grab.cur").c_str());
	grabbingCursor = LoadCursorFromFileW((directory + L"\\grabbing.cur").c_str());
	if (!grabCursor || !grabbingCursor) {
		error = L"Unable to load grab.cur or grabbing.cur.";
		return false;
	}
	catWindowClass = CatWindow::RegisterWindowClass(instance);
	if (!catWindowClass) {
		error = L"Unable to register the cat window class.";
		return false;
	}

	WNDCLASSW definition { };
	definition.lpfnWndProc = WindowProc;
	definition.hInstance = instance;
	definition.lpszClassName = WindowClassName;
	definition.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_CAT));
	definition.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	windowClass = RegisterClassW(&definition);
	if (!windowClass) {
		error = L"Unable to register the tray window class.";
		return false;
	}
	// A hidden top-level window keeps the tray alive independently of any cat.
	window = CreateWindowExW(WS_EX_TOOLWINDOW, WindowClassName, L"Tiny Cat", WS_POPUP,
		0, 0, 0, 0, nullptr, nullptr, instance, this);
	if (!window) {
		error = L"Unable to create the tray window.";
		return false;
	}
	taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
	if (!trayIcon.Create(instance, window)) {
		error = L"Unable to create the notification area icon.";
		return false;
	}
	return AddCat(error);
}

POINT Application::SpawnPosition() const {
	POINT position { 300, 100 };
	if (!cats.empty())
		GetCursorPos(&position);
	const HMONITOR monitor = MonitorFromPoint(position, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info { };
	info.cbSize = sizeof(info);
	if (!GetMonitorInfoW(monitor, &info))
		return position;

	if (!cats.empty()) {
		// Spread new cats across the active monitor so additions are easy to see.
		const int column = static_cast<int>((cats.size() - 1) % 5) - 2;
		const int row = static_cast<int>(((cats.size() - 1) / 5) % 4);
		position.x = info.rcWork.left + (info.rcWork.right - info.rcWork.left) / 2 -
			static_cast<LONG>(CatWidth) / 2 + column * static_cast<LONG>(RenderWidth);
		position.y = info.rcWork.top + 100 + row * static_cast<LONG>(RenderHeight);
	}
	position.x = std::clamp(position.x, info.rcWork.left,
		std::max(info.rcWork.left, info.rcWork.right - static_cast<LONG>(CatWidth)));
	position.y = std::clamp(position.y, info.rcWork.top,
		std::max(info.rcWork.top, info.rcWork.bottom - static_cast<LONG>(CatHeight)));
	return position;
}

bool Application::AddCat(std::wstring& error) {
	if (shuttingDown)
		return false;
	try {
		auto pet = std::make_unique<CatWindow>(sprites, grabCursor, grabbingCursor);
		if (!pet->Create(instance, SpawnPosition(), error))
			return false;
		cats.push_back(std::move(pet));
		return true;
	} catch (const std::exception&) {
		error = L"Unable to add another cat. The system may be low on resources.";
		return false;
	}
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
		std::erase_if(cats, [](const auto& pet) { return !pet->IsOpen(); });

		LARGE_INTEGER currentTime { };
		if (!QueryPerformanceCounter(&currentTime))
			return 1;
		const double elapsed = static_cast<double>(currentTime.QuadPart - previousTime.QuadPart) /
			static_cast<double>(frequency.QuadPart);
		previousTime = currentTime;
		const float deltaTime = std::clamp(static_cast<float>(elapsed), 0.0f, 0.05f);

		POINT cursor { };
		const bool hasCursor = GetCursorPos(&cursor) != FALSE;
		for (const auto& pet : cats) {
			if (!pet->Update(deltaTime, hasCursor ? &cursor : nullptr)) {
				MessageBoxW(window, L"Unable to render a cat frame.", L"Tiny Cat", MB_OK | MB_ICONERROR);
				return 1;
			}
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
	if (taskbarCreatedMessage != 0 && message == taskbarCreatedMessage) {
		if (!shuttingDown && !trayIcon.Restore())
			MessageBoxW(hwnd, L"Unable to restore the notification area icon.", L"Tiny Cat", MB_OK | MB_ICONERROR);
		return 0;
	}
	switch (message) {
		case WM_TRAYICON: {
			const UINT event = LOWORD(lParam);
			if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) {
				const UINT command = trayIcon.ShowMenu();
				if (command != 0)
					PostMessageW(hwnd, WM_COMMAND, command, 0);
			}
			return 0;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case MenuAddCat: {
					std::wstring error;
					if (!AddCat(error) && !shuttingDown)
						MessageBoxW(hwnd, error.c_str(), L"Tiny Cat", MB_OK | MB_ICONERROR);
					return 0;
				}
				case MenuExit:
					DestroyWindow(hwnd);
					return 0;
			}
			break;
		case WM_DESTROY:
			shuttingDown = true;
			trayIcon.Reset();
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProcW(hwnd, message, wParam, lParam);
}
