#include "TrayIcon.h"
#include "CatConfig.h"
#include "resource.h"

#pragma comment(lib, "shell32.lib")

using namespace CatConfig;

TrayIcon::~TrayIcon() {
	Reset();
}

bool TrayIcon::Create(HINSTANCE instance, HWND owner) {
	Reset();
	data.cbSize = sizeof(data);
	data.hWnd = owner;
	data.uID = TrayIconId;
	data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	data.uCallbackMessage = WM_TRAYICON;
	data.hIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_CAT),
		IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
	lstrcpyW(data.szTip, L"Tiny Cat");
	if (!data.hIcon || !AddToShell()) {
		Reset();
		return false;
	}
	return true;
}

bool TrayIcon::AddToShell() {
	installed = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
	if (installed) {
		data.uVersion = NOTIFYICON_VERSION_4;
		Shell_NotifyIconW(NIM_SETVERSION, &data);
	}
	return installed;
}

bool TrayIcon::Restore() {
	// Explorer discarded the old icon when its taskbar was recreated.
	installed = false;
	return data.hWnd && data.hIcon && AddToShell();
}

void TrayIcon::Reset() noexcept {
	if (installed)
		Shell_NotifyIconW(NIM_DELETE, &data);
	if (data.hIcon)
		DestroyIcon(data.hIcon);
	data = { };
	installed = false;
}

UINT TrayIcon::ShowMenu() {
	if (menuOpen || !data.hWnd)
		return 0;
	HMENU menu = CreatePopupMenu();
	if (!menu)
		return 0;
	if (!AppendMenuW(menu, MF_STRING, MenuAddCat, L"Add Cat") ||
		!AppendMenuW(menu, MF_SEPARATOR, 0, nullptr) ||
		!AppendMenuW(menu, MF_STRING, MenuExit, L"Exit")) {
		DestroyMenu(menu);
		return 0;
	}
	POINT cursor { };
	if (!GetCursorPos(&cursor)) {
		DestroyMenu(menu);
		return 0;
	}

	menuOpen = true;
	SetCursor(LoadCursorW(nullptr, IDC_ARROW));
	SetForegroundWindow(data.hWnd);
	const UINT command = TrackPopupMenu(menu,
		TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
		cursor.x, cursor.y, 0, data.hWnd, nullptr);
	menuOpen = false;
	DestroyMenu(menu);
	PostMessageW(data.hWnd, WM_NULL, 0, 0);
	return command;
}
