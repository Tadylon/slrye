#pragma once

#include <string>
#include <vector>

namespace slrye
{

class ConsolePanel
{
public:
    enum class LogLevel
    {
        Info = 0,
        Warning = 1,
        Error = 2
    };

    struct LogEntry
    {
        std::string message;
        LogLevel level = LogLevel::Info;
    };

    ConsolePanel();

    void add_log(std::string message, LogLevel level = LogLevel::Info);
    void clear() noexcept;
    void render();

    void set_visible(bool visible) noexcept;
    [[nodiscard]] bool is_visible() const noexcept;

private:
    std::vector<LogEntry> entries_;
    bool auto_scroll_ = true;
    bool visible_ = true;
};

}
