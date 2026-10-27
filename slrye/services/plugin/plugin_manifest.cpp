#include <services/plugin/plugin_manifest.h>

#include <utility>

#include <yaml-cpp/yaml.h>

namespace slrye
{
namespace
{

std::string read_yaml_string(const YAML::Node& node, const char* key)
{
    const YAML::Node value = node[key];
    if (!value)
    {
        return {};
    }

    return value.as<std::string>();
}

void set_error(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

}

std::optional<PluginManifest> load_plugin_manifest(
    const std::filesystem::path& path,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    try
    {
        const YAML::Node root = YAML::LoadFile(path.string());
        if (!root || !root.IsMap())
        {
            set_error(error, "Plugin manifest must contain a YAML mapping");
            return std::nullopt;
        }

        PluginManifest manifest;
        manifest.id = read_yaml_string(root, "id");
        manifest.name = read_yaml_string(root, "name");
        manifest.version = read_yaml_string(root, "version");
        manifest.author = read_yaml_string(root, "author");
        manifest.description = read_yaml_string(root, "description");
        manifest.library = read_yaml_string(root, "library");

        const YAML::Node enabled = root["enabled"];
        if (enabled)
        {
            manifest.enabled = enabled.as<bool>();
        }

        if (manifest.id.empty())
        {
            set_error(error, "Plugin manifest requires a non-empty id");
            return std::nullopt;
        }

        if (manifest.library.empty())
        {
            manifest.library = manifest.id;
        }

        return manifest;
    }
    catch (const YAML::Exception& exception)
    {
        set_error(error, exception.what());
        return std::nullopt;
    }
}

}
