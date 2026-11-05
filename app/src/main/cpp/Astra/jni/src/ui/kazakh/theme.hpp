// KAZAKH_GUI_THEME_V1
// Styles, colors and scaling copied from the KazakhDLC menu.
#pragma once
#include "imgui.h"
#include "imgui_internal.h"

namespace kazakh {

inline constexpr float kDpi = 1.8f;
inline ImVec4 accentColor = ImColor(255, 225, 255);

inline float lerp(float a, float b, float f) {
    return a + f * (b - a);
}

template <class T>
inline T clampv(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline void styles() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    style.ScrollbarRounding = 0;
    style.WindowMinSize = ImVec2(480, 295);

    colors[ImGuiCol_WindowBg] = ImColor(16, 16, 16);
    colors[ImGuiCol_ChildBg] = ImColor(24, 24, 24);
    colors[ImGuiCol_Text] = ImColor(255, 255, 255);
    colors[ImGuiCol_CheckMark] = ImColor(255, 255, 255);

    colors[ImGuiCol_Header] = ImColor(30, 30, 30);
    colors[ImGuiCol_HeaderActive] = ImColor(28, 28, 28);
    colors[ImGuiCol_HeaderHovered] = ImColor(28, 28, 28);

    colors[ImGuiCol_Button] = ImColor(31, 31, 31);
    colors[ImGuiCol_ButtonActive] = ImColor(34, 34, 34);
    colors[ImGuiCol_ButtonHovered] = ImColor(34, 34, 34);

    colors[ImGuiCol_Border] = ImColor(0, 0, 0, 255);

    colors[ImGuiCol_FrameBg] = ImVec4(0.231f, 0.231f, 0.229f, 0.541f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.231f, 0.231f, 0.229f, 0.400f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.232f, 0.232f, 0.232f, 0.670f);

    style.ScaleAllSizes(2.0f);
}

inline void stylesGrape() {
    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_ChildBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.5f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 0.5f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 0.5f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.53f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.81f, 0.83f, 0.81f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = accentColor;
    style->Colors[ImGuiCol_SliderGrab] = accentColor;
    style->Colors[ImGuiCol_SliderGrabActive] = accentColor;
    style->Colors[ImGuiCol_Button] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_Separator] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_SeparatorHovered] = accentColor;
    style->Colors[ImGuiCol_SeparatorActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = accentColor;
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_Tab] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    style->Colors[ImGuiCol_TabHovered] = accentColor;
    style->Colors[ImGuiCol_TabActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style->Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    style->Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style->Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style->Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0, 0, 0, 0);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0, 0, 0, 0);

    style->FrameBorderSize = kDpi / 1.50f;
    style->ChildBorderSize = 0;
    style->WindowBorderSize = 0;
    style->FramePadding = ImVec2(2.0f * kDpi / 1.50f, 2.0f * kDpi / 1.50f);
    style->ItemSpacing = ImVec2(2.0f * kDpi / 1.50f, 2.0f * kDpi / 1.50f);
    style->IndentSpacing = 0;
    style->ScrollbarSize = 10 * kDpi / 1.50f;
    style->WindowRounding = 0;
    style->FrameRounding = 2 * kDpi / 1.50f;
    style->PopupRounding = 0;
    style->ScrollbarRounding = 1 * kDpi / 1.50f;
    style->GrabRounding = 2 * kDpi / 1.50f;
    style->TabRounding = 0;
    style->WindowTitleAlign = ImVec2(0, 0);
    style->DisplaySafeAreaPadding = ImVec2(0, 0);
    style->WindowRounding = 0;
    style->Alpha = 1;
    style->WindowPadding = ImVec2(0, 0);

    style->ScaleAllSizes(3);
}

inline void applyStyle() {
    stylesGrape();
}

inline void SetBlackGoldTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.ButtonTextAlign = ImVec2(0.5, 0.65);
    style.WindowTitleAlign = ImVec2(0.5, 0.5);
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 0.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0);
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
}

inline void ozTheme() {
    ImGui::StyleColorsDark();
    auto& Style = ImGui::GetStyle();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.11f, 0.18f, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.41f, 0.16f, 0.47f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.27f, 0.13f, 0.40f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.45f, 0.31f, 0.59f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.44f, 0.19f, 0.56f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.12f, 0.28f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.22f, 0.57f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.18f, 0.69f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.34f, 0.19f, 0.43f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.26f, 0.09f, 0.42f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.07f, 0.24f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.17f, 0.06f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.10f, 0.12f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.22f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.63f, 0.00f, 0.93f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.72f, 0.46f, 0.92f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.21f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.13f, 0.46f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.17f, 0.97f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.49f, 0.17f, 0.62f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.64f, 0.26f, 0.68f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.79f, 0.36f, 1.00f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.10f, 0.40f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.80f, 0.26f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.39f, 0.08f, 0.62f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.46f, 0.25f, 0.54f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.65f, 0.21f, 0.82f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.31f, 0.15f, 0.62f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.66f, 0.22f, 0.76f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.87f, 0.40f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.12f, 0.05f, 0.12f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.47f, 0.17f, 0.66f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.43f, 0.18f, 0.62f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.17f, 0.06f, 0.39f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.05f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.31f, 0.10f, 0.49f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.22f, 0.08f, 0.40f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.16f, 0.06f, 0.40f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.20f, 0.14f, 0.46f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.52f, 0.21f, 0.72f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.58f, 0.19f, 0.76f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.41f, 0.08f, 0.64f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.18f, 0.04f, 0.62f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    Style.WindowPadding = ImVec2(20.000f, 11.000f);
    Style.WindowRounding = 12.000f;
    Style.ChildRounding = 5.000f;
    Style.PopupRounding = 6.000f;
    Style.FramePadding = ImVec2(10.000f, 5.000f);
    Style.FrameRounding = 8.000f;
    Style.FrameBorderSize = 1.000f;
    Style.ItemInnerSpacing = ImVec2(16.000f, 4.000f);
    Style.CellPadding = ImVec2(9.000f, 11.000f);
    Style.ScrollbarRounding = 12.000f;
    Style.GrabRounding = 7.000f;
    Style.LogSliderDeadzone = 6.000f;
    Style.TabRounding = 8.000f;
    Style.TabBorderSize = 1.000f;
    Style.ButtonTextAlign = ImVec2(0.5, 0.65);
}

} // namespace kazakh
