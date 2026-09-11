#include "DesktopPlatforms.h"

#include "Cat.h"
#include "CatConfig.h"

#include <cmath>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

using namespace CatConfig;

void DesktopPlatforms::SetPetWindow(HWND window) {
	petWindow_ = window;
}

void DesktopPlatforms::Refresh(float deltaTime) {
	refreshTimer_ += deltaTime;
	if (refreshTimer_ < PlatformUpdateInterval)
		return;

	refreshTimer_ = 0.0f;
	platforms_.clear();
	EnumWindows(EnumerateWindow, reinterpret_cast<LPARAM>(this));
}

float DesktopPlatforms::FloorY(const Cat& cat) {
	POINT position { static_cast<LONG>(cat.x + static_cast<float>(CatWidth) * 0.5f), static_cast<LONG>(cat.y + static_cast<float>(CatHeight) * 0.5f) };
	HMONITOR monitor = MonitorFromPoint(position, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info { };

	info.cbSize = sizeof(info);
	if (!GetMonitorInfoW(monitor, &info))
		return cat.y;

	return static_cast<float>(info.rcWork.bottom) - static_cast<float>(CatHeight);
}

bool DesktopPlatforms::IsWindowCloaked(HWND window) {
	DWORD cloaked = 0;
	HRESULT result = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
	return SUCCEEDED(result) && cloaked != 0;
}

bool DesktopPlatforms::IsPlatformWindow(HWND window) const {
	if (window == petWindow_ || !IsWindowVisible(window) || IsIconic(window) || IsWindowCloaked(window))
		return false;

	LONG_PTR extendedStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
	if ((extendedStyle & (WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE)) != 0)
		return false;

	if ((GetWindowLongPtrW(window, GWL_STYLE) & WS_DISABLED) != 0)
		return false;

	RECT bounds { };
	HRESULT result = DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds));

	return SUCCEEDED(result) && bounds.right > bounds.left && bounds.bottom > bounds.top;
}

bool DesktopPlatforms::GetWindowBounds(HWND window, RECT& bounds) {
	if (!IsWindow(window) || !IsWindowVisible(window) || IsIconic(window) || IsWindowCloaked(window))
		return false;

	return SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds)));
}

BOOL CALLBACK DesktopPlatforms::EnumerateWindow(HWND window, LPARAM context) {
	auto& desktop = *reinterpret_cast<DesktopPlatforms*>(context);
	if (!desktop.IsPlatformWindow(window))
		return TRUE;

	RECT bounds { };
	if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds))))
		desktop.platforms_.push_back({ window, bounds });

	return TRUE;
}

void DesktopPlatforms::LeavePlatform(Cat& cat) {
	cat.grounded = false;
	cat.standingWindow = nullptr;
	cat.supportLostTimer = 0.0f;
	cat.occlusionTimer = 0.0f;
	cat.occludedTime = 0.0f;
	cat.occlusionIgnoreTimer = 0.0f;
	cat.velocityY = 0.0f;
	cat.SetState(CatState::Fall);
}

void DesktopPlatforms::UpdateStandingPlatform(Cat& cat) {
	if (!cat.grounded || !cat.standingWindow)
		return;

	RECT newBounds { };
	if (!GetWindowBounds(cat.standingWindow, newBounds)) {
		LeavePlatform(cat);
		return;
	}

	int deltaX = newBounds.left - cat.standingWindowRect.left;
	int deltaY = newBounds.top - cat.standingWindowRect.top;
	cat.x += static_cast<float>(deltaX);
	cat.y += static_cast<float>(deltaY);
	cat.standingWindowRect = newBounds;

	if (deltaX != 0 || deltaY != 0) {
		cat.occlusionIgnoreTimer = OcclusionAfterWindowMoveGraceTime;
		cat.occlusionTimer = 0.0f;
		cat.occludedTime = 0.0f;
	}

	for (Platform& platform : platforms_) {
		if (platform.window == cat.standingWindow) {
			platform.bounds = newBounds;
			break;
		}
	}
}

HWND DesktopPlatforms::TopPlatformWindowAt(POINT point) const {
	for (HWND window = GetTopWindow(nullptr); window; window = GetWindow(window, GW_HWNDNEXT)) {
		if (!IsPlatformWindow(window))
			continue;

		RECT bounds { };
		if (GetWindowBounds(window, bounds) && PtInRect(&bounds, point))
			return window;
	}
	return nullptr;
}

bool DesktopPlatforms::IsVisibleAt(const Platform& platform, float x) const {
	POINT point { static_cast<LONG>(x), platform.bounds.top + 1 };
	return TopPlatformWindowAt(point) == platform.window;
}

bool DesktopPlatforms::IsVisibleForCat(const Platform& platform, float left, float right) const {
	const float samples[] { left, (left + right) * 0.5f, right };
	int visibleCount = 0;
	for (float x : samples) {
		if (IsVisibleAt(platform, x))
			++visibleCount;
	}

	return visibleCount >= 2;
}

bool DesktopPlatforms::IsStandingWindowSupportingCat(const Cat& cat) {
	RECT bounds { };
	if (!cat.standingWindow || !GetWindowBounds(cat.standingWindow, bounds))
		return false;

	float catLeft = cat.x + FootInset;
	float catRight = cat.x + static_cast<float>(CatWidth) - FootInset;
	return catRight > static_cast<float>(bounds.left) && catLeft < static_cast<float>(bounds.right);
}

