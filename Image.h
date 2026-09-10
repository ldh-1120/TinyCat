#pragma once

#include <windows.h>
#include <cstddef>
#include <string>
#include <vector>

struct Image {
	UINT width = 0;
	UINT height = 0;
	std::vector<BYTE> pixels;

	// Images use premultiplied BGRA, as required by UpdateLayeredWindow.
	bool IsValid() const noexcept;
};

bool GetImageByteSize(UINT width, UINT height, std::size_t& byteSize) noexcept;
bool LoadPng(const std::wstring& path, Image& image);
Image CreateSpriteFrame(const Image& sheet, UINT x, UINT y, UINT width, UINT height,
	UINT scale, bool flipX);
Image RotateImageNearest(const Image& source, float degrees, float pivotX, float pivotY);
