#include <services/project/recent_projects.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <system_error>

#include <platform/path_encoding.h>

namespace slrye
{
namespace
{

std::filesystem::path normalize_path(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return {};
    }

    std::filesystem::path normalized = path.lexically_normal();
    if (normalized.is_relative())
    {
        std::error_code error;
        const std::filesystem::path absolute = std::filesystem::absolute(normalized, error);
        if (!error)
        {
            normalized = absolute.lexically_normal();
        }
    }
    return normalized;
}

std::string path_key(const std::filesystem::path& path)
{
    std::string key = platform::path_to_utf8(path);
    std::replace(key.begin(), key.end(), '\\', '/');
#ifdef _WIN32
    std::transform(
        key.begin(),
        key.end(),
        key.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
#endif
    return key;
}

}

RecentProjects::RecentProjects(std::size_t max_entries)
    : max_entries_(std::max<std::size_t>(max_entries, 1))
{
}

bool RecentProjects::add(const std::filesystem::path& path)
{
    const std::filesystem::path normalized = normalize_path(path);
    if (normalized.empty())
    {
        return false;
    }

    const std::string key = path_key(normalized);
    const auto existing = std::find_if(
        entries_.begin(),
        entries_.end(),
        [&key](const std::filesystem::path& entry)
        {
            return path_key(entry) == key;
        });

    if (existing != entries_.end())
    {
        if (existing == entries_.begin())
        {
            return false;
        }
        std::filesystem::path value = *existing;
        entries_.erase(existing);
        entries_.insert(entries_.begin(), std::move(value));
        return true;
    }

    entries_.insert(entries_.begin(), normalized);
    if (entries_.size() > max_entries_)
    {
        entries_.resize(max_entries_);
    }
    return true;
}

void RecentProjects::replace(const std::vector<std::filesystem::path>& paths)
{
    entries_.clear();
    for (const std::filesystem::path& path : paths)
    {
        add(path);
    }
}

bool RecentProjects::remove(const std::filesystem::path& path)
{
    const std::filesystem::path normalized = normalize_path(path);
    if (normalized.empty())
    {
        return false;
    }

    const std::string key = path_key(normalized);
    const auto new_end = std::remove_if(
        entries_.begin(),
        entries_.end(),
        [&key](const std::filesystem::path& entry)
        {
            return path_key(entry) == key;
        });
    if (new_end == entries_.end())
    {
        return false;
    }

    entries_.erase(new_end, entries_.end());
    return true;
}

void RecentProjects::clear() noexcept
{
    entries_.clear();
}

void RecentProjects::set_max_entries(std::size_t max_entries)
{
    max_entries_ = std::max<std::size_t>(max_entries, 1);
    if (entries_.size() > max_entries_)
    {
        entries_.resize(max_entries_);
    }
}

std::size_t RecentProjects::max_entries() const noexcept
{
    return max_entries_;
}

const std::vector<std::filesystem::path>& RecentProjects::entries() const noexcept
{
    return entries_;
}

}
