#include "CatSprites.h"
#include "Cat.h"
#include "CatConfig.h"

#include <algorithm>
#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

using namespace CatConfig;

namespace {
	bool UsesIdleFrames(CatState state) {
		return state == CatState::Idle || state == CatState::Edge || state == CatState::WatchCursor;
	}
}

bool CatSprites::LoadFrames(const std::wstring& path, UINT frameCount, Frames& frames,
	std::wstring& error) {
	Image sheet;
	if (!LoadPng(path, sheet)) {
		error = L"Unable to load sprite image: " + path;
		return false;
	}

	if (sheet.width < frameCount * SpriteSize || sheet.height < SpriteSize) {
		error = L"Sprite image is too small: " + path + L" (requires at least " +
			std::to_wstring(frameCount * SpriteSize) + L" x " + std::to_wstring(SpriteSize) + L")";
		return false;
	}

	frames.right.reserve(frameCount);
	frames.left.reserve(frameCount);
	for (UINT index = 0; index < frameCount; ++index) {
		auto right = CreateSpriteFrame(sheet, index * SpriteSize, 0, SpriteSize, SpriteSize, SpriteScale, false);
		auto left = CreateSpriteFrame(sheet, index * SpriteSize, 0, SpriteSize, SpriteSize, SpriteScale, true);
		if (!right.IsValid() || !left.IsValid()) {
			error = L"Unable to create sprite frames: " + path;
			return false;
		}

		frames.right.push_back(std::move(right));
		frames.left.push_back(std::move(left));
	}
	return true;
}

bool CatSprites::Load(const std::wstring& directory, std::wstring& error) {
	error.clear();
	const std::wstring prefix = directory.empty() || directory.back() == L'\\' || directory.back() == L'/' ? directory : directory + L"\\";

	// Commit only after every sheet has been decoded and validated, so retries are safe.
	CatSprites loaded;
	try {
		if (!LoadFrames(prefix + L"cat_walk.png", WalkFrameCount, loaded.walk, error) ||
			!LoadFrames(prefix + L"cat_idle.png", IdleFrameCount, loaded.idle, error) ||
			!LoadFrames(prefix + L"cat_drag.png", DragHangFrameCount + 2, loaded.drag, error) ||
			!LoadFrames(prefix + L"cat_fall.png", FallFrameCount, loaded.fall, error) ||
			!LoadFrames(prefix + L"cat_land.png", LandFrameCount, loaded.land, error))
			return false;
	} catch (const std::bad_alloc&) {
		error = L"Not enough memory to load sprite images.";
		return false;
	} catch (const std::length_error&) {
		error = L"Sprite image dimensions are too large.";
		return false;
	}

	*this = std::move(loaded);
	return true;
}

const Image& CatSprites::SelectFrame(const Frames& frames, bool facingRight, int index) {
	const auto& direction = facingRight ? frames.right : frames.left;
	if (direction.empty()) {
		static const Image empty;
		return empty;
	}

	const std::size_t frame = index < 0 ? 0 : (std::min)(static_cast<std::size_t>(index), direction.size() - 1);
	return direction[frame];
}

const Image& CatSprites::CurrentFrame(const Cat& cat) const {
	if (cat.dragging) {
		switch (cat.dragPose) {
			case DragPose::LeanLeft:

				return SelectFrame(drag, cat.dragFacingRight, cat.dragFacingRight ? 2 : 3);
			case DragPose::LeanRight:
				return SelectFrame(drag, cat.dragFacingRight, cat.dragFacingRight ? 3 : 2);

			case DragPose::Hanging:
				return SelectFrame(drag, cat.dragFacingRight, (std::clamp)(cat.frame, 0, static_cast<int>(DragHangFrameCount) - 1));
		}
	}

	if (cat.state == CatState::Fall)
		return SelectFrame(fall, cat.facingRight, cat.frame);

	if (cat.state == CatState::Land)
		return SelectFrame(land, cat.facingRight, cat.frame);

	if (UsesIdleFrames(cat.state))
		return SelectFrame(idle, cat.facingRight, cat.frame);

	return SelectFrame(walk, cat.facingRight, cat.frame);
}

bool CatSprites::HitTest(const Cat&, int x, int y) const {
	// Preserve the forgiving rectangular grab area used by the original application.
	return x >= static_cast<int>(RenderPadding) && x < static_cast<int>(RenderPadding + CatWidth) &&
		y >= static_cast<int>(RenderPadding) && y < static_cast<int>(RenderPadding + CatHeight);
}

void CatSprites::UpdateAnimation(Cat& cat, float deltaTime) const {
	if (!std::isfinite(deltaTime) || deltaTime < 0.0f)
		return;

	float frameTime = WalkFrameTime;
	UINT frameCount = WalkFrameCount;
	if (cat.dragging) {
		if (cat.dragPose != DragPose::Hanging) {
			cat.frame = 0;
			cat.animationTimer = 0.0f;
			return;
		}
		frameTime = DragAnimationFrameTime;
		frameCount = DragHangFrameCount;
	} else if (UsesIdleFrames(cat.state)) {
		frameTime = IdleFrameTime;
		frameCount = IdleFrameCount;
	} else if (cat.state == CatState::Fall) {
		frameTime = FallFrameTime;
		frameCount = FallFrameCount;
	} else if (cat.state == CatState::Land) {
		frameTime = LandFrameTime;
		frameCount = LandFrameCount;
	}

	cat.frame = (std::clamp)(cat.frame, 0, static_cast<int>(frameCount) - 1);
	if (!std::isfinite(cat.animationTimer) || cat.animationTimer < 0.0f)
		cat.animationTimer = 0.0f;

	const double elapsed = static_cast<double>(cat.animationTimer) + deltaTime;
	const double steps = std::floor(elapsed / frameTime);
	cat.animationTimer = static_cast<float>(std::fmod(elapsed, frameTime));
	if (!cat.dragging && cat.state == CatState::Land) {
		cat.frame += static_cast<int>((std::min)(steps,
			static_cast<double>(frameCount - 1 - cat.frame)));
	} else
		cat.frame = (cat.frame + static_cast<int>(std::fmod(steps, frameCount))) % static_cast<int>(frameCount);
}
