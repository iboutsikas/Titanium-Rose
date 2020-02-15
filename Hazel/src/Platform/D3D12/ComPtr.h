#pragma once

#ifdef HZ_PLATFORM_WINDOWS
#include <wrl.h>
namespace Hazel {
	template<typename T>
	using TComPtr = Microsoft::WRL::ComPtr<T>;
}
#endif
