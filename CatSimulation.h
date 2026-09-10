#pragma once

#include "Cat.h"
#include "DesktopPlatforms.h"

// Advances behavior and physics. Window messages and rendering stay in Application.
class CatSimulation {
public:
	CatSimulation(Cat& cat, DesktopPlatforms& platforms);
	void Update(float deltaTime, const POINT* cursor);
	void BeginDrag(int localX, int localY, POINT cursor);
	void EndDrag(bool applyThrow = true);

private:
	void UpdateCursorMotion(float deltaTime, POINT cursor);
	void UpdateCursorInteraction(float deltaTime, POINT cursor);
	void FinishCursorInteraction();
	void UpdateBehavior(float deltaTime);
	void UpdateDrag(float deltaTime, POINT cursor);
	void UpdateMovement(float deltaTime);
	void UpdateDynamics(float deltaTime);

	Cat& cat;
	DesktopPlatforms& platforms;
};
