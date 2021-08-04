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
