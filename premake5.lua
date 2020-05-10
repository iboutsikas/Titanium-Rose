workspace "Hazel"
	architecture "x86_64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"DebugNoDXL",
		"Release",
		"Dist"
	}
	
	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["GLFW"] = "Hazel/vendor/GLFW/include"
-- IncludeDir["Glad"] = "Hazel/vendor/Glad/include"
IncludeDir["ImGui"] = "Hazel/vendor/imgui"
IncludeDir["glm"] = "Hazel/vendor/glm"
IncludeDir["stb_image"] = "Hazel/vendor/stb_image"
IncludeDir["tinygltf"] = "Hazel/vendor/tinygltf"
IncludeDir["tinyobjloader"] = "Hazel/vendor/tinyobjloader"
IncludeDir["assimp"] = "Hazel/vendor/assimp"
IncludeDir["winpix"] = "Hazel/vendor/winpixeventruntime/Include"

group "Dependencies"
	include "Hazel/vendor/GLFW"
	-- include "Hazel/vendor/Glad"
	include "Hazel/vendor/imgui"

group ""

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
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl"
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
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		-- "%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
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

	filter { "system:windows", "configurations:Debug"}
		libdirs { "Hazel/vendor/assimp/lib/debug" }
		links {	"IrrXMLd", "assimpd", "zlibstaticd" }

	filter { "system:windows", "configurations:Release"}
		libdirs { "Hazel/vendor/assimp/lib/release" }
		links {	"IrrXML", "assimp", "zlibstatic" }

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
	
	filter "configurations:DebugNoDXL"
		defines { "HZ_NO_D3D12_DEBUG_LAYER", "HZ_DEBUG" }
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
		"Hazel/vendor/spdlog/include",
		"Hazel/src",
		"Hazel/vendor",
		"%{IncludeDir.glm}",
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

	filter { "system:windows", "configurations:Debug"}
		libdirs { "Hazel/vendor/assimp/lib/debug" }
		links {	"assimp-vc142-mtd" }

	filter { "system:windows", "configurations:Release"}
		libdirs { "Hazel/vendor/assimp/lib/release" }
		links {	"assimp-vc142-mt" }

	filter "system:windows"
		systemversion "latest"
		-- libdirs { "Hazel/vendor/winpixeventruntime/bin/x64" }
		-- links {  "WinPixEventRuntime.lib" }
		includedirs {
			"%{IncludeDir.winpix}"
		}

	filter "configurations:Debug"
		defines "HZ_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:DebugNoDXL"
		defines { "HZ_NO_D3D12_DEBUG_LAYER", "HZ_DEBUG" }
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
