// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

#include <string>
#include <vector>

// Skyboxes for Ambience > Override Sky.
//
// The list is every sky material the installed game ships under materials/skybox plus the
// compiled ones the user drops into <csgo>/materials/skybox/custom. Each entry gets a
// preview: the sky's cube map is decoded out of its .vtex_c and unwrapped into a strip of
// the horizon on the GPU. Implemented in the cheat side of the bridge; the desktop harness
// only shows Default.
namespace nl::skies {

struct Entry {
    std::string name;     // "Dust 2"
    std::string material; // "materials/skybox/sky_de_dust2.vmat", empty for Default
    bool custom = false;
};

void set_device(void* device, void* context);
void release();

// Entry 0 is always Default. Built once the game archive is open.
const std::vector<Entry>& list();
bool ready();

// Rescans the archive and the custom folder; previews already made are kept.
void refresh();
// Opens the custom folder in Explorer, creating it first.
void open_folder();

// Queues every preview; `pending` counts the ones not finished yet.
void warm();
int pending();

// Draws the preview of entry `index` into [min, max]. False while it is not ready. `fade` as
// in nl::maps::draw.
bool draw(ImDrawList* dl, int index, const ImVec2& min, const ImVec2& max, ImU32 tint = IM_COL32_WHITE,
          float rounding = 0.0f, ImDrawFlags corners = ImDrawFlags_RoundCornersAll, float fade = 0.0f);

// Index of the entry with this material, 0 (Default) when there is none.
int find(const char* material);

} // namespace nl::skies
