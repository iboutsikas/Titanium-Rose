workspace "Hazel"
	architecture "x64"
	targetdir "build"

	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}
	
	startproject "RoseGarden"
	
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
IncludeDir["spdlog"]	= "Hazel/vendor/spdlog/include"
IncludeDir["assimp"]	= "Hazel/vendor/assimp/include"
IncludeDir["stbi"] 		= "Hazel/vendor/stb_image"
IncludeDir["cxxopts"] 	= "Hazel/vendor/cxxopts/include"
IncludeDir["json"] 		= "Hazel/vendor/json/single_include"

-- Windows Only
IncludeDir["winpix"] 	= "Hazel/vendor/winpixeventruntime/Include"
IncludeDir["d3d12a"]	= "Hazel/vendor/d3d12/include"

group "Dependancies"
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
		"%{IncludeDir.stbi}",
		"%{IncludeDir.cxxopts}",
		"%{IncludeDir.json}"
	}

	links 
	{ 
		"GLFW",
		-- "Glad",
		"ImGui",
		-- "opengl32.lib",
		
	}

	filter "system:windows"
		systemversion "latest"
		libdirs { 
			"Hazel/vendor/winpixeventruntime/bin/x64",
			"Hazel/vendor/D3D12/bin"
		}
		links {  
			"WinPixEventRuntime.lib",
			-- "D3DCompiler",
			"dxcompiler.lib",
			"dxguid",
			"d3d12",
			"dxgi"
		}
		includedirs {
			"%{IncludeDir.winpix}",
			"%{IncludeDir.d3d12a}"
		}
		defines {
			"HZ_BUILD_DLL",
			"GLFW_INCLUDE_NONE",
			"USE_PIX"
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

project "RoseGarden"
	location "RoseGarden"
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
		"%{IncludeDir.GLFW}",
		"%{prj.name}/src",
		"Hazel/src",
		"Hazel/vendor",
		"%{IncludeDir.glm}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.stbi}",
		"%{IncludeDir.json}"
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
		libdirs { "Hazel/vendor/winpixeventruntime/bin/x64" }
		links {  "WinPixEventRuntime.lib" }
		includedirs {
			"%{IncludeDir.winpix}"
		}

		postbuildcommands {
			'{COPY} "../Hazel/vendor/winpixeventruntime/bin/x64/WinPixEventRuntime.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/d3d12/bin/D3D12Core.dll" "%{cfg.targetdir}/D3D12/"',
			'{COPY} "../Hazel/vendor/d3d12/bin/D3D12SDKLayers.dll" "%{cfg.targetdir}/D3D12/"',
			'{COPY} "../Hazel/vendor/d3d12/bin/dxcompiler.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/d3d12/bin/dxil.dll" "%{cfg.targetdir}"'
		}

		defines {
			"PROFILE_BUILD"
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
