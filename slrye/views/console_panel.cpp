#include <views/console_panel.h>

#include <utility>
#include <imgui.h>

namespace slrye
{

ConsolePanel::ConsolePanel()
{
    add_log("Slrye Engine Editor initialized.", LogLevel::Info);
}

void ConsolePanel::add_log(std::string message, LogLevel level)
{
    entries_.push_back({std::move(message), level});
}

void ConsolePanel::clear() noexcept
{
    entries_.clear();
}

void ConsolePanel::set_visible(bool visible) noexcept
{
    visible_ = visible;
}

bool ConsolePanel::is_visible() const noexcept
{
    return visible_;
}

void ConsolePanel::render()
{
    if (!visible_)
    {
        return;
    }

    if (!ImGui::Begin("Console", &visible_))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear"))
    {
        clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);

    ImGui::Separator();

    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& entry : entries_)
    {
        ImVec4 color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        if (entry.level == LogLevel::Warning)
        {
            color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
        }
        else if (entry.level == LogLevel::Error)
        {
            color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(entry.message.c_str());
        ImGui::PopStyleColor();
    }

    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();
}

}
