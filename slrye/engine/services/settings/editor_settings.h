#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

// #include <editor/shell/editor_settings.h>

namespace slrye
{

inline constexpr std::uint32_t kEditorSettingsFormatVersion = 1;

[[nodiscard]] std::optional<EditorSettings> load_editor_settings(
    const std::filesystem::path& path,
    std::string* error = nullptr);

bool save_editor_settings(
    const std::filesystem::path& path,
    const EditorSettings& settings,
    std::string* error = nullptr);

}
