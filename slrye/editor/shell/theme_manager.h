#pragma once

#include <editor/shell/editor_settings.h>

namespace slrye
{

class ThemeManager
{
public:
    void apply(EditorTheme theme);

    [[nodiscard]] static const char* display_name(EditorTheme theme) noexcept;
};

}
