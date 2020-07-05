workspace "Hazel"
	architecture "x64"
	targetdir "build"

	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	
	startproject "Sandbox"
	
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] 		= "Hazel/vendor/GLFW/include"
IncludeDir["ImGui"] 	= "Hazel/vendor/imgui"
IncludeDir["glm"] 		= "Hazel/vendor/glm"
IncludeDir["assimp"] 	= "Hazel/vendor/assimp"
IncludeDir["winpix"] 	= "Hazel/vendor/winpixeventruntime/Include"
IncludeDir["spdlog"]	= "Hazel/vendor/spdlog/include"
IncludeDir["assimp"]	= "Hazel/vendor/assimp/include"
-- IncludeDir["Glad"] = "Hazel/vendor/Glad/include"


include "Hazel/vendor/GLFW"
-- include "Hazel/vendor/Glad"
include "Hazel/vendor/imgui"

-- group ""

project "Hazel"
	location "Hazel"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "hzpch.h"
	pchsource "Hazel/src/hzpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		-- "%{prj.name}/vendor/glm/glm/**.hpp",
		-- "%{prj.name}/vendor/glm/glm/**.inl"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_ENABLE_EXPERIMENTAL"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor",
		"%{IncludeDir.GLFW}",
		-- "%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.assimp}",

	}

	links 
	{ 
		"GLFW",
		-- "Glad",
		"ImGui",
		-- "opengl32.lib",
		"D3DCompiler",
		"dxguid",
		"d3d12",
		"dxgi"
	}

	filter "system:windows"
		systemversion "latest"
		libdirs { "Hazel/vendor/winpixeventruntime/bin/x64" }
		links {  "WinPixEventRuntime.lib" }
		includedirs {
			"%{IncludeDir.winpix}"
		}
		defines {
			"HZ_BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		defines "HZ_DEBUG"
		runtime "Debug"
		symbols "on"
	
	filter "configurations:Release"
		defines "HZ_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "HZ_DIST"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"Hazel/src",
		"Hazel/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.assimp}"
	}

	defines
	{
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_ENABLE_EXPERIMENTAL"
	}

	links
	{
		"Hazel"
	}

	filter "system:windows"
		systemversion "latest"
		-- libdirs { "Hazel/vendor/winpixeventruntime/bin/x64" }
		-- links {  "WinPixEventRuntime.lib" }
		includedirs {
			"%{IncludeDir.winpix}"
		}

		postbuildcommands {
			'{COPY} "../Hazel/vendor/winpixeventruntime/bin/x64/WinPixEventRuntime.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Debug"
		defines "HZ_DEBUG"
		runtime "Debug"
		symbols "on"

		links { "Hazel/vendor/assimp/bin/Debug/assimpd.lib" }

		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/Debug/assimpd.dll" "%{cfg.targetdir}"'
		}
		
	filter "configurations:Release"
		defines "HZ_RELEASE"
		runtime "Release"
		optimize "on"

		links { "Hazel/vendor/assimp/bin/Release/assimp.lib" }

		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/Release/assimp.dll" "%{cfg.targetdir}"'
		}

	filter "configurations:Dist"
		defines "HZ_DIST"
		runtime "Release"
		optimize "on"
