// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// Inventory pictures (see menu/inventory_catalog.h). Images are decoded the first time they are
// drawn, a few milliseconds' worth per frame, and fade in once ready. Each image is kept at the
// power-of-two reduction closest to its on-screen size, so downscaled pictures stay smooth.
namespace nl::images {

// Call once per frame before drawing.
void new_frame();
// Call after the renderer backend has shut down and before ImGui::DestroyContext().
void shutdown();

// Where `image` lands when its canvas is fitted (aspect kept, centered) into [min, max].
// With `crop`, the opaque part of the picture is fitted instead of the whole canvas.
void fit(int image, const ImVec2& min, const ImVec2& max, bool crop, ImVec2* out_min, ImVec2* out_max,
         float max_scale = 0.0f);
// Draws `image` fitted into [min, max]. `tint` alpha multiplies the fade-in.
void draw(ImDrawList* dl, int image, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE,
          bool crop = false, float max_scale = 0.0f);
// Draws the picture's opaque part exactly at [min, max].
void draw_rect(ImDrawList* dl, int image, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE);

} // namespace nl::images
