// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// The enemy model of the Visuals -> Players preview.
//
// The menu owns no D3D11 device and no cheat state, so the model is rendered
// offscreen by the chams preview inside the cheat core and handed over as a
// texture. Everything here is a thin, allocation-free facade; the implementation
// lives in src/cheat/port/esp_model_bridge.cpp and is only linked into the
// Windows builds.
namespace nl::esp_model {

// Borrowed device/context of the host. Never AddRef'd, so the platform layer must
// keep them alive for as long as the menu draws (release() before the backend
// shuts down).
void set_device(void* device, void* context);
void release();

// True once a device is known and the offscreen renderer could be created.
bool available();

struct frame {
    ImTextureID texture{}; // owned by the preview, valid until the next render
    bool ready{};
    float width{};  // size of the offscreen target in pixels
    float height{};
};

// Chooses which model the preview renders (an agent path from the inventory catalogue).
// Passing null or an empty string returns to the default agent.
void set_model(const char* model_path);

// Renders one frame of the model. `orbit` is a yaw change in degrees (mouse drag
// inside the preview), `use_chams` applies the live Player (visible) material.
// Returns a frame with ready == false while the model is still being extracted.
frame render(float width, float height, float orbit, bool use_chams);

// Skeleton of the last rendered frame, in normalized [0,1] image coordinates, so
// the caller can map it onto whatever rectangle it drew the texture into.
int skeleton_segments();
bool skeleton_segment(int index, float* x0, float* y0, float* x1, float* y1);

// True once a render succeeded; the overlays then follow the real bones instead of
// the built-in placeholder shapes.
bool frame_ready();

// Name of the map the player is on ("de_mirage"), or an empty string when the game
// has not published one yet. Used by the Ambience map list.
const char* current_map_name();

// Bounds of a body part of the last rendered frame, normalized the same way.
enum class part : int { head = 0, torso, left_leg, right_leg, count };
bool part_bounds(part which, float* x0, float* y0, float* x1, float* y1);

} // namespace nl::esp_model
