#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <queue>

#include "TitaniumRose/Core/Log.h"

#include "TitaniumRose/Debug/Instrumentor.h"

#ifdef HZ_PLATFORM_WINDOWS
	#include <Windows.h>
	//#define USE_PIX
	//#include <WinPixEventRuntime/pix3.h>
	//#include <Commdlg.h>
#ifndef GLFW_EXPOSE_NATIVE_WIN32
	#include <GLFW/glfw3.h>
	#define GLFW_EXPOSE_NATIVE_WIN32
	#include <GLFW/glfw3native.h>
#endif // !GLFW_EXPOSE_NATIVE_WIN32
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)