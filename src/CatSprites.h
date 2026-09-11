#pragma once

#include "Image.h"

struct Cat;

class CatSprites {
public:
	bool Load(const std::wstring& directory, std::wstring& error);
	const Image& CurrentFrame(const Cat& cat) const;
	bool HitTest(const Cat& cat, int x, int y) const;
	void UpdateAnimation(Cat& cat, float deltaTime) const;

private:
	struct Frames {
		std::vector<Image> right;
		std::vector<Image> left;
	};

	static bool LoadFrames(const std::wstring& path, UINT frameCount, Frames& frames, std::wstring& error);
	static const Image& SelectFrame(const Frames& frames, bool facingRight, int index);

	Frames walk;
	Frames idle;
	Frames drag;
	Frames fall;
	Frames land;
};
