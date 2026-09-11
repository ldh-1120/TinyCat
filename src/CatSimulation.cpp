#include "CatSimulation.h"
#include "CatConfig.h"

#include <algorithm>
#include <cmath>

using namespace CatConfig;

namespace {
	void UpdateSpring(float& offset, float& velocity, float target, float deltaTime) {
		constexpr float Spring = 80.0f;
		constexpr float Damping = 10.0f;
		velocity += (target - offset) * Spring * deltaTime;
		velocity /= 1.0f + Damping * deltaTime;
		offset += velocity * deltaTime;
	}
}

CatSimulation::CatSimulation(Cat& cat, DesktopPlatforms& platforms) : cat(cat), platforms(platforms) {}

void CatSimulation::BeginDrag(int localX, int localY, POINT cursor) {
	cat.dragging = true;
	cat.grounded = false;
	cat.standingWindow = nullptr;
	cat.dragFacingRight = cat.facingRight;
	cat.dragOffsetX = static_cast<float>(localX) - RenderPadding;
	cat.dragOffsetY = static_cast<float>(localY) - RenderPadding;
	cat.targetDragOffsetX = ScruffX;
	cat.targetDragOffsetY = ScruffY;
	cat.moveVelocityX = 0.0f;
	cat.physicsVelocityX = 0.0f;
	cat.velocityY = 0.0f;
	cat.thrown = false;
	cat.dragVelocityX = 0.0f;
	cat.dragVelocityY = 0.0f;
	cat.dragPose = DragPose::Hanging;
	cat.frame = 0;
	cat.animationTimer = 0.0f;
	cat.supportLostTimer = 0.0f;
	cat.occlusionTimer = 0.0f;
	cat.occludedTime = 0.0f;
	cat.occlusionIgnoreTimer = 0.0f;
	cat.previousCursorPosition = cursor;
}

void CatSimulation::EndDrag(bool applyThrow) {
	if (!cat.dragging)
		return;

	cat.dragging = false;
	cat.physicsVelocityX = applyThrow ? cat.dragVelocityX : 0.0f;
	cat.velocityY = applyThrow ? cat.dragVelocityY : 0.0f;
	cat.thrown = applyThrow;
	if (cat.physicsVelocityX > ThrowFacingThreshold)
		cat.facingRight = true;
	else if (cat.physicsVelocityX < -ThrowFacingThreshold)
		cat.facingRight = false;

	cat.SetState(CatState::Fall);
	cat.dragPose = DragPose::Hanging;
	cat.frame = 0;
	cat.animationTimer = 0.0f;
}

void CatSimulation::Update(float deltaTime, const POINT* cursor) {
	if (!std::isfinite(deltaTime) || deltaTime <= 0.0f)
		return;
	deltaTime = std::min(deltaTime, 0.05f);
	platforms.Refresh(deltaTime);
	if (cursor)
		UpdateCursorMotion(deltaTime, *cursor);

	if (cat.dragging) {
		if (cursor)
			UpdateDrag(deltaTime, *cursor);
	} else {
		platforms.UpdateStandingPlatform(cat);
		if (cursor)
			UpdateCursorInteraction(deltaTime, *cursor);
		UpdateMovement(deltaTime);
	}
	UpdateDynamics(deltaTime);
}

void CatSimulation::UpdateCursorMotion(float deltaTime, POINT cursor) {
	cat.cursorVelocityX = static_cast<float>(cursor.x - cat.previousPlayCursor.x) / deltaTime;
	cat.cursorVelocityY = static_cast<float>(cursor.y - cat.previousPlayCursor.y) / deltaTime;
	cat.previousPlayCursor = cursor;
}

void CatSimulation::FinishCursorInteraction() {
	cat.SetState(CatState::Idle);
	cat.cursorPlayCooldown = CursorPlayCooldownTime;
}

