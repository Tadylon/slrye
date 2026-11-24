#pragma once

#include <engine/audio/sound.h>
#include <miniaudio.h>

namespace slrye::engine
{

struct Sound::Impl
{
    ma_sound sound{};
    std::filesystem::path path;
    bool is_initialized = false;
    float min_dist = 1.0f;
    float max_dist = 50.0f;
};

}
