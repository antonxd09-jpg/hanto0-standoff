// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// Map previews for the Ambience MAP SELECTION list.
//
// The pictures are the ones the game itself shows while a map loads: compiled panorama
// textures inside pak01 under panorama/images/map_icons/screenshots, each one wrapping
// the source PNG. They are read through the same VPK reader the chams use, so the
// previews always match the installed build and no assets have to ship with the menu.
// Implemented in the cheat side of the bridge; the desktop harness reports nothing.
namespace nl::maps {

// True once the game archive is open.
bool available();

// Starts decoding a map's picture in the background without drawing it.
void request(const char* map_id);

// Pictures requested but not decoded yet.
int pending();

// Texture for a map id ("de_mirage"), or 0 while it is still decoding or when the game
// has no icon for it. The caller must not release it.
ImTextureID texture(const char* map_id);

// Size of the picture behind `texture`, in pixels.
bool texture_size(const char* map_id, int* width, int* height);

// Draws the preview cover-fitted into [min, max] (cropped through the UVs, nothing
// spills outside the box). Returns false when there is nothing to draw, so callers can
// fall back to a flat fill. `fade` is the share of the width over which the picture fades in
// from its left edge.
bool draw(ImDrawList* dl, const char* map_id, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE,
          float rounding = 0.0f, ImDrawFlags corners = ImDrawFlags_RoundCornersAll, float fade = 0.0f);

// Fades the vertices [first, end) of `dl` in from transparent at `left` to their own alpha at
// `right`. Shared by the map and sky pictures.
inline void fade_in_x(ImDrawList* dl, int first, float left, float right)
{
    if (right <= left)
        return;
    for (int i = first; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        float t = (v.pos.x - left) / (right - left);
        t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
        const float a = static_cast<float>((v.col >> IM_COL32_A_SHIFT) & 0xFF) * t * t * (3.0f - 2.0f * t);
        v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5f) << IM_COL32_A_SHIFT);
    }
}

} // namespace nl::maps
