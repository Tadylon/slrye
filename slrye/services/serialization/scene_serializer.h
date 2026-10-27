#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <domain/scene.h>

namespace slrye
{

inline constexpr std::uint32_t kSceneFormatVersion = 1;
inline constexpr const char* kSceneFileExtension = ".slscene";

class SceneSerializer
{
public:
    static bool save(
        const std::filesystem::path& path,
        const Scene& scene,
        std::string* error = nullptr);

    [[nodiscard]] static std::unique_ptr<Scene> load(
        const std::filesystem::path& path,
        std::string* error = nullptr);
};

}
