#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <services/plugin/plugin_api.h>

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

struct LoadedPlugin
{
    PluginManifest manifest;
    SlryePluginCapabilities capabilities = SLRYE_PLUGIN_CAPABILITY_NONE;
    std::filesystem::path directory;
    std::filesystem::path library_path;
};

class PluginManager
{
public:
    using LogFn = std::function<void(const std::string&)>;

    explicit PluginManager(LogFn log = {});
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    std::size_t load_all(const std::filesystem::path& plugins_root);
    bool load_plugin(const std::filesystem::path& plugin_directory);
    void shutdown();

    [[nodiscard]] const std::vector<LoadedPlugin>& plugins() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}
