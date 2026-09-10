#pragma once

#include "Cat.h"
#include "CatSimulation.h"
#include "CatSprites.h"
#include "DesktopPlatforms.h"
#include "PetRenderer.h"
#include <string>

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
	bool Render();

	HINSTANCE instance;
	HWND window = nullptr;
	ATOM windowClass = 0;
	HCURSOR grabCursor = nullptr;
	HCURSOR grabbingCursor = nullptr;
	Cat cat;
	DesktopPlatforms platforms;
	CatSimulation simulation;
	CatSprites sprites;
	PetRenderer renderer;
};
