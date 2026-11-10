// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// Frosted-glass backgrounds for floating windows. The renderer host provides a draw callback that
// copies the frame rendered so far and blurs it into a texture; windows draw that texture under
// their translucent fill, so whatever lies behind them shows through blurred.
namespace nl::blur {

struct Backend {
    // Runs inside the renderer at its place in the draw list: copies the current render target,
    // blurs it into `texture` and leaves the render target bound as it found it. A
    // ImDrawCallback_ResetRenderState command always follows it.
    ImDrawCallback capture = nullptr;
    // The blurred frame, sampled with UV = screen position / display size. It must stay the same
    // object for the whole frame; hosts recreate it only between frames.
    ImTextureID texture = ImTextureID_Invalid;
};

// Without a backend (or with an empty one) windows fall back to an almost opaque fill.
void set_backend(const Backend& backend);
bool available();

// Captures and blurs everything rendered before this point of `dl`.
void capture(ImDrawList* dl);
// Draws the most recent capture of this frame inside the rounded rect.
void image(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImDrawFlags flags = 0);
// capture() followed by image().
void backdrop(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImDrawFlags flags = 0);

} // namespace nl::blur
