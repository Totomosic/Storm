workspace "Storm"
    architecture "x64"

    configurations
    {
        "Dist",
        "Debug",
        "Release",
    }

    flags
    {
        "MultiProcessorCompile"
    }

    filter "system:linux"
        configurations
        {
            "DistShared",
            "ReleaseShared",
        }

include ("Paths.lua")

include (StormLibDir)
include (StormCliDir)
include (StormTestDir)
include (StormBookDir)
include ("Storm-Emscripten")

if os.target() == "windows" then
    -- Windows
    if os.isfile("SwigConfigWindows.lua") then
        include ("SwigConfigWindows.lua")
        include ("Storm-Swig")
    end
else
    -- Linux
    if os.isfile("SwigConfigLinux.lua") then
        include ("SwigConfigLinux.lua")
        include ("Storm-Swig")
    end
end
