StormOutputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Project Directories
StormLibDir = "Storm-Lib/"
StormCliDir = "Storm-Cli/"
StormTestDir = "Storm-Test/"
StormBookDir = "Storm-Book/"

-- Include directories relative to solution directory
StormIncludeDirs = {}
StormIncludeDirs["spdlog"] =     StormLibDir .. "vendor/spdlog/include/"
StormIncludeDirs["Storm"] =      StormLibDir .. "src/"
StormIncludeDirs["Catch"] =      StormTestDir .. "vendor/Catch2/"

-- Library directories relative to solution directory
LibraryDirs = {}

-- Links
Links = {}