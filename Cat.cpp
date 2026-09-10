#include "Cat.h"
#include "CatConfig.h"

using namespace CatConfig;

Cat::Cat() {
	stateDuration = RandomDuration(4.0f, 8.0f);
	GetCursorPos(&previousPlayCursor);
}

float Cat::RandomDuration(float minimum, float maximum) {
	return std::uniform_real_distribution<float>(minimum, maximum)(randomEngine);
}

void Cat::SetState(CatState newState) {
	if (state == newState)
		return;

	state = newState;
	frame = 0;
	animationTimer = 0.0f;
	stateTimer = 0.0f;
	stateDuration = 0.0f;
	moveVelocityX = 0.0f;

	switch (state) {
		case CatState::Idle:
			stateDuration = RandomDuration(4.0f, 8.0f);
			break;

		case CatState::Walk:
			stateDuration = RandomDuration(3.0f, 7.0f);
			facingRight = std::uniform_int_distribution<int>(0, 1)(randomEngine) == 1;
			moveVelocityX = facingRight ? WalkSpeed : -WalkSpeed;
			break;

		case CatState::Edge:
			stateDuration = RandomDuration(0.6f, 1.3f);
			break;

		case CatState::Land:
			stateDuration = LandFrameTime * static_cast<float>(LandFrameCount);
			break;

		case CatState::Fall:
		case CatState::WatchCursor:
		case CatState::ChaseCursor:
			break;
	}
}

void Cat::TurnAwayFromEdge() {
	const bool nextFacingRight = !facingRight;
	SetState(CatState::Walk);

	facingRight = nextFacingRight;
	stateDuration = RandomDuration(2.0f, 5.0f);
	moveVelocityX = facingRight ? WalkSpeed : -WalkSpeed;
}
