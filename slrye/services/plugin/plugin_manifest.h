#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace slrye
{

struct PluginManifest
{
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string library;
    bool enabled = true;
};

std::optional<PluginManifest> load_plugin_manifest(
    const std::filesystem::path& path,
    std::string* error = nullptr);

}
