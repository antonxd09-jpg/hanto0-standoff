// ASTRA_MULOAD_FONTS_V1
// Font set ported verbatim from the FloatingModMenu project:
// Montserrat (regular / big / medium) + FontAwesome 6 icon font.
#pragma once
#include "imgui.h"

namespace {
#include "MontserratFont.h"
#include "MontserratFontBig.h"
#include "MontserratFontMed.h"
#include "autofont.h"
}

namespace astra::fonts {
inline ImFont* base = nullptr;
inline ImFont* big = nullptr;
inline ImFont* med = nullptr;
inline ImFont* icon = nullptr;
inline ImFont* iconLarge = nullptr;

inline void load(ImGuiIO& io) {
    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 1;
    cfg.OversampleV = 1;
    cfg.PixelSnapH = false;

    base = io.Fonts->AddFontFromMemoryTTF(
        MontserratFont, static_cast<int>(sizeof(MontserratFont)), 30.0f, &cfg);
    if (!base) {
        io.Fonts->AddFontDefault();
        base = io.Fonts->Fonts[0];
    }
    big = io.Fonts->AddFontFromMemoryTTF(
        MontserratFontBig, static_cast<int>(sizeof(MontserratFontBig)), 70.0f, &cfg);
    med = io.Fonts->AddFontFromMemoryTTF(
        MontserratFontMed, static_cast<int>(sizeof(MontserratFontMed)), 40.0f, &cfg);

    ImFontConfig icons{};
    icons.FontDataOwnedByAtlas = false;
    icons.OversampleH = 1;
    icons.OversampleV = 1;
    icons.PixelSnapH = false;
    static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    icon = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        FontAwesome6_compressed_data_base85, 30.0f, &icons, iconRanges);
    // Only the five nav glyphs: keeps the atlas small on mobile GPUs.
    static const ImWchar navRanges[] = {
        0xf013, 0xf013, // gear
        0xf05b, 0xf05b, // crosshairs
        0xf06e, 0xf06e, // eye
        0xf132, 0xf132, // shield
        0xf1ce, 0xf1ce, // circle-notch
        0};
    iconLarge = io.Fonts->AddFontFromMemoryCompressedBase85TTF(
        FontAwesome6_compressed_data_base85, 60.0f, &icons, navRanges);
    if (!icon) icon = base;
    if (!iconLarge) iconLarge = icon;
    if (!big) big = base;
    if (!med) med = base;
}
}
