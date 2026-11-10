// Menu source watermark: @use_sberbank
#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "imgui.h"

namespace nl {
struct Config;
}

// Hotkeys: right-clicking any bindable row opens its bind menu. A bind applies a value to one
// setting of the current preset while its key is active (toggled, held or always), and restores
// the previous value when it stops.
namespace nl::binds {

enum class Kind { toggle, slider, combo, multi };
enum class Mode { toggle, hold, always };

// The setting a row edits. `ptr` points into the Config passed to set_context().
struct Target {
    const char* label = nullptr;
    void* ptr = nullptr;
    Kind kind = Kind::toggle;
    float lo = 0.0f, hi = 1.0f, step = 1.0f;
    const char* format = nullptr;
    std::span<const char* const> items = {}; // combo and multi
};

struct Bind {
    std::string name;
    std::size_t offset = 0; // of the setting inside Config
    Kind kind = Kind::toggle;
    float lo = 0.0f, hi = 1.0f, step = 1.0f;
    const char* format = nullptr;
    std::span<const char* const> items = {};

    ImGuiKey key = ImGuiKey_None;
    Mode mode = Mode::toggle;
    // Applied while active: 0/1 for toggles, the value of sliders, the index of combos and a
    // bit mask of the selected items of multi-selects.
    float value = 1.0f;
    bool visible = true; // listed in the Hotkeys window

    bool toggled = false; // state of Mode::toggle
    bool applied = false;
    float saved = 0.0f; // value of the setting before the bind applied
    float shown = 0.0f; // presence in the Hotkeys window, animated 0..1
};

using List = std::vector<Bind>;

// The menu calls this every frame before drawing: the config the rows edit, its defaults (for
// Reset) and the binds of that config.
void set_context(Config* config, const Config* defaults, List* list);

// Menu-owned settings that live outside the preset (the profile window and the watermark
// options) are bindable as well; `base`/`size` describe the block that holds them. Without
// it, right-clicking one of those rows never opens the bind menu.
void set_external_state(void* base, std::size_t size);

// Adds a bind of `target` to the current list; null when the target is outside the config.
Bind* add(const Target& target);

// Handles right-click on [min, max]: opens the bind menu of `target`. Call inside the row's ID
// scope, in the window that draws the row.
void row(const Target& target, const ImVec2& min, const ImVec2& max);
// The setting has at least one bind (rows show a keyboard icon after the label).
bool bound(const void* ptr);
// Draws the keyboard icon after a label that ends at `x`.
void bound_icon(ImDrawList* dl, float x, float cy, ImU32 col);

// Key handling. Call once per frame after ImGui::NewFrame() and before the menu.
void update();
// Restores every applied setting, e.g. before switching presets.
void release_all();

// Key name as shown in the menu ("M4", "Shift", "F").
const char* key_name(ImGuiKey key);
const char* mode_name(Mode mode);

// The "Hotkeys" overlay listing the active binds. Without active binds it only shows (empty)
// while the menu is open, which also lets the window be dragged. Binds slide in and out.
void draw_hotkeys(bool show, bool menu_open);

} // namespace nl::binds
