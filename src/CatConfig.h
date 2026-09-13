#pragma once

#include <windows.h>

namespace CatConfig {
	inline constexpr UINT WM_TRAYICON = WM_APP + 1;
	inline constexpr UINT TrayIconId = 1;
	inline constexpr UINT MenuExit = 1001;
	inline constexpr UINT MenuAddCat = 1002;

	// Geometry and motion use physical screen pixels, matching the DPI manifest
	// and DWM platform bounds. Sprite pixels keep the same size on every monitor.
	inline constexpr UINT SpriteSize = 10;
	inline constexpr UINT SpriteScale = 6;
	inline constexpr UINT CatWidth = SpriteSize * SpriteScale;
	inline constexpr UINT CatHeight = SpriteSize * SpriteScale;
	inline constexpr UINT RenderPadding = 12;
	inline constexpr UINT RenderWidth = CatWidth + RenderPadding * 2;
	inline constexpr UINT RenderHeight = CatHeight + RenderPadding * 2;

	inline constexpr UINT WalkFrameCount = 4;
	inline constexpr UINT IdleFrameCount = 4;
	inline constexpr UINT FallFrameCount = 2;
	inline constexpr UINT LandFrameCount = 3;
	inline constexpr UINT DragHangFrameCount = 2;
	inline constexpr float WalkFrameTime = 0.12f;
	inline constexpr float IdleFrameTime = 0.25f;
	inline constexpr float FallFrameTime = 0.16f;
	inline constexpr float LandFrameTime = 0.1f;
	inline constexpr float DragAnimationFrameTime = 0.18f;

	inline constexpr float Gravity = 1200.0f;
	inline constexpr float WalkSpeed = 70.0f;
	inline constexpr float MaximumThrowSpeed = 1800.0f;
	inline constexpr float ThrowFacingThreshold = 80.0f;
	inline constexpr float DragLeanThreshold = 140.0f;
	inline constexpr float IdleSpeedThreshold = 10.0f;
	inline constexpr float ScruffX = 3.5f * static_cast<float>(SpriteScale);
	inline constexpr float ScruffY = -1.0f * static_cast<float>(SpriteScale);

	inline constexpr float PlatformUpdateInterval = 0.1f;
	inline constexpr float FootInset = 2.0f * static_cast<float>(SpriteScale);
	inline constexpr float SupportLostGraceTime = 0.12f;
	inline constexpr float OcclusionCheckInterval = 0.1f;
	inline constexpr float OcclusionGraceTime = 0.3f;
	inline constexpr float OcclusionAfterWindowMoveGraceTime = 0.4f;

	inline constexpr float CursorNoticeDistance = 280.0f;
	inline constexpr float CursorChaseDistance = 75.0f;
	inline constexpr float CursorInterestingSpeed = 1200.0f;
	inline constexpr float CursorChaseSpeed = 95.0f;
	inline constexpr float CursorInterestDuration = 4.0f;
	inline constexpr float CursorPlayCooldownTime = 6.0f;
}
