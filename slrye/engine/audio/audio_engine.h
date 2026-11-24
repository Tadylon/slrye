#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <glm/vec3.hpp>

#include <engine/audio/sound.h>

namespace slrye::engine
{

class Scene;

/**
 * @brief 音频系统主控制器（Audio Engine）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Audio 模块，负责底层音频硬件设备（WASAPI / CoreAudio / ALSA）的初始化与全生命周期管理；
 * 2. 内部通过 PImpl 封装 miniaudio 的 ma_engine 句柄；
 * 3. 负责创建和加载声音文件（Sound 工厂）；
 * 4. 负责管理 3D 空间主听音者（Audio Listener）的世界位置与朝向；
 * 5. 与 ECS 深度集成：提供 update(Scene& scene)，根据 Scene 中的 AudioListenerComponent
 *    与 AudioSourceComponent 实时同步 3D 空间位置与播放状态。
 */
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
    AudioEngine(AudioEngine&&) = delete;
    AudioEngine& operator=(AudioEngine&&) = delete;

    [[nodiscard]] static AudioEngine& instance();

    bool initialize(std::string* error = nullptr);
    void shutdown();
    [[nodiscard]] bool is_initialized() const noexcept;

    /// @brief 主音量控制 [0.0f, 1.0f+]
    void set_master_volume(float volume) noexcept;
    [[nodiscard]] float master_volume() const noexcept;

    /// @brief 设置主听音者（Listener）的空间位置与朝向（通常来自摄像机）
    void set_listener_position(const glm::vec3& position) noexcept;
    void set_listener_orientation(const glm::vec3& forward, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) noexcept;

    /// @brief 从文件创建并载入一个声音对象
    [[nodiscard]] std::shared_ptr<Sound> load_sound(
        const std::filesystem::path& path,
        bool is_3d = true,
        std::string* error = nullptr);

    /// @brief 快速播放一段全局 2D 一次性音效（如 UI 提示音）
    bool play_one_shot(const std::filesystem::path& path, float volume = 1.0f);

    /// @brief 根据当前场景的实体与组件驱动 3D 空间音效
    void update(Scene& scene);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    bool initialized_ = false;
};

}

namespace slrye
{
using engine::AudioEngine;
}
