#pragma once

#include <windows.h>
#include <vector>

struct Cat;

// Tracks desktop windows that can support the cat, in desktop Z order.
class DesktopPlatforms {
public:
	void SetPetWindow(HWND window);
	void Refresh(float deltaTime);
	void UpdateStandingPlatform(Cat& cat);
	void UpdatePlatformSupport(Cat& cat, float deltaTime);
	void UpdateStandingWindowOcclusion(Cat& cat, float deltaTime);
	void UpdateStandingWindowTransfer(Cat& cat);
	bool ResolveWindowLanding(Cat& cat, float previousX, float previousY);
	bool HasPlatformAhead(const Cat& cat) const;
	static float FloorY(const Cat& cat);

private:
	struct Platform {
		HWND window;
		RECT bounds;
	};

	static BOOL CALLBACK EnumerateWindow(HWND window, LPARAM context);
	static bool IsWindowCloaked(HWND window);
	static bool GetWindowBounds(HWND window, RECT& bounds);
	static bool IsStandingWindowSupportingCat(const Cat& cat);
	static void LeavePlatform(Cat& cat);

	bool IsPlatformWindow(HWND window) const;
	HWND TopPlatformWindowAt(POINT point) const;
	bool IsVisibleAt(const Platform& platform, float x) const;
	bool IsVisibleForCat(const Platform& platform, float left, float right) const;
	bool IsStandingWindowOccluded(const Cat& cat) const;
	const Platform* FindPlatformAt(float x, float feetY) const;

	std::vector<Platform> platforms_;
	HWND petWindow_ = nullptr;
	float refreshTimer_ = 0.0f;
};
