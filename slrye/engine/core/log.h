#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace slrye
{

class Log
{
public:
    static void init();

    [[nodiscard]] static std::shared_ptr<spdlog::logger>& core_logger() noexcept
    {
        return core_logger_;
    }

    [[nodiscard]] static std::shared_ptr<spdlog::logger>& client_logger() noexcept
    {
        return client_logger_;
    }

private:
    static std::shared_ptr<spdlog::logger> core_logger_;
    static std::shared_ptr<spdlog::logger> client_logger_;
};

#define SLRYE_CORE_TRACE(...)    ::slrye::Log::core_logger()->trace(__VA_ARGS__)
#define SLRYE_CORE_INFO(...)     ::slrye::Log::core_logger()->info(__VA_ARGS__)
#define SLRYE_CORE_WARN(...)     ::slrye::Log::core_logger()->warn(__VA_ARGS__)
#define SLRYE_CORE_ERROR(...)    ::slrye::Log::core_logger()->error(__VA_ARGS__)

#define SLRYE_TRACE(...)         ::slrye::Log::client_logger()->trace(__VA_ARGS__)
#define SLRYE_INFO(...)          ::slrye::Log::client_logger()->info(__VA_ARGS__)
#define SLRYE_WARN(...)          ::slrye::Log::client_logger()->warn(__VA_ARGS__)
#define SLRYE_ERROR(...)         ::slrye::Log::client_logger()->error(__VA_ARGS__)

}
