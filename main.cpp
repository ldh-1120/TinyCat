#include "Application.h"

#include <objbase.h>
#include <exception>

#pragma comment(lib, "ole32.lib")

namespace {
	class ComApartment {
	public:
		ComApartment() : result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
		~ComApartment() { if (SUCCEEDED(result)) CoUninitialize(); }
		bool IsInitialized() const { return SUCCEEDED(result); }
		ComApartment(const ComApartment&) = delete;
		ComApartment& operator=(const ComApartment&) = delete;
	private:
		HRESULT result;
	};
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
	ComApartment apartment;
	if (!apartment.IsInitialized()) {
		MessageBoxW(nullptr, L"COM을 초기화하지 못했습니다.", L"Desktop Cat", MB_OK | MB_ICONERROR);
		return 1;
	}

	try {
		Application application(instance);
		std::wstring error;
		if (!application.Initialize(error)) {
			MessageBoxW(nullptr, error.c_str(), L"Desktop Cat", MB_OK | MB_ICONERROR);
			return 1;
		}

		return application.Run();
	} catch (const std::exception&) {
		MessageBoxW(nullptr, L"프로그램 실행 중 오류가 발생했습니다.", L"Desktop Cat", MB_OK | MB_ICONERROR);
		return 1;
	}
}
