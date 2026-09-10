#include "Image.h"
#include "CatConfig.h"

#include <wincodec.h>
#include <wrl/client.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

#pragma comment(lib, "windowscodecs.lib")

bool GetImageByteSize(UINT width, UINT height, std::size_t& byteSize) noexcept {
	byteSize = 0;
	if (width == 0 || height == 0)
		return false;

	constexpr auto maximum = (std::numeric_limits<std::size_t>::max)();
	if (static_cast<std::size_t>(width) > maximum / 4)
		return false;

	const std::size_t stride = static_cast<std::size_t>(width) * 4;
	if (static_cast<std::size_t>(height) > maximum / stride)
		return false;

	byteSize = stride * height;
	return true;
}

bool Image::IsValid() const noexcept {
	std::size_t byteSize = 0;
	return GetImageByteSize(width, height, byteSize) && pixels.size() == byteSize;
}

bool LoadPng(const std::wstring& path, Image& image) {
	using Microsoft::WRL::ComPtr;
	ComPtr<IWICImagingFactory> factory;
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(factory.GetAddressOf()))))
		return false;

	ComPtr<IWICBitmapDecoder> decoder;
	if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
		WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf())))
		return false;

	ComPtr<IWICBitmapFrameDecode> frame;
	if (FAILED(decoder->GetFrame(0, frame.GetAddressOf())))
		return false;

	Image decoded;
	if (FAILED(frame->GetSize(&decoded.width, &decoded.height)))
		return false;

	std::size_t byteSize = 0;
	if (!GetImageByteSize(decoded.width, decoded.height, byteSize) ||
		byteSize > (std::numeric_limits<UINT>::max)())
		return false;

	ComPtr<IWICFormatConverter> converter;
	if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
		FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
		return false;

	try {
		decoded.pixels.resize(byteSize);
	} catch (const std::bad_alloc&) {
		return false;
	} catch (const std::length_error&) {
		return false;
	}

	const UINT stride = decoded.width * 4;
	if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(byteSize),
		decoded.pixels.data())))
		return false;

	image = std::move(decoded);
	return true;
}

Image CreateSpriteFrame(const Image& sheet, UINT x, UINT y, UINT width, UINT height,
	UINT scale, bool flipX) {
	if (!sheet.IsValid() || width == 0 || height == 0 || scale == 0 ||
		x > sheet.width || y > sheet.height || width > sheet.width - x ||
		height > sheet.height - y || width > (std::numeric_limits<UINT>::max)() / scale ||
		height > (std::numeric_limits<UINT>::max)() / scale)
		return { };

	Image result;
	result.width = width * scale;
	result.height = height * scale;
	std::size_t byteSize = 0;
	if (!GetImageByteSize(result.width, result.height, byteSize))
		return { };

	result.pixels.resize(byteSize);
	for (UINT destinationY = 0; destinationY < result.height; ++destinationY) {
		const UINT sourceY = y + destinationY / scale;
		for (UINT destinationX = 0; destinationX < result.width; ++destinationX) {
			const UINT frameX = destinationX / scale;
			const UINT sourceX = x + (flipX ? width - 1 - frameX : frameX);
			const std::size_t sourceIndex = (static_cast<std::size_t>(sourceY) * sheet.width + sourceX) * 4;
			const std::size_t destinationIndex =
				(static_cast<std::size_t>(destinationY) * result.width + destinationX) * 4;
			std::memcpy(result.pixels.data() + destinationIndex, sheet.pixels.data() + sourceIndex, 4);
		}
	}

	return result;
}

Image RotateImageNearest(const Image& source, float degrees, float pivotX, float pivotY) {
	if (!source.IsValid() || !std::isfinite(degrees) || !std::isfinite(pivotX) ||
		!std::isfinite(pivotY))
		return { };

	Image result;
	result.width = CatConfig::RenderWidth;
	result.height = CatConfig::RenderHeight;

	std::size_t byteSize = 0;
	if (!GetImageByteSize(result.width, result.height, byteSize))
		return { };

	result.pixels.resize(byteSize, 0);

	constexpr float Pi = 3.14159265358979323846f;
	const float radians = std::fmod(degrees, 360.0f) * Pi / 180.0f;
	const float cosine = std::cos(-radians);
	const float sine = std::sin(-radians);
	const float destinationPivotX = pivotX + static_cast<float>(CatConfig::RenderPadding);
	const float destinationPivotY = pivotY + static_cast<float>(CatConfig::RenderPadding);

	for (UINT y = 0; y < result.height; ++y) {
		for (UINT x = 0; x < result.width; ++x) {
			const float relativeX = static_cast<float>(x) - destinationPivotX;
			const float relativeY = static_cast<float>(y) - destinationPivotY;
			const float nearestX = std::round(relativeX * cosine - relativeY * sine + pivotX);
			const float nearestY = std::round(relativeX * sine + relativeY * cosine + pivotY);
			// Compare before converting to an integer, including non-finite intermediates.
			if (!(nearestX >= 0.0f && nearestX < static_cast<double>(source.width) &&
				nearestY >= 0.0f && nearestY < static_cast<double>(source.height)))
				continue;

			const std::size_t sourceIndex = (static_cast<std::size_t>(nearestY) * source.width + static_cast<std::size_t>(nearestX)) * 4;
			const std::size_t destinationIndex = (static_cast<std::size_t>(y) * result.width + x) * 4;
			std::memcpy(result.pixels.data() + destinationIndex, source.pixels.data() + sourceIndex, 4);
		}
	}

	return result;
}
