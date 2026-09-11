#pragma once

#include "CatSprites.h"
#include "CatWindow.h"
#include "TrayIcon.h"
#include <memory>
#include <string>
#include <vector>

class Application {
public:
	explicit Application(HINSTANCE instance);
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	bool Initialize(std::wstring& error);
	int Run();

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool AddCat(std::wstring& error);
	POINT SpawnPosition() const;

	HINSTANCE instance;
	HWND window = nullptr;
	ATOM windowClass = 0;
	ATOM catWindowClass = 0;
	UINT taskbarCreatedMessage = 0;
	HCURSOR grabCursor = nullptr;
	HCURSOR grabbingCursor = nullptr;
	bool shuttingDown = false;
	CatSprites sprites;
	TrayIcon trayIcon;
	std::vector<std::unique_ptr<CatWindow>> cats;
};
