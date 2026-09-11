#pragma once

#include <windows.h>

struct Image;

class PetRenderer {
public:
	PetRenderer() = default;
	~PetRenderer();
	PetRenderer(const PetRenderer&) = delete;
	PetRenderer& operator=(const PetRenderer&) = delete;

	bool Initialize(UINT width, UINT height);
	bool Render(HWND hwnd, const Image& image, int x, int y);

private:
	void Reset() noexcept;

	HDC screenDeviceContext = nullptr;
	HDC memoryDeviceContext = nullptr;

	HBITMAP bitmap = nullptr;
	HGDIOBJ oldBitmap = nullptr;

	void* bitmapBits = nullptr;

	UINT width = 0;
	UINT height = 0;
};
