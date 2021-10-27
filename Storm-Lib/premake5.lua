project "Storm-Lib"
    location ""
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("../bin/" .. StormOutputDir .. "/Storm-Lib")
    objdir ("../bin-int/" .. StormOutputDir .. "/Storm-Lib")

    files
    {
        "src/**.h",
        "src/**.cpp"
    }
    
    includedirs
    {
        "../%{StormIncludeDirs.spdlog}",
        "src"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "STORM_PLATFORM_WINDOWS",
            "STORM_BUILD_STATIC",
            "_CRT_SECURE_NO_WARNINGS",
            "NOMINMAX"
        }

    filter "system:linux"
        systemversion "latest"

        defines
        {
            "STORM_PLATFORM_LINUX",
            "STORM_BUILD_STATIC"
        }

        links
        {
            "pthread"
        }

    filter "system:macosx"
        systemversion "latest"

        defines
        {
            "STORM_PLATFORM_MAC",
            "STORM_BUILD_STATIC"
        }

    filter "configurations:Debug"
        defines "STORM_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "STORM_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "STORM_DIST"
        runtime "Release"
        optimize "on"

    filter "configurations:ReleaseShared"
        defines "STORM_RELEASE"
        runtime "Release"
        optimize "on"

        buildoptions { "-fPIC" }

    filter "configurations:DistShared"
        defines "STORM_DIST"
        runtime "Release"
        optimize "on"

        buildoptions { "-fPIC" }
