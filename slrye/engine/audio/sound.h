#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <glm/vec3.hpp>

namespace slrye::engine
{

class AudioEngine;

/**
 * @brief 音频声源实例（Sound Instance）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Audio 模块，代表一个已加载的声音源（音乐/音效）；
 * 2. 内部采用 PImpl 模式封装 miniaudio 的 ma_sound 句柄，隔离底层 C 头文件，保障高编译效率；
 * 3. 支持播放、暂停、停止、循环、音量、音调以及 3D 空间立体声属性（空间坐标、最大最小衰减距离）。
 */
class Sound
{
public:
    Sound();
    ~Sound();

    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
    Sound(Sound&&) noexcept;
    Sound& operator=(Sound&&) noexcept;

    void play();
    void pause();
    void stop();

    [[nodiscard]] bool is_playing() const noexcept;
    [[nodiscard]] bool is_looping() const noexcept;
    void set_looping(bool looping) noexcept;

    [[nodiscard]] float volume() const noexcept;
    void set_volume(float volume) noexcept;

    [[nodiscard]] float pitch() const noexcept;
    void set_pitch(float pitch) noexcept;

    [[nodiscard]] bool is_spatialized() const noexcept;
    void set_spatialized(bool spatialized) noexcept;

    [[nodiscard]] glm::vec3 position() const noexcept;
    void set_position(const glm::vec3& position) noexcept;

    void set_attenuation_distances(float min_distance, float max_distance) noexcept;
    [[nodiscard]] float min_distance() const noexcept;
    [[nodiscard]] float max_distance() const noexcept;

    [[nodiscard]] const std::filesystem::path& path() const noexcept;
    [[nodiscard]] bool is_valid() const noexcept;

private:
    friend class AudioEngine;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}

namespace slrye
{
using engine::Sound;
}
