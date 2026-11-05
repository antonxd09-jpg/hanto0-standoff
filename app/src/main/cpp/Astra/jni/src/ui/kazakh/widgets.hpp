// KAZAKH_GUI_WIDGETS_V1
#pragma once
#include <string>
#include <vector>
#include "imgui.h"

namespace kazakh {

inline const char* toasttext = nullptr;
inline float toasttimer = 0.0f;
inline float toastmaxtime = 0.0f;

inline void Toast(const char* text, int length = 1) {
    toasttext = text;
    toasttimer = static_cast<float>(length);
    toastmaxtime = static_cast<float>(length);
}

inline void TextCentered(const std::string& text) {
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth = ImGui::CalcTextSize(text.c_str()).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextUnformatted(text.c_str());
}

inline void RenderToast() {
    ImGuiIO& io = ImGui::GetIO();
    if (toasttimer <= 0.0f || !toasttext) return;
    toasttimer -= io.DeltaTime;
    float alpha = 1.0f;
    if (toasttimer >= 0.1f && toasttimer <= toastmaxtime - 0.1f) alpha = 1.0f;
    if (toasttimer < 0.1f) alpha = toasttimer * 10.0f;
    if (toasttimer > toastmaxtime - 0.1f) alpha = (toastmaxtime - toasttimer) * 10.0f;

    const auto textSize = ImGui::CalcTextSize(toasttext);
    ImGui::GetForegroundDrawList()->AddRectFilled(
        ImVec2(io.DisplaySize.x / 2 - textSize.x / 2 - 20,
               io.DisplaySize.y * 0.8f - textSize.y / 2 - 20 - alpha * 5),
        ImVec2(io.DisplaySize.x / 2 + textSize.x / 2 + 20,
               io.DisplaySize.y * 0.8f + textSize.y / 2 + 20 - alpha * 5),
        ImColor(0.1f, 0.1f, 0.1f, alpha), 5.0f);

    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(io.DisplaySize.x / 2 - textSize.x / 2,
               io.DisplaySize.y * 0.8f - textSize.y / 2 - alpha * 5),
        ImColor(1.0f, 1.0f, 1.0f, alpha), toasttext);
}

inline void MultiCombo(const char* label, bool combos[], const char* items[], int items_count) {
    std::vector<std::string> vec;
    static std::string preview;
    for (int i = 0, j = 0; i < items_count; i++) {
        if (combos[i]) {
            vec.push_back(items[i]);
            if (j > 2)
                preview = vec.at(0) + ", " + vec.at(1) + ", " + vec.at(2) + ", ...";
            else if (j)
                preview += ", " + std::string(items[i]);
            else
                preview = items[i];
            j++;
        }
    }
    if (ImGui::BeginCombo(label, preview.c_str())) {
        for (int i = 0; i < items_count; i++)
            ImGui::Selectable(items[i], &combos[i], ImGuiSelectableFlags_DontClosePopups);
        ImGui::EndCombo();
    }
    preview = "None";
}

inline void AddColorPicker(const char* name, ImVec4& color, bool prd = false,
                           bool* rainbow = nullptr, bool* pulse = nullptr, bool* dark = nullptr) {
    ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_AlphaPreview;
    static ImVec4 backup_color;
    bool open_popup = ImGui::ColorButton((std::string(name) + std::string("##3b")).c_str(), color, misc_flags);
    if (open_popup) {
        ImGui::OpenPopup(name);
        backup_color = color;
    }
    if (ImGui::BeginPopup(name)) {
        ImGui::Spacing();
        ImGui::TextUnformatted(name);
        ImGui::Separator();
        ImGui::ColorPicker4("##picker", (float*)&color,
                            misc_flags | ImGuiColorEditFlags_NoSidePreview |
                            ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted("Current");
        ImGui::ColorButton("##current", color,
                           ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf,
                           ImVec2(60, 40));
        ImGui::TextUnformatted("Previous");
        if (ImGui::ColorButton("##previous", backup_color,
                               ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf,
                               ImVec2(60, 40)))
            color = backup_color;
        ImGui::EndGroup();
        if (prd) {
            if (rainbow) ImGui::Checkbox("rainbow", rainbow);
            if (pulse) ImGui::Checkbox("pulse", pulse);
            if (dark) ImGui::Checkbox("dark", dark);
        }
        ImGui::Spacing();
        ImGui::EndPopup();
    }
}

inline void ColorPicker(const char* name, ImVec4& color) {
    static ImVec4 backup_color;
    bool open_popup = ImGui::ColorButton((std::string(name) + std::string("##3b")).c_str(), color);
    if (open_popup) {
        ImGui::OpenPopup(name);
        backup_color = color;
    }
    if (ImGui::BeginPopup(name)) {
        ImGui::Spacing();
        ImGui::TextUnformatted("Select Color");
        ImGui::Separator();
        ImGui::ColorPicker4("##picker", (float*)&color,
                            ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted("Current");
        ImGui::ColorButton("##current", color,
                           ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf,
                           ImVec2(60, 40));
        ImGui::TextUnformatted("Previous");
        if (ImGui::ColorButton("##previous", backup_color,
                               ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf,
                               ImVec2(60, 40)))
            color = backup_color;
        ImGui::EndGroup();
        ImGui::EndPopup();
    }
}

} // namespace kazakh
