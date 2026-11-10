// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// The inventory's model inspector.
//
// Like the ESP preview, the menu owns no D3D11 device, so the model is rendered offscreen
// inside the cheat core and handed over as a texture. What differs is the camera: this one
// is a turntable framed on the model's bounding box, so a weapon can be turned over and
// looked at from any side, and it can paint a weapon finish over the model.
//
// The declaration is shared; the implementation lives in
// src/cheat/port/model_preview_bridge.cpp and is only linked into the Windows builds.
namespace nl::model_preview {

// Borrowed device/context of the host. Never AddRef'd, so the platform layer must keep
// them alive for as long as the menu draws (release() before the backend shuts down).
void set_device(void* device, void* context);
void release();

// True once a device is known and the offscreen renderer could be created.
bool available();

struct Frame {
    ImTextureID texture{}; // owned by the preview, valid until the next render
    bool ready{};
    float width{};  // size of the offscreen target in pixels
    float height{};
};

// Model to inspect, from nl::models::weapon(). Passing null or an empty string renders
// nothing, which is how the inspector reports "this slot has no 3D preview".
void set_model(const char* model_path);

// Finish to paint over that model, straight from nl::models::Finish. Everything may be
// null, which draws the weapon as it ships. `palette` selects the style-paint treatment,
// where `color` holds region weights for the material's own g_vColor0..3 rather than
// colour. Changing the finish invalidates nothing cached that matters, so it is cheap to
// call every frame with the same values.
void set_finish(const char* material, const char* color, const char* normal,
                const char* roughness, const char* ao, const char* mask, bool palette);
void clear_finish();

// Wear float of the slot (0 = Factory New .. 1 = Battle-Scarred): scratches the finish off
// through the paint's own wear and grunge maps, the way the game does.
void set_wear(float wear);

// Chams over the model (the Model row of a slot's options): `type` is a chams material
// type of the cheat core, negative for none. `exponent` is the menu's 0..100 value.
// `overlay` keeps the model's own textures under the chams instead of replacing them.
void set_chams(int type, const ImVec4& color, float exponent, bool overlay);

// Turntable controls, in mouse pixels (drag) and wheel notches.
void orbit(float dx, float dy);
void zoom(float notches);

// Back to the default three-quarter view, framed on the model again.
void reset_view();

// Renders one frame. Returns a frame with ready == false while the model is still being
// extracted or when nothing is selected.
Frame render(float width, float height);

} // namespace nl::model_preview
