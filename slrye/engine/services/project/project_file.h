#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace slrye::engine
{

inline constexpr std::uint32_t kProjectFileFormatVersion = 1;
inline constexpr const char* kProjectFileExtension = ".slproj";

struct ProjectFile
{
    std::string name = "Untitled";
    std::filesystem::path start_scene = "scenes/main.slscene";
};

[[nodiscard]] std::optional<ProjectFile> load_project_file(
    const std::filesystem::path& path,
    std::string* error = nullptr);

bool save_project_file(
    const std::filesystem::path& path,
    const ProjectFile& project,
    std::string* error = nullptr);

[[nodiscard]] std::filesystem::path resolve_project_scene_path(
    const std::filesystem::path& project_path,
    const std::filesystem::path& scene_path);

}

namespace slrye
{
using engine::ProjectFile;
using engine::kProjectFileFormatVersion;
using engine::kProjectFileExtension;
using engine::load_project_file;
using engine::save_project_file;
using engine::resolve_project_scene_path;
}
