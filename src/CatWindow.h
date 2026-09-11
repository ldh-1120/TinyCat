#pragma once

#include "Cat.h"
#include "CatSimulation.h"
#include "CatSprites.h"
#include "DesktopPlatforms.h"
#include "PetRenderer.h"
#include <string>

// Owns one cat's state, simulation, renderer, and desktop window.
// Shared sprites and cursors must outlive every CatWindow that uses them.
class CatWindow {
public:
	static ATOM RegisterWindowClass(HINSTANCE instance);
	static void UnregisterWindowClass(HINSTANCE instance);

	CatWindow(const CatSprites& sprites, HCURSOR grabCursor, HCURSOR grabbingCursor);
	~CatWindow();
	CatWindow(const CatWindow&) = delete;
	CatWindow& operator=(const CatWindow&) = delete;

	bool Create(HINSTANCE instance, POINT position, std::wstring& error);
	bool Update(float deltaTime, const POINT* cursor);
	bool IsOpen() const { return window != nullptr; }
	HWND Handle() const { return window; }

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool Render();

	const CatSprites& sprites;
	HCURSOR grabCursor;
	HCURSOR grabbingCursor;
	HWND window = nullptr;
	Cat cat;
	DesktopPlatforms platforms;
	CatSimulation simulation;
	PetRenderer renderer;
};
