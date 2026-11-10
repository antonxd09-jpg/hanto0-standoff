// Menu source watermark: @use_sberbank
#pragma once

#include <span>

#include "imgui.h"
#include "menu/binds.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {

enum class Column { left, right };

// A rounded settings panel with a section header above it and fixed-height rows inside.
// Rows are added top to bottom; every row has a label on the left and its control on the right.
// Rows that edit a setting can be bound to a key with a right-click.
// While search::collecting() the panel draws nothing and registers its rows instead.
class Panel {
public:
    // `header_cap_y` is in menu coordinates; `origin` is the menu's top-left corner on screen.
    Panel(const ImVec2& origin, Column column, float header_cap_y, const char* header, int rows);
    // Same, but at an explicit `x` and width `w` (menu coordinates) for views with their own grid.
    Panel(const ImVec2& origin, Column column, float x, float w, float header_cap_y, const char* header, int rows);
    ~Panel();
    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    void toggle(const char* label, bool* v, bool enabled = true);
    // `width` of the settings window, 0 for the default.
    template <class Settings>
    void toggle(const char* label, bool* v, Settings&& settings, bool enabled = true, float width = 0.0f);
    // A toggle whose dots open a window the caller draws itself: `window(pressed)` runs
    // every frame with `pressed` true on the frame the dots were clicked.
    template <class Window>
    void toggle_window(const char* label, bool* v, Window&& window);
    void color_toggle(const char* label, ImVec4* color, bool* v);
    void color_multi(const char* label, ImVec4* color, std::span<const char* const> items, bool* selected);
    void color_combo(const char* label, ImVec4* color, int* current, std::span<const char* const> items);
    void combo(const char* label, int* current, std::span<const char* const> items);
    void multi(const char* label, std::span<const char* const> items, bool* selected,
               const char* placeholder = "Select");
    template <class Settings>
    void multi(const char* label, std::span<const char* const> items, bool* selected, Settings&& settings);
    void slider(const char* label, float* v, float lo, float hi, const char* format, float step = 1.0f);
    template <class Settings>
    void slider(const char* label, float* v, float lo, float hi, const char* format, float step, Settings&& settings);
    // The whole row is a button that opens a settings window (World > Bomb...).
    template <class Settings>
    void chevron(const char* label, Settings&& settings);
    // A chevron row that only reports the click, for rows that switch a menu view
    // instead of opening a settings window.
    bool open_row(const char* label);
    // A plain clickable row with the panel's own styling (map list, list rows).
    bool list_row(const char* label, bool active, bool with_dots, bool* dots_pressed = nullptr,
                   const char* row_image = nullptr);
    // Reserves a row whose content the caller draws itself (the Ambience map poster).
    // Returns the row rectangle on screen; draws nothing.
    void content_row(ImVec2* out_min, ImVec2* out_max);

    // Header position for a panel stacked below this one.
    float next_header_cap_y() const;

private:
    // Screen pixels.
    struct Row {
        float top;
        float bottom;
        float center;
    };

    // Menu coordinates (unscaled, relative to the origin) to whole screen pixels.
    ImVec2 at(float x, float y) const;
    // `highlight` (0..1) fills the row behind the label, for rows that act as buttons. `target`
    // makes the row bindable; call it inside the row's ID scope.
    Row begin_row(const char* label, bool enabled, float highlight = 0.0f, const binds::Target* target = nullptr,
                  const char* row_image = nullptr);
    ImVec2 row_min(const Row& row) const;
    Row row_geometry() const;
    ImVec2 row_max(const Row& row) const;
    ImVec2 toggle_pos(const Row& row) const;
    ImVec2 combo_min(const Row& row) const;
    float control_right() const;
    float slider_x0() const;
    bool dots(const Row& row, bool enabled);
    // "•••" button centered at `x`.
    bool dots_at(float x, const Row& row, bool enabled = true);
    void slider_controls(const Row& row, float* v, float lo, float hi, const char* format, float step);
    void multi_box(const Row& row, std::span<const char* const> items, bool* selected, const char* placeholder);
    // Adds a chevron row; returns true when it was clicked.
    bool chevron_row(const char* label);

    ImDrawList* dl_;
    ImVec2 o_;
    // Menu coordinates (unscaled).
    float x_;
    float y_;
    float w_;
    float pad_left_;
    float pad_right_;
    float sep_pad_right_;
    int rows_;
    int row_ = 0;
};

// Rows of a settings window, collected under the path segment `label` while searching.
template <class Settings>
void collect_settings(const char* label, Settings&& settings)
{
    search::push(label);
    settings();
    search::pop();
}

template <class Settings>
void open_settings(const char* label, bool pressed, Settings&& settings, float width = 0.0f)
{
    if (search::collecting()) {
        collect_settings(label, settings);
        return;
    }
    if (pressed)
        ui::popup::open("##settings");
    if (ui::popup::begin("##settings", label, width)) {
        settings();
        ui::popup::end();
    }
}

template <class Settings>
void Panel::toggle(const char* label, bool* v, Settings&& settings, bool enabled, float width)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::toggle, .b = v, .enabled = enabled });
        if (enabled)
            collect_settings(label, settings);
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::toggle };
    const Row row = begin_row(label, enabled, 0.0f, &t);
    const bool pressed = dots(row, enabled);
    ui::toggle("##toggle", v, toggle_pos(row), enabled);
    open_settings(label, pressed, settings, width);
    ImGui::PopID();
}

template <class Window>
void Panel::toggle_window(const char* label, bool* v, Window&& window)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::toggle, .b = v, .enabled = true });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::toggle };
    const Row row = begin_row(label, true, 0.0f, &t);
    const bool pressed = dots(row, true);
    ui::toggle("##toggle", v, toggle_pos(row), true);
    window(pressed);
    ImGui::PopID();
}

template <class Settings>
void Panel::multi(const char* label, std::span<const char* const> items, bool* selected, Settings&& settings)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::multi, .b = selected, .items = items });
        collect_settings(label, settings);
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, selected, binds::Kind::multi, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row row = begin_row(label, true, 0.0f, &t);
    const bool pressed = dots_at(combo_min(row).x - style::px(style::layout::multi_dots_gap), row);
    multi_box(row, items, selected, "Select");
    open_settings(label, pressed, settings);
    ImGui::PopID();
}

template <class Settings>
void Panel::slider(const char* label, float* v, float lo, float hi, const char* format, float step,
                   Settings&& settings)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::slider, .f = v, .lo = lo, .hi = hi, .step = step,
                      .format = format });
        collect_settings(label, settings);
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::slider, lo, hi, step, format };
    const Row row = begin_row(label, true, 0.0f, &t);
    const bool pressed = dots_at(slider_x0() - style::px(style::layout::slider_dots_dx), row);
    slider_controls(row, v, lo, hi, format, step);
    open_settings(label, pressed, settings);
    ImGui::PopID();
}

template <class Settings>
void Panel::chevron(const char* label, Settings&& settings)
{
    if (search::collecting()) {
        collect_settings(label, settings);
        return;
    }
    ImGui::PushID(label);
    const bool pressed = chevron_row(label);
    open_settings(label, pressed, settings);
    ImGui::PopID();
}

} // namespace nl