void CatSimulation::UpdateCursorInteraction(float deltaTime, POINT cursor) {
	if (cat.thrown || !cat.grounded)
		return;
	if (cat.cursorPlayCooldown > 0.0f) {
		cat.cursorPlayCooldown = std::max(0.0f, cat.cursorPlayCooldown - deltaTime);
		return;
	}

	const float dx = static_cast<float>(cursor.x) - (cat.x + CatWidth * 0.5f);
	const float dy = static_cast<float>(cursor.y) - (cat.y + CatHeight * 0.5f);
	const float distance = std::hypot(dx, dy);
	const float cursorSpeed = std::hypot(cat.cursorVelocityX, cat.cursorVelocityY);

	if (cat.state == CatState::Idle && distance <= CursorNoticeDistance &&
		cursorSpeed >= CursorInterestingSpeed) {
		cat.SetState(CatState::WatchCursor);
		cat.cursorInterestTimer = CursorInterestDuration;
	}

	if (cat.state == CatState::WatchCursor) {
		cat.cursorInterestTimer -= deltaTime;
		cat.moveVelocityX = 0.0f;
		if (dx > 30.0f)
			cat.facingRight = true;
		else if (dx < -30.0f)
			cat.facingRight = false;

		if (distance > CursorNoticeDistance || cat.cursorInterestTimer <= 0.0f)
			FinishCursorInteraction();
		else if (cursorSpeed >= CursorInterestingSpeed && distance > CursorChaseDistance)
			cat.SetState(CatState::ChaseCursor);

		return;
	}

	if (cat.state == CatState::ChaseCursor) {
		cat.cursorInterestTimer -= deltaTime;
		if (distance > CursorNoticeDistance * 1.5f || cat.cursorInterestTimer <= 0.0f) {
			FinishCursorInteraction();
			return;
		}

		if (std::abs(dx) <= CatWidth * 0.5f) {
			cat.SetState(CatState::WatchCursor);
			return;
		}

		cat.facingRight = dx > 0.0f;
		cat.moveVelocityX = cat.facingRight ? CursorChaseSpeed : -CursorChaseSpeed;
		if (!platforms.HasPlatformAhead(cat)) {
			cat.SetState(CatState::WatchCursor);
			cat.cursorInterestTimer = 1.0f;
		}
	}
}

void CatSimulation::UpdateBehavior(float deltaTime) {
	if (!cat.grounded || cat.state == CatState::WatchCursor || cat.state == CatState::ChaseCursor)
		return;

	if (cat.state == CatState::Land) {
		cat.stateTimer += deltaTime;
		if (cat.stateTimer >= cat.stateDuration)
			cat.SetState(CatState::Idle);
		return;
	}

	if (cat.thrown)
		return;

	cat.stateTimer += deltaTime;
	if (cat.stateTimer < cat.stateDuration)
		return;

	switch (cat.state) {
		case CatState::Idle:
			cat.SetState(CatState::Walk);
			break;

		case CatState::Walk:
			cat.SetState(CatState::Idle);
			break;

		case CatState::Edge:
			cat.TurnAwayFromEdge();
			break;

		default:
			break;
	}
}

void CatSimulation::UpdateDrag(float deltaTime, POINT cursor) {
	constexpr float DragAnchorSpeed = 12.0f;
	const float interpolation = 1.0f - std::exp(-DragAnchorSpeed * deltaTime);

	cat.dragOffsetX += (cat.targetDragOffsetX - cat.dragOffsetX) * interpolation;
	cat.dragOffsetY += (cat.targetDragOffsetY - cat.dragOffsetY) * interpolation;

	cat.x = static_cast<float>(cursor.x) - cat.dragOffsetX;
	cat.y = static_cast<float>(cursor.y) - cat.dragOffsetY;

	const float mouseVelocityX = static_cast<float>(cursor.x - cat.previousCursorPosition.x) / deltaTime;
	const float mouseVelocityY = static_cast<float>(cursor.y - cat.previousCursorPosition.y) / deltaTime;

	cat.dragVelocityX = std::clamp(cat.dragVelocityX * 0.7f + mouseVelocityX * 0.3f, -MaximumThrowSpeed, MaximumThrowSpeed);
	cat.dragVelocityY = std::clamp(cat.dragVelocityY * 0.7f + mouseVelocityY * 0.3f, -MaximumThrowSpeed, MaximumThrowSpeed);

	if (cat.dragVelocityX > DragLeanThreshold) {
		cat.dragPose = DragPose::LeanLeft;
		cat.facingRight = true;
	} else if (cat.dragVelocityX < -DragLeanThreshold) {
		cat.dragPose = DragPose::LeanRight;
		cat.facingRight = false;
	} else
		cat.dragPose = DragPose::Hanging;

	cat.previousCursorPosition = cursor;
}

