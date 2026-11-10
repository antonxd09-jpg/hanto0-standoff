// Menu source watermark: @use_sberbank
#pragma once

#include <span>
#include <string>
#include <vector>

#include "imgui.h"

// Menu search. While collecting, panels and settings-window rows don't draw: they register
// themselves (label, the setting they edit, where they live) so the search overlay can list
// matching rows with working controls.
namespace nl::search {

enum class Kind { toggle, color_toggle, color, slider, combo, multi };

struct Entry {
    int path = 0; // index into paths()
    const char* label = nullptr;
    Kind kind = Kind::toggle;
    bool* b = nullptr;     // toggle, color_toggle; selected items of multi
    ImVec4* c = nullptr;   // color, color_toggle
    float* f = nullptr;    // slider
    int* i = nullptr;      // combo
    float lo = 0.0f, hi = 1.0f, step = 1.0f;
    const char* format = nullptr;
    std::span<const char* const> items = {};
    bool enabled = true;
};

struct Path {
    std::string text; // "AIMBOT > RAGE > MAIN > SILENT AIM"
    int tab = 0, subtab = 0;
};

bool collecting();
// Starts a new collection; segments pushed afterwards become the path of the entries.
void begin();
void end();
// Location of the rows that follow: the menu tab they open.
void set_tab(int tab, int subtab);
void push(const char* segment);
void pop();
void add(Entry e);

const std::vector<Entry>& entries();
const std::vector<Path>& paths();

// Case-insensitive substring test.
bool matches(const char* text, const char* query);

// Folds a query typed on a Cyrillic (ЙЦУКЕН) layout back onto the Latin characters
// the same physical keys produce, so "фшmь" still finds "AIMBOT". ASCII input is
// returned unchanged; `out` must hold `size` bytes.
void fold_query(const char* query, char* out, size_t size);

} // namespace nl::search
