#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace arc
{
    class Log
    {
    public:
        static void Init();
        static std::shared_ptr<spdlog::logger>& GetCoreLogger();

    private:
        static std::shared_ptr<spdlog::logger> s_CoreLogger;
    };
}

#define ARC_CORE_INFO(...)  ::arc::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ARC_CORE_WARN(...)  ::arc::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ARC_CORE_ERROR(...) ::arc::Log::GetCoreLogger()->error(__VA_ARGS__)
