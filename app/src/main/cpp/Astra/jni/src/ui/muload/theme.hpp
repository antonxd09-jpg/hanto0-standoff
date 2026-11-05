// ASTRA_MULOAD_THEME_V1
// BlackStyle + rainbow header ported from the FloatingModMenu menu.
#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace astra::gui {

// Muload's ImDrawRectRainbow, one-pixel columns so the gradient is exact.
inline void rainbowLine(ImDrawList* d, float x, float y, float width, float height,
                        float& phase, float speed) {
    if (!d || width <= 0.0f) return;
    phase += speed;
    if (phase > 1.0f) phase -= std::floor(phase);
    const int columns = static_cast<int>(width);
    for (int i = 0; i < columns; ++i) {
        float hue = (1.0f / width) * static_cast<float>(i);
        hue -= phase;
        if (hue < 0.0f) hue += 1.0f;
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(
            ImColor::HSV(hue, 1.0f, 1.0f).Value);
        d->AddRectFilled(ImVec2(x + i, y), ImVec2(x + i + 1, y + height), color);
    }
}

inline void applyMuloadTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowMinSize = ImVec2(1, 1);
    style.ScrollbarSize = 14.0f;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowBorderSize = 3.5f;
    style.WindowRounding = 6.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(16.0f / 255.0f, 16.0f / 255.0f, 16.0f / 255.0f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
    c[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
    c[ImGuiCol_Border] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.260f, 0.619f, 0.400f);
    c[ImGuiCol_FrameBg] = ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(28.0f / 255.0f, 28.0f / 255.0f, 28.0f / 255.0f, 1.00f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.000f, 0.260f, 0.619f, 0.700f);
    c[ImGuiCol_Button] = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(40.0f / 255.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    c[ImGuiCol_Separator] = c[ImGuiCol_Border];
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    c[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    c[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    c[ImGuiCol_Tab] = ImLerp(c[ImGuiCol_Header], c[ImGuiCol_TitleBgActive], 0.80f);
    c[ImGuiCol_TabHovered] = c[ImGuiCol_HeaderHovered];
    c[ImGuiCol_TabActive] = ImLerp(c[ImGuiCol_HeaderActive], c[ImGuiCol_TitleBgActive], 0.60f);
    c[ImGuiCol_TabUnfocused] = ImLerp(c[ImGuiCol_Tab], c[ImGuiCol_TitleBg], 0.80f);
    c[ImGuiCol_TabUnfocusedActive] = ImLerp(c[ImGuiCol_TabActive], c[ImGuiCol_TitleBg], 0.40f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    c[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    c[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    c[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    c[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    c[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    c[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    c[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);

    // Muload runs a 3 * 1.1 scale on its reference device. Scale from the
    // actual display height so the same proportions survive on other phones.
    // Muload calls ScaleAllSizes(3.0f * 1.1f) once, independent of resolution.
    style.ScaleAllSizes(3.3f);
}

} // namespace astra::gui
