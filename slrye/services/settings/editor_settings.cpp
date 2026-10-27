#include <services/settings/editor_settings.h>

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

glm::vec3 read_vec3(const YAML::Node& node, const char* field)
{
    if (!node || !node.IsSequence() || node.size() != 3)
    {
        throw std::runtime_error(std::string("Editor settings field '") + field + "' must contain three numbers");
    }
    return glm::vec3(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
}

YAML::Node write_vec3(const glm::vec3& value)
{
    YAML::Node node(YAML::NodeType::Sequence);
    node.push_back(value.x);
    node.push_back(value.y);
    node.push_back(value.z);
    return node;
}

float read_number(const YAML::Node& node, const char* field, float fallback)
{
    if (!node)
    {
        return fallback;
    }
    if (!node.IsScalar())
    {
        throw std::runtime_error(std::string("Editor settings field '") + field + "' must be a number");
    }
    return node.as<float>();
}

}

std::string_view editor_theme_name(EditorTheme theme) noexcept
{
    switch (theme)
    {
    case EditorTheme::SpectrumLight:
        return "SpectrumLight";
    case EditorTheme::SpectrumDark:
        return "SpectrumDark";
    case EditorTheme::EnemyMouse:
        return "EnemyMouse";
    }
    return {};
}

std::optional<EditorTheme> parse_editor_theme(std::string_view name) noexcept
{
    if (name == "SpectrumLight")
    {
        return EditorTheme::SpectrumLight;
    }
    if (name == "SpectrumDark")
    {
        return EditorTheme::SpectrumDark;
    }
    if (name == "EnemyMouse")
    {
        return EditorTheme::EnemyMouse;
    }
    return std::nullopt;
}

std::optional<EditorSettings> load_editor_settings(
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
            throw std::runtime_error("Editor settings must contain a YAML mapping");
        }

        const YAML::Node version_node = root["version"];
        if (!version_node || !version_node.IsScalar())
        {
            throw std::runtime_error("Editor settings require a format version");
        }

        const std::uint32_t version = version_node.as<std::uint32_t>();
        if (version != kEditorSettingsFormatVersion)
        {
            throw std::runtime_error(
                "Unsupported editor settings version " + std::to_string(version) +
                " (expected " + std::to_string(kEditorSettingsFormatVersion) + ")");
        }

        const YAML::Node theme_node = root["theme"];
        if (!theme_node || !theme_node.IsScalar())
        {
            throw std::runtime_error("Editor settings require a theme");
        }
        const std::optional<EditorTheme> theme = parse_editor_theme(theme_node.as<std::string>());
        if (!theme.has_value())
        {
            throw std::runtime_error("Editor settings contain an unknown theme");
        }

        EditorSettings settings;
        settings.theme = *theme;

        const YAML::Node viewport = root["viewport"];
        if (viewport && !viewport.IsMap())
        {
            throw std::runtime_error("Editor settings field 'viewport' must be a mapping");
        }
        if (viewport)
        {
            const YAML::Node camera_position = viewport["camera_position"];
            if (camera_position)
            {
                settings.camera_position = read_vec3(camera_position, "camera_position");
            }
            settings.camera_yaw_degrees = read_number(
                viewport["yaw_degrees"], "yaw_degrees", settings.camera_yaw_degrees);
            settings.camera_pitch_degrees = read_number(
                viewport["pitch_degrees"], "pitch_degrees", settings.camera_pitch_degrees);
        }

        const YAML::Node recent_projects = root["recent_projects"];
        if (recent_projects)
        {
            if (!recent_projects.IsSequence())
            {
                throw std::runtime_error("Editor settings field 'recent_projects' must be a sequence");
            }
            for (const YAML::Node& entry : recent_projects)
            {
                if (!entry.IsScalar())
                {
                    throw std::runtime_error("Recent project entries must be paths");
                }
                std::filesystem::path project_path =
                    platform::path_from_utf8(entry.as<std::string>());
                if (!project_path.empty())
                {
                    settings.recent_projects.push_back(std::move(project_path));
                }
            }
        }

        return settings;
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to load editor settings: " + std::string(exception.what()));
        return std::nullopt;
    }
}

bool save_editor_settings(
    const std::filesystem::path& path,
    const EditorSettings& settings,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    try
    {
        YAML::Node root;
        root["version"] = kEditorSettingsFormatVersion;
        root["theme"] = std::string(editor_theme_name(settings.theme));

        YAML::Node viewport;
        viewport["camera_position"] = write_vec3(settings.camera_position);
        viewport["yaw_degrees"] = settings.camera_yaw_degrees;
        viewport["pitch_degrees"] = settings.camera_pitch_degrees;
        root["viewport"] = viewport;

        YAML::Node recent_projects(YAML::NodeType::Sequence);
        for (const std::filesystem::path& project_path : settings.recent_projects)
        {
            recent_projects.push_back(platform::path_to_utf8(project_path));
        }
        root["recent_projects"] = recent_projects;

        std::string contents = YAML::Dump(root);
        contents.push_back('\n');
        return platform::write_text_file_atomic(path, contents, error);
    }
    catch (const std::exception& exception)
    {
        set_error(error, "Unable to save editor settings: " + std::string(exception.what()));
        return false;
    }
}

}