bool DesktopPlatforms::ResolveWindowLanding(Cat& cat, float previousX, float previousY) {
	if (cat.velocityY <= 0.0f)
		return false;

	float previousFeetY = previousY + static_cast<float>(CatHeight);
	float currentFeetY = cat.y + static_cast<float>(CatHeight);
	float movementY = currentFeetY - previousFeetY;
	if (movementY <= 0.0f)
		return false;

	const Platform* landingPlatform = nullptr;
	float bestTime = 2.0f;
	for (const Platform& platform : platforms_) {
		float platformTop = static_cast<float>(platform.bounds.top);
		if (previousFeetY >= platformTop || currentFeetY < platformTop)
			continue;

		// Sweep between the previous and current positions to avoid skipping a window.
		float collisionTime = (platformTop - previousFeetY) / movementY;
		float landingX = previousX + (cat.x - previousX) * collisionTime;
		float catLeft = landingX + FootInset;
		float catRight = landingX + static_cast<float>(CatWidth) - FootInset;
		if (catRight <= static_cast<float>(platform.bounds.left) || catLeft >= static_cast<float>(platform.bounds.right))
			continue;

		if (!IsVisibleForCat(platform, catLeft, catRight) || collisionTime >= bestTime)
			continue;

		bestTime = collisionTime;
		landingPlatform = &platform;
	}

	if (!landingPlatform)
		return false;

	cat.y = static_cast<float>(landingPlatform->bounds.top) - static_cast<float>(CatHeight);
	cat.velocityY = 0.0f;
	cat.grounded = true;
	cat.standingWindow = landingPlatform->window;
	cat.standingWindowRect = landingPlatform->bounds;

	cat.supportLostTimer = 0.0f;
	cat.occlusionTimer = 0.0f;
	cat.occludedTime = 0.0f;
	cat.SetState(CatState::Land);

	return true;
}

void DesktopPlatforms::UpdatePlatformSupport(Cat& cat, float deltaTime) {
	if (!cat.grounded || !cat.standingWindow)
		return;

	if (IsStandingWindowSupportingCat(cat)) {
		cat.supportLostTimer = 0.0f;
		return;
	}

	cat.supportLostTimer += deltaTime;
	if (cat.supportLostTimer >= SupportLostGraceTime)
		LeavePlatform(cat);
}

bool DesktopPlatforms::IsStandingWindowOccluded(const Cat& cat) const {
	if (!cat.standingWindow)
		return false;

	float catLeft = cat.x + FootInset;
	float catRight = cat.x + static_cast<float>(CatWidth) - FootInset;
	Platform platform { cat.standingWindow, cat.standingWindowRect };
	return !IsVisibleForCat(platform, catLeft, catRight);
}

void DesktopPlatforms::UpdateStandingWindowOcclusion(Cat& cat, float deltaTime) {
	if (!cat.grounded || !cat.standingWindow) {
		cat.occlusionTimer = 0.0f;
		cat.occludedTime = 0.0f;
		cat.occlusionIgnoreTimer = 0.0f;
		return;
	}

	if (cat.occlusionIgnoreTimer > 0.0f) {
		cat.occlusionIgnoreTimer -= deltaTime;
		if (cat.occlusionIgnoreTimer < 0.0f)
			cat.occlusionIgnoreTimer = 0.0f;
		cat.occlusionTimer = 0.0f;
		cat.occludedTime = 0.0f;
		return;
	}

	cat.occlusionTimer += deltaTime;
	if (cat.occlusionTimer < OcclusionCheckInterval)
		return;

	float checkTime = cat.occlusionTimer;
	cat.occlusionTimer = 0.0f;
	if (IsStandingWindowOccluded(cat)) {
		cat.occludedTime += checkTime;
		if (cat.occludedTime >= OcclusionGraceTime)
			LeavePlatform(cat);
	} else
		cat.occludedTime = 0.0f;
}

const DesktopPlatforms::Platform* DesktopPlatforms::FindPlatformAt(float x, float feetY) const {
	constexpr float HeightTolerance = 2.0f;
	for (const Platform& platform : platforms_) {
		if (x < static_cast<float>(platform.bounds.left) || x >= static_cast<float>(platform.bounds.right))
			continue;

		if (std::abs(static_cast<float>(platform.bounds.top) - feetY) > HeightTolerance)
			continue;

		if (IsVisibleAt(platform, x))
			return &platform;
	}

	return nullptr;
}

void DesktopPlatforms::UpdateStandingWindowTransfer(Cat& cat) {
	if (!cat.grounded || !cat.standingWindow)
		return;

	float feetY = cat.y + static_cast<float>(CatHeight);
	float footCenterX = cat.x + static_cast<float>(CatWidth) * 0.5f;
	const Platform* platform = FindPlatformAt(footCenterX, feetY);
	if (!platform || platform->window == cat.standingWindow)
		return;

	cat.standingWindow = platform->window;
	cat.standingWindowRect = platform->bounds;
	cat.y = static_cast<float>(platform->bounds.top) - static_cast<float>(CatHeight);
	cat.velocityY = 0.0f;

	cat.supportLostTimer = 0.0f;
	cat.occlusionTimer = 0.0f;
	cat.occludedTime = 0.0f;
}

bool DesktopPlatforms::HasPlatformAhead(const Cat& cat) const {
	if (!cat.grounded)
		return false;

	if (!cat.standingWindow)
		return true;

	constexpr float LookAhead = 1.5f * static_cast<float>(SpriteScale);
	float sampleX = cat.facingRight ? cat.x + static_cast<float>(CatWidth) - FootInset + LookAhead : cat.x + FootInset - LookAhead;

	return FindPlatformAt(sampleX, cat.y + static_cast<float>(CatHeight)) != nullptr;
}
