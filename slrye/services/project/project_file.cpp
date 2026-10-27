#include <services/project/project_file.h>

#include <stdexcept>
#include <utility>

#include <yaml-cpp/yaml.h>

#include <platform/file_io.h>
#include <platform/path_encoding.h>

namespace slrye
{
namespace
{

void set_error(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

std::string read_required_string(const YAML::Node& node, const char* key)
{
    const YAML::Node value = node[key];
    if (!value || !value.IsScalar())
    {
        throw std::runtime_error(std::string("Project file requires '") + key + "'");
    }

    std::string result = value.as<std::string>();
    if (result.empty())
    {
        throw std::runtime_error(std::string("Project file field '") + key + "' cannot be empty");
    }
    return result;
}

}

std::optional<ProjectFile> load_project_file(
    const std::filesystem::path& path,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    std::string contents;
    if (!platform::read_text_file(path, contents, error))
    {
        return std::nullopt;
    }

    try
    {
        const YAML::Node root = YAML::Load(contents);
        if (!root || !root.IsMap())
        {
            throw std::runtime_error("Project file must contain a YAML mapping");
        }

        const YAML::Node version_node = root["version"];
        if (!version_node || !version_node.IsScalar())
        {
            throw std::runtime_error("Project file requires a format version");
        }

        const std::uint32_t version = version_node.as<std::uint32_t>();
        if (version != kProjectFileFormatVersion)
        {
            throw std::runtime_error(
                "Unsupported project file version " + std::to_string(version) +
                " (expected " + std::to_string(kProjectFileFormatVersion) + ")");
        }

        ProjectFile project;
        project.name = read_required_string(root, "name");
        project.start_scene = platform::path_from_utf8(read_required_string(root, "start_scene"));
        return project;
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to load project file: " + std::string(exception.what()));
        return std::nullopt;
    }
}

bool save_project_file(
    const std::filesystem::path& path,
    const ProjectFile& project,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    if (project.name.empty() || project.start_scene.empty())
    {
        set_error(error, "Project name and start scene must not be empty");
        return false;
    }

    try
    {
        YAML::Node root;
        root["version"] = kProjectFileFormatVersion;
        root["name"] = project.name;
        root["start_scene"] = platform::path_to_utf8(project.start_scene);

        std::string contents = YAML::Dump(root);
        contents.push_back('\n');
        return platform::write_text_file_atomic(path, contents, error);
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to save project file: " + std::string(exception.what()));
        return false;
    }
}

std::filesystem::path resolve_project_scene_path(
    const std::filesystem::path& project_path,
    const std::filesystem::path& scene_path)
{
    if (scene_path.is_absolute())
    {
        return scene_path.lexically_normal();
    }
    return (project_path.parent_path() / scene_path).lexically_normal();
}

}
