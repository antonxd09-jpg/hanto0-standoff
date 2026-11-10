// Menu source watermark: @use_sberbank
#pragma once

#include <span>

#include "imgui.h"

// Custom widgets drawn in the Neverlose style. Main-menu widgets are placed at absolute
// screen positions (the menu layout is fixed); popup rows use the regular ImGui cursor.
namespace nl::ui {

// Exponential approach to `target`, stored per id in the current window.
// The first call for an id returns `target`, so nothing animates on startup.
float animate(ImGuiID id, float target, float speed = 16.0f);

struct ItemState {
    bool hovered = false;
    bool held = false;
    bool pressed = false;
};
ItemState item_behavior(ImGuiID id, const ImVec2& min, const ImVec2& max, bool enabled = true);
// Appends a line about a duplicate ID to %TEMP%\vesta\imgui_ids.log (Windows only).
void debug_log_id_conflict(const char* window, unsigned id);

// Reserves an ID for this frame inside the current window. False when the same ID was
// already submitted here, which is what ImGui reports as a conflicting-ID error.
bool claim_item_id(ImGuiID id);

// Sizes of the fixed-size controls at the current UI scale, in whole pixels.
// `scale` shrinks the toggle (the bind editor uses a smaller one).
ImVec2 toggle_size(float scale = 1.0f);
ImVec2 combo_size();

// `pos` is the top-left corner; it is snapped to whole pixels.
bool toggle(const char* str_id, bool* v, const ImVec2& pos, bool enabled = true, float scale = 1.0f);
bool dots_button(const char* str_id, const ImVec2& center, bool enabled = true);
bool color_swatch(const char* str_id, ImVec4* color, const ImVec2& min, float size, bool enabled = true);
// The swatch alone: a click opens popup `id`, whose contents the caller draws.
void swatch_face(ImGuiID id, const ImVec4* color, const ImVec2& min, const ImVec2& max, bool enabled = true);
bool combo_box(const char* str_id, const ImVec2& min, const ImVec2& max, std::span<const char* const> items,
               int* current, bool enabled = true);
// Shows the selected items joined with commas, or `placeholder` when nothing is selected.
bool multi_combo_box(const char* str_id, const ImVec2& min, const ImVec2& max, std::span<const char* const> items,
                     bool* selected, const char* placeholder = "None", bool enabled = true);
// Slider of the main panels: the track starts at `x0`, the value box ends at `right`, both
// centered on `cy`. Values snap to multiples of `step`.
bool slider(const char* str_id, float x0, float right, float cy, float* v, float lo, float hi, const char* format,
            float step = 1.0f, bool enabled = true);
// Slider of settings windows: a bar without knob; the value shows in a tooltip while hovered.
bool bar_slider(const char* str_id, const ImVec2& min, const ImVec2& max, float* v, float lo, float hi,
                const char* format, float step = 1.0f, bool enabled = true);
// Small label next to the mouse cursor ("200%").
void value_tooltip(const char* text);
// Frameless single-line text input over [min, max], vertically centered, with a dim
// `placeholder` while empty. `focus` grabs the keyboard this frame. Returns true when edited,
// or when Enter is pressed if `flags` contains ImGuiInputTextFlags_EnterReturnsTrue.
bool text_field(const char* str_id, char* buf, int size, const ImVec2& min, const ImVec2& max,
                const char* placeholder, bool focus = false, ImU32 text_col = 0, ImGuiInputTextFlags flags = 0);

// Popup window styled like the menu, opened below the anchor rect (or above it near the bottom
// of the display). Call end_popup() only when begin_popup() returned true.
bool begin_popup(const char* str_id, const ImVec2& anchor_min, const ImVec2& anchor_max, float width);
bool begin_popup_id(ImGuiID id, const ImVec2& anchor_min, const ImVec2& anchor_max, float width);
// Popup with its top-left corner at `pos` (kept inside the display) and the given padding.
// A negative `rounding` uses the default of dropdown lists.
bool begin_popup_at(ImGuiID id, const ImVec2& pos, float width, const ImVec2& padding, float rounding = -1.0f);
void end_popup();

// Frosted glass: the blurred frame behind the rect under a translucent `fill`, with an optional
// soft shadow around it.
void glass(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImU32 fill, bool shadow,
           ImDrawFlags flags = 0);
// Background of floating windows: glass with a shadow and a faint outline.
void window_frame(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding);

// One line of a dropdown list inside a popup. Returns true when clicked.
bool dropdown_item(const char* label, bool selected, bool checkbox);
// Scrollable list of dropdown lines (at most six visible, wheel and scrollbar). `checked` turns
// it into a checklist. Returns the index clicked this frame, or -1.
int dropdown_list(std::span<const char* const> items, int current, const bool* checked = nullptr);
// How a picker looks. The default is the swatch popup: "Primary"/"Secondary" tag, alpha bar
// and alpha box, 8-digit hex.
struct PickerStyle {
    const char* title = nullptr; // tag text for the edited color instead of "Primary"
    int colors = 2;              // colors the tag cycles through (1..3)
    bool alpha = true;           // alpha bar, alpha box and the alpha digits of the hex
    bool saved_row = false;      // the saved colors as a row under the value row
};

// The color picker drawn inline in the current window (the contents of a swatch's popup).
bool color_picker_embed(const char* str_id, ImVec4* color);
bool color_picker_embed(const char* str_id, ImVec4* color, const PickerStyle& style);

// Settings windows: a title and rounded groups of rows, opened next to the mouse.
//
//   if (ui::popup::begin("##bomb", "Bomb")) {
//       ui::popup::group([&] { ui::popup::toggle("Enabled", &v); });
//       ui::popup::end();
//   }
namespace popup {

// Opens the window `str_id` next to the mouse cursor.
void open(const char* str_id);
// Moves the window `str_id` (top-left corner); call before begin(), e.g. every frame.
void place(const char* str_id, const ImVec2& pos);
// Height of the window in the last frame it was shown, 0 before that.
float last_height(const char* str_id);
// A null `title` leaves the title line out.
bool begin(const char* str_id, const char* title, float width = 0.0f);
void end();

void begin_group();
void end_group();
template <class F>
void group(F&& rows)
{
    begin_group();
    rows();
    end_group();
}

// Rows. `icon` is a glyph from nl::icon drawn in front of the label. Disabled rows are dimmed
// and don't react to input (used for options that depend on a toggle above them).
bool toggle(const char* label, bool* v, const char* icon = nullptr, bool enabled = true);
bool color_toggle(const char* label, ImVec4* color, bool* v, const char* icon = nullptr, bool enabled = true);
// `extra_icon` is drawn in front of the swatch (the gear of the Style row).
bool color(const char* label, ImVec4* color, bool enabled = true, const char* icon = nullptr,
           const char* extra_icon = nullptr);
bool slider(const char* label, float* v, float lo, float hi, const char* format, float step = 1.0f, bool enabled = true,
            const char* icon = nullptr);
bool combo(const char* label, int* current, std::span<const char* const> items, bool enabled = true);
bool multi_combo(const char* label, std::span<const char* const> items, bool* selected, bool enabled = true);
// Label with the current choice and a chevron on the right; opens a list of `items`.
bool value(const char* label, int* current, std::span<const char* const> items, bool enabled = true,
           const char* icon = nullptr);
// A label that acts as a button (About, Chat).
bool item(const char* label, const char* icon = nullptr);
// Checklist line: a check mark in front of the label when set.
bool check(const char* label, bool* v, bool enabled = true);
// Label with an editable dark box on the right (name tags).
bool input(const char* label, char* buf, int size, const char* placeholder = nullptr, bool enabled = true);
// Same box holding an integer clamped to [lo, hi].
bool number(const char* label, int* v, int lo, int hi, bool enabled = true);
// Full-width wear scale: the condition bands with a marker over the value, dragged to set it.
bool wear_bar(const char* str_id, float* wear);
// Chams material of a model: a swatch (its picker also holds Overlay and Exponent) in
// front of the chosen material, which opens the list like value().
bool material(const char* label, ImVec4* color, int* current, std::span<const char* const> items, bool* overlay,
              float* exponent, bool enabled = true);
// Flow of selectable tags ("Icon", "Text", "Distance"...).
bool chips(std::span<const char* const> items, bool* selected);

// A toggle row with a "•••" button that opens a nested settings window.
bool toggle_row(const char* label, bool* v, const char* icon, bool enabled, bool with_dots, bool* dots_pressed);
bool collecting();
void collect_push(const char* label);
void collect_pop();
template <class Settings>
bool toggle(const char* label, bool* v, Settings&& settings, const char* icon = nullptr, bool enabled = true)
{
    if (collecting()) {
        toggle_row(label, v, icon, enabled, true, nullptr);
        collect_push(label);
        settings();
        collect_pop();
        return false;
    }
    ImGui::PushID(label);
    bool dots = false;
    const bool changed = toggle_row(label, v, icon, enabled, true, &dots);
    if (dots)
        open("##settings");
    if (begin("##settings", label)) {
        settings();
        end();
    }
    ImGui::PopID();
    return changed;
}

} // namespace popup

} // namespace nl::ui