void CatSimulation::UpdateMovement(float deltaTime) {
	const float previousX = cat.x;
	const float previousY = cat.y;

	const float desktopLeft = static_cast<float>(GetSystemMetrics(SM_XVIRTUALSCREEN));
	const float desktopTop = static_cast<float>(GetSystemMetrics(SM_YVIRTUALSCREEN));
	const float desktopRight = desktopLeft + GetSystemMetrics(SM_CXVIRTUALSCREEN);

	if (cat.grounded && !cat.thrown && cat.state == CatState::Walk && !platforms.HasPlatformAhead(cat))
		cat.SetState(CatState::Edge);

	cat.x += (cat.moveVelocityX + cat.physicsVelocityX) * deltaTime;
	if (cat.x + CatWidth > desktopRight) {
		cat.x = desktopRight - CatWidth;
		if (cat.physicsVelocityX > 0.0f) {
			cat.impactVelocityX += cat.physicsVelocityX * 0.03f;
			cat.physicsVelocityX *= -0.01f;
		}

		if (cat.moveVelocityX > 0.0f) {
			cat.moveVelocityX = -WalkSpeed;
			cat.facingRight = false;
		}
	}
	if (cat.x < desktopLeft) {
		cat.x = desktopLeft;
		if (cat.physicsVelocityX < 0.0f) {
			cat.impactVelocityX += cat.physicsVelocityX * 0.03f;
			cat.physicsVelocityX *= -0.01f;
		}

		if (cat.moveVelocityX < 0.0f) {
			cat.moveVelocityX = WalkSpeed;
			cat.facingRight = true;
		}
	}

	if (cat.grounded && cat.standingWindow)
		platforms.UpdateStandingWindowTransfer(cat);

	platforms.UpdatePlatformSupport(cat, deltaTime);
	platforms.UpdateStandingWindowOcclusion(cat, deltaTime);

	if (cat.grounded) {
		if (!cat.standingWindow) {
			cat.y = DesktopPlatforms::FloorY(cat);
			cat.velocityY = 0.0f;
		}
	} else {
		cat.velocityY += Gravity * deltaTime;
		cat.y += cat.velocityY * deltaTime;
		if (cat.y < desktopTop) {
			cat.y = desktopTop;
			cat.velocityY = std::max(0.0f, cat.velocityY);
		}

		if (!platforms.ResolveWindowLanding(cat, previousX, previousY)) {
			const float floorY = DesktopPlatforms::FloorY(cat);
			if (cat.y >= floorY) {
				cat.y = floorY;
				cat.velocityY = 0.0f;

				cat.grounded = true;
				cat.standingWindow = nullptr;

				cat.SetState(CatState::Land);
			}
		}
	}

	UpdateBehavior(deltaTime);
	if (cat.thrown) {
		if (cat.physicsVelocityX > ThrowFacingThreshold)
			cat.facingRight = true;
		else if (cat.physicsVelocityX < -ThrowFacingThreshold)
			cat.facingRight = false;
	} else if (std::abs(cat.moveVelocityX) > IdleSpeedThreshold)
		cat.facingRight = cat.moveVelocityX > 0.0f;
}

void CatSimulation::UpdateDynamics(float deltaTime) {
	constexpr float MaximumDragRotation = 30.0f;
	const float targetRotation = cat.dragging ? std::clamp(-cat.dragVelocityX * 0.01f, -MaximumDragRotation, MaximumDragRotation) : 0.0f;
	UpdateSpring(cat.rotation, cat.angularVelocity, targetRotation, deltaTime);

	if (cat.grounded) {
		cat.physicsVelocityX *= std::max(0.0f, 1.0f - 7.0f * deltaTime);
		if (std::abs(cat.physicsVelocityX) < IdleSpeedThreshold) {
			cat.physicsVelocityX = 0.0f;
			cat.thrown = false;
		}
	}

	UpdateSpring(cat.impactOffsetX, cat.impactVelocityX, 0.0f, deltaTime);
	cat.impactOffsetX = std::clamp(cat.impactOffsetX, -10.0f, 10.0f);
	UpdateSpring(cat.impactOffsetY, cat.impactVelocityY, 0.0f, deltaTime);
}
