#include "PetRenderer.h"
#include "Image.h"

#include <cstring>
#include <limits>

PetRenderer::~PetRenderer() {
	Reset();
}

void PetRenderer::Reset() noexcept {
	if (memoryDeviceContext && oldBitmap)
		SelectObject(memoryDeviceContext, oldBitmap);

	if (bitmap)
		DeleteObject(bitmap);

	if (memoryDeviceContext)
		DeleteDC(memoryDeviceContext);

	if (screenDeviceContext)
		ReleaseDC(nullptr, screenDeviceContext);

	screenDeviceContext = nullptr;
	memoryDeviceContext = nullptr;
	bitmap = nullptr;
	oldBitmap = nullptr;
	bitmapBits = nullptr;
	width = 0;
	height = 0;
}

bool PetRenderer::Initialize(UINT newWidth, UINT newHeight) {
	Reset();
	std::size_t byteSize = 0;
	if (newWidth > static_cast<UINT>((std::numeric_limits<LONG>::max)()) ||
		newHeight > static_cast<UINT>((std::numeric_limits<LONG>::max)()) ||
		!GetImageByteSize(newWidth, newHeight, byteSize) ||
		byteSize > (std::numeric_limits<DWORD>::max)())
		return false;

	screenDeviceContext = GetDC(nullptr);
	if (screenDeviceContext)
		memoryDeviceContext = CreateCompatibleDC(screenDeviceContext);

	if (!memoryDeviceContext) {
		Reset();
		return false;
	}

	BITMAPINFO bitmapInfo { };
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = static_cast<LONG>(newWidth);
	bitmapInfo.bmiHeader.biHeight = -static_cast<LONG>(newHeight);
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	bitmapInfo.bmiHeader.biSizeImage = static_cast<DWORD>(byteSize);

	bitmap = CreateDIBSection(screenDeviceContext, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits, nullptr, 0);
	if (!bitmap || !bitmapBits) {
		Reset();
		return false;
	}

	oldBitmap = SelectObject(memoryDeviceContext, bitmap);
	if (!oldBitmap || oldBitmap == HGDI_ERROR) {
		oldBitmap = nullptr;
		Reset();
		return false;
	}

	width = newWidth;
	height = newHeight;

	return true;
}

bool PetRenderer::Render(HWND hwnd, const Image& image, int x, int y) {
	if (!hwnd || !screenDeviceContext || !memoryDeviceContext || !bitmapBits ||
		image.width != width || image.height != height || !image.IsValid())
		return false;

	std::memcpy(bitmapBits, image.pixels.data(), image.pixels.size());

	POINT windowPosition { x, y };
	SIZE windowSize { static_cast<LONG>(width), static_cast<LONG>(height) };
	POINT sourcePosition { };
	BLENDFUNCTION blendFunction { };
	blendFunction.BlendOp = AC_SRC_OVER;
	blendFunction.SourceConstantAlpha = 255;
	blendFunction.AlphaFormat = AC_SRC_ALPHA;

	return UpdateLayeredWindow(hwnd, screenDeviceContext, &windowPosition, &windowSize, memoryDeviceContext, &sourcePosition, 0, &blendFunction, ULW_ALPHA) == TRUE;
}
