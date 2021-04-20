#include "hzpch.h"
#include "Hazel/Core/Input.h"

#ifdef HZ_PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsInput.h"
#endif

namespace Hazel {

	Scope<Input> Input::s_Instance = Input::Initialize();

	Scope<Input> Input::Initialize()
	{
	#ifdef HZ_PLATFORM_WINDOWS
		return CreateScope<WindowsInput>();
	#else
		HZ_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}
} 