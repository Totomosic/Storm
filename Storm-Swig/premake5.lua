project "Storm-Swig"
    location ""
    kind "SharedLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "on"

    targetdir ("../bin/" .. StormOutputDir .. "/Storm-Swig")
    objdir ("../bin-int/" .. StormOutputDir .. "/Storm-Swig")
    targetname ("_Storm")

    configmap
    {
        ["Debug"] = "Release"
    }

    prebuildcommands
    {
        "\"" .. PYTHON_EXECUTABLE .. "\" generate_swig.py --swig \"" .. SWIG_EXECUTABLE .. "\" " .. "\"../bin/" .. StormOutputDir .. "/Storm-Swig\""
    }

    files
    {
        "Storm_wrapper.cpp"
    }

    includedirs
    {
        "../%{StormIncludeDirs.spdlog}",
        "../%{StormIncludeDirs.Storm}",
        PYTHON_INCLUDE_DIR,
    }

    links
    {
        "Storm-Lib",
        PYTHON_LIB_FILE,
    }

    filter "system:windows"
        systemversion "latest"

        targetextension (".pyd")

        defines
        {
            "STORM_PLATFORM_WINDOWS",
            "STORM_BUILD_STATIC",
            "_CRT_SECURE_NO_WARNINGS",
            "NOMINMAX"
        }

    filter "system:linux"
        systemversion "latest"

        targetextension (".so")
        targetprefix ("")

        removeconfigurations { "Release", "Dist" }

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

    filter "configurations:DistShared"
        defines "STORM_DIST"
        runtime "Release"
        optimize "on"