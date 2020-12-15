project "Storm-Test"
    location ""
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"
    
    targetdir ("../bin/" .. StormOutputDir .. "/Storm-Test")
    objdir ("../bin-int/" .. StormOutputDir .. "/Storm-Test")
    
    files
    {
        "src/**.h",
        "src/**.cpp",
        "vendor/Catch2/catch_amalgamated.hpp",
        "vendor/Catch2/catch_amalgamated.cpp"
    }
    
    includedirs
    {
        "src",
        "../%{StormIncludeDirs.spdlog}",
        "../%{StormIncludeDirs.Storm}",
        "../%{StormIncludeDirs.Catch}"
    }

    links
    {
        "Storm-Lib"
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

        removeconfigurations { "DistShared", "ReleaseShared" }

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