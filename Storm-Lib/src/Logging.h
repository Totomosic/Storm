#pragma once
#include <memory>
#include <string>
#include <functional>
#include <cstdint>

#define STORM_API

#if (!defined(EMSCRIPTEN) || !defined(STORM_DIST))
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/ostr.h>

namespace Storm
{

    class STORM_API Logger
    {
    private:
        static bool s_Initialized;
        static std::shared_ptr<spdlog::logger> s_Logger;

    public:
        static void Init();
        static inline spdlog::logger& GetLogger()
        {
            return *s_Logger;
        }
    };

}

#endif

#if defined(STORM_DIST) || defined(EMSCRIPTEN)
#define STORM_TRACE(...)
#define STORM_INFO(...)
#define STORM_WARN(...)
#define STORM_ERROR(...)
#define STORM_FATAL(...)

#define STORM_ASSERT(arg, ...)

#define STORM_DEBUG_ONLY(x)
#else

#define STORM_TRACE(...) ::Storm::Logger::GetLogger().trace(__VA_ARGS__)
#define STORM_INFO(...) ::Storm::Logger::GetLogger().info(__VA_ARGS__)
#define STORM_WARN(...) ::Storm::Logger::GetLogger().warn(__VA_ARGS__)
#define STORM_ERROR(...) ::Storm::Logger::GetLogger().error(__VA_ARGS__)
#define STORM_FATAL(...) ::Storm::Logger::GetLogger().critical(__VA_ARGS__)

#ifdef STORM_PLATFORM_WINDOWS
#define STORM_ASSERT(arg, ...) \
    { \
        if (!(arg)) \
        { \
            STORM_FATAL(__VA_ARGS__); \
            __debugbreak(); \
        } \
    }
#else
#define STORM_ASSERT(arg, ...) \
    { \
        if (!(arg)) \
        { \
            STORM_FATAL(__VA_ARGS__); \
        } \
    }
#endif

#define STORM_DEBUG_ONLY(x) x
#endif
