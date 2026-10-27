#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace slrye
{

inline constexpr std::size_t kDefaultRecentProjectLimit = 10;

class RecentProjects
{
public:
    explicit RecentProjects(std::size_t max_entries = kDefaultRecentProjectLimit);

    [[nodiscard]] bool add(const std::filesystem::path& path);
    void replace(const std::vector<std::filesystem::path>& paths);
    bool remove(const std::filesystem::path& path);
    void clear() noexcept;

    void set_max_entries(std::size_t max_entries);
    [[nodiscard]] std::size_t max_entries() const noexcept;
    [[nodiscard]] const std::vector<std::filesystem::path>& entries() const noexcept;

private:
    std::size_t max_entries_ = kDefaultRecentProjectLimit;
    std::vector<std::filesystem::path> entries_;
};

}
