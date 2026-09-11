#pragma once

#include <windows.h>
#include <shellapi.h>

// One notification icon and menu for the entire application.
class TrayIcon {
public:
	TrayIcon() = default;
	~TrayIcon();
	TrayIcon(const TrayIcon&) = delete;
	TrayIcon& operator=(const TrayIcon&) = delete;

	bool Create(HINSTANCE instance, HWND owner);
	bool Restore();
	void Reset() noexcept;
	UINT ShowMenu();

private:
	bool AddToShell();
	NOTIFYICONDATAW data { };
	bool installed = false;
	bool menuOpen = false;
};
