// KAZAKH_GUI_FONTS_V1
#pragma once
#include "imgui.h"

namespace kazakh::fonts {

// Raw TTF blobs, kept in their own namespace so they cannot collide with
// anything else in the single translation unit.
#include "fonts/Verdana.h"
#include "fonts/PixelFont.h"

inline ImFont* verdana = nullptr;
inline ImFont* pixel = nullptr;
inline ImFont* smallpixel = nullptr;
inline ImFont* fivepixel = nullptr;

inline void load(ImGuiIO& io) {
    io.Fonts->ClearFonts();

    ImFontConfig cfg;
    cfg.SizePixels = 22.0f;
    cfg.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();
    cfg.FontDataOwnedByAtlas = false;

    ImFontConfig cfg2;
    cfg2.SizePixels = 18.0f;
    cfg2.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();
    cfg2.FontDataOwnedByAtlas = false;

    verdana = io.Fonts->AddFontFromMemoryTTF(Verdana, sizeof Verdana, 21.6f, &cfg);
    pixel = io.Fonts->AddFontFromMemoryTTF(PixelFont, sizeof PixelFont, 21.6f, &cfg2);
    fivepixel = io.Fonts->AddFontFromMemoryTTF(PixelFont, sizeof PixelFont, 18.0f, &cfg2);
    smallpixel = io.Fonts->AddFontFromMemoryTTF(PixelFont, sizeof PixelFont, 16.2f, &cfg2);

    io.FontGlobalScale = 1.5f; // fonts rasterised 20% larger for the 1.8 UI scale
}

} // namespace kazakh::fonts
