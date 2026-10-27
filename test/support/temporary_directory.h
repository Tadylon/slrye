#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        static std::atomic<unsigned long long> sequence = 0;
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("slrye_test_" + std::to_string(timestamp) + "_" + std::to_string(sequence.fetch_add(1)));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept
    {
        return path_;
    }

    std::filesystem::path write_file(std::string_view name, std::string_view contents) const
    {
        const std::filesystem::path file_path = path_ / std::filesystem::path(std::string(name));
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream stream(file_path, std::ios::binary);
        stream << contents;
        return file_path;
    }

private:
    std::filesystem::path path_;
};
