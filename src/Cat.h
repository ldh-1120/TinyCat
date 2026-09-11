#pragma once

#include <windows.h>
#include <random>

enum class DragPose { Hanging, LeanLeft, LeanRight };
enum class CatState { Idle, Walk, Edge, Fall, Land, WatchCursor, ChaseCursor };

// State shared by behavior, desktop collision, and animation systems.
struct Cat {
	Cat();
	void SetState(CatState newState);
	void TurnAwayFromEdge();

	float x = 300.0f;
	float y = 100.0f;
	float moveVelocityX = 0.0f;
	float physicsVelocityX = 0.0f;
	float velocityY = 0.0f;
	float dragVelocityX = 0.0f;
	float dragVelocityY = 0.0f;
	float rotation = 0.0f;
	float angularVelocity = 0.0f;

	int frame = 0;
	float animationTimer = 0.0f;
	float stateTimer = 0.0f;
	float stateDuration = 0.0f;
	bool facingRight = true;
	bool grounded = false;
	bool dragging = false;
	bool dragFacingRight = true;
	bool thrown = false;
	DragPose dragPose = DragPose::Hanging;
	CatState state = CatState::Idle;

	HWND standingWindow = nullptr;
	RECT standingWindowRect { };
	float supportLostTimer = 0.0f;
	float occlusionTimer = 0.0f;
	float occlusionIgnoreTimer = 0.0f;
	float occludedTime = 0.0f;

	float dragOffsetX = 0.0f;
	float dragOffsetY = 0.0f;
	float targetDragOffsetX = 0.0f;
	float targetDragOffsetY = 0.0f;
	POINT previousCursorPosition { };

	float impactOffsetX = 0.0f;
	float impactVelocityX = 0.0f;
	float impactOffsetY = 0.0f;
	float impactVelocityY = 0.0f;

	POINT previousPlayCursor { };
	float cursorVelocityX = 0.0f;
	float cursorVelocityY = 0.0f;
	float cursorInterestTimer = 0.0f;
	float cursorPlayCooldown = 0.0f;

private:
	float RandomDuration(float minimum, float maximum);
	std::mt19937 randomEngine { std::random_device { }() };
};
