// Menu source watermark: @use_sberbank
#include "menu/map_preview.h"
#include "menu/panel.h"

#include <cmath>

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/style.h"

namespace nl {
namespace {

using namespace nl::style;

constexpr float k_header_to_panel = layout::panel_y - layout::section_cap_y;
constexpr float k_panel_stack_gap = 21.0f;
float panel_height(int rows) { return static_cast<float>(rows) * layout::row_h + 0.6f; }

} // namespace

Panel::Panel(const ImVec2& origin, Column column, float header_cap_y, const char* header, int rows)
    : Panel(origin, column, column == Column::left ? layout::left_panel_x : layout::right_panel_x,
            column == Column::left ? layout::left_panel_w : layout::right_panel_w, header_cap_y, header, rows)
{
}

Panel::Panel(const ImVec2& origin, Column column, float x, float w, float header_cap_y, const char* header, int rows)
    : dl_(ImGui::GetWindowDrawList()), o_(origin), rows_(rows)
{
    const bool left = column == Column::left;
    x_ = x;
    w_ = w;
    sep_pad_right_ = left ? layout::row_sep_pad_right_l : layout::row_sep_pad_right_r;
    pad_left_ = left ? layout::row_pad_left : layout::row_pad_left_r;
    pad_right_ = left ? layout::row_pad_right_l : layout::row_pad_right_r;
    y_ = header_cap_y + k_header_to_panel;
    ImGui::PushID(header);
    if (search::collecting()) {
        search::push(header);
        return;
    }

    const float header_pad = left ? layout::section_pad_left_l : layout::section_pad_left_r;
    draw::text(dl_, fonts().medium, font_size::header, at(x_ + header_pad, header_cap_y), col::section_header, header);

    const ImVec2 min = at(x_, y_);
    const ImVec2 max = at(x_ + w_, y_ + panel_height(rows));
    dl_->AddRectFilled(min, max, col::panel_bg, px(layout::panel_rounding));
    draw::border(dl_, min, max, col::panel_border, px(layout::panel_rounding), draw::hairline());
}

Panel::~Panel()
{
    if (search::collecting())
        search::pop();
    ImGui::PopID();
}

float Panel::next_header_cap_y() const { return y_ + panel_height(rows_) + k_panel_stack_gap; }

ImVec2 Panel::at(float x, float y) const { return ImVec2(o_.x + draw::snap(px(x)), o_.y + draw::snap(px(y))); }

Panel::Row Panel::row_geometry() const
{
    const float top = y_ + layout::row_top_pad + static_cast<float>(row_) * layout::row_h;
    Row r;
    r.top = at(0.0f, top).y;
    r.bottom = at(0.0f, top + layout::row_h).y;
    r.center = (r.top + r.bottom) * 0.5f;
    return r;
}

Panel::Row Panel::begin_row(const char* label, bool enabled, float highlight, const binds::Target* target,
                            const char* row_image)
{
    IM_ASSERT(row_ < rows_);
    const Row r = row_geometry();

    if (row_ > 0)
        draw::hline(dl_, at(x_ + layout::row_pad_left, 0.0f).x, at(x_ + w_ - sep_pad_right_, 0.0f).x,
                    r.top - std::floor(draw::hairline() * 0.5f), col::row_separator);
    if (highlight > 0.0f) {
        // The first and last rows follow the panel's rounded corners.
        const float rounding = px(layout::panel_rounding) - draw::hairline();
        ImDrawFlags flags = ImDrawFlags_RoundCornersNone;
        if (row_ == 0)
            flags |= ImDrawFlags_RoundCornersTop;
        if (row_ == rows_ - 1)
            flags |= ImDrawFlags_RoundCornersBottom;
        const float top_inset = row_ == 0 ? draw::hairline() : 0.0f;
        const float bottom_inset = row_ == rows_ - 1 ? draw::hairline() : 0.0f;
        dl_->AddRectFilled(ImVec2(row_min(r).x, r.top + top_inset), ImVec2(row_max(r).x, r.bottom - bottom_inset),
                           draw::with_alpha(col::panel_row_hover, highlight), rounding, flags);
    }

    ImFont* font = fonts().medium;
    const float size = font_size::label;
    const float cap_y = r.center - draw::cap_height(font, size) * 0.5f + px(layout::row_label_dy);
    const ImU32 color = enabled ? col::label : draw::lerp_color(col::panel_bg, col::label, disabled_alpha);
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    float pen_x = o_.x + px(x_ + pad_left_);
    if (row_image && *row_image) {
        // A picture in front of the label: the Ambience map list shows the map preview
        // here, and the label moves over to make room for it.
        const float inset = px(4.0f);
        const float thumb_h = (r.bottom - r.top) - inset * 2.0f;
        const float thumb_w = thumb_h * 1.6f;
        const ImVec2 image_min(pen_x, r.top + inset);
        const ImVec2 image_max(image_min.x + thumb_w, image_min.y + thumb_h);
        if (!nl::maps::draw(dl_, row_image, image_min, image_max)) {
            dl_->AddRectFilled(image_min, image_max, col::row_separator, px(2.0f));
        } else {
            draw::border(dl_, image_min, image_max, col::row_separator, px(2.0f), draw::hairline());
        }
        pen_x = image_max.x + px(8.0f);
    }
    draw::text(dl_, font, size, ImVec2(pen_x, cap_y), color, label, label_end);
    if (target && enabled) {
        if (binds::bound(target->ptr))
            binds::bound_icon(dl_, pen_x + draw::text_width(font, size, label, label_end), r.center, col::bind_icon);
        binds::row(*target, row_min(r), row_max(r));
    }

    ++row_;
    return r;
}

float Panel::control_right() const { return at(x_ + w_ - pad_right_, 0.0f).x; }

ImVec2 Panel::toggle_pos(const Row& row) const
{
    const ImVec2 size = ui::toggle_size();
    return ImVec2(draw::snap(control_right() - px(layout::toggle_inset_right)) - size.x,
                  draw::snap(row.center - size.y * 0.5f + px(layout::toggle_dy)));
}

ImVec2 Panel::combo_min(const Row& row) const
{
    const ImVec2 size = ui::combo_size();
    return ImVec2(control_right() - size.x, draw::snap(row.center - size.y * 0.5f + px(layout::multi_dy)));
}

bool Panel::dots(const Row& row, bool enabled) { return dots_at(toggle_pos(row).x - px(layout::dots_gap), row, enabled); }

bool Panel::dots_at(float x, const Row& row, bool enabled)
{
    return ui::dots_button("##dots", ImVec2(x, row.center + px(layout::dots_dy)), enabled);
}

ImVec2 Panel::row_min(const Row& row) const { return ImVec2(at(x_, 0.0f).x + draw::hairline(), row.top); }
ImVec2 Panel::row_max(const Row& row) const { return ImVec2(at(x_ + w_, 0.0f).x - draw::hairline(), row.bottom); }

bool Panel::chevron_row(const char* label)
{
    const Row geom = row_geometry();
    const ImGuiID id = ImGui::GetID("##row");
    const ui::ItemState s = ui::item_behavior(id, row_min(geom), row_max(geom));
    const bool open = ImGui::IsPopupOpen(ImGui::GetID("##settings"), ImGuiPopupFlags_None);
    const float h = ui::animate(id + 1, (s.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const Row row = begin_row(label, true, h);
    const ImU32 c = draw::lerp_color(col::dots, col::tab_text_active, h);
    draw::chevron_right(dl_, ImVec2(control_right() - px(4.0f), row.center), px(2.3f), px(4.4f), c, px(1.6f));
    return s.pressed;
}

void Panel::toggle(const char* label, bool* v, bool enabled)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::toggle, .b = v, .enabled = enabled });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::toggle };
    const Row row = begin_row(label, enabled, 0.0f, &t);
    ui::toggle("##toggle", v, toggle_pos(row), enabled);
    ImGui::PopID();
}

void Panel::color_toggle(const char* label, ImVec4* color, bool* v)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::color_toggle, .b = v, .c = color });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::toggle };
    const Row row = begin_row(label, true, 0.0f, &t);
    const ImVec2 tp = toggle_pos(row);
    const float size = draw::snap(px(layout::swatch_size));
    const ImVec2 swatch_min(draw::snap(tp.x - px(layout::swatch_gap)) - size,
                            draw::snap(row.center - size * 0.5f + px(layout::swatch_dy)));
    ui::color_swatch("##color", color, swatch_min, size);
    ui::toggle("##toggle", v, tp);
    ImGui::PopID();
}

void Panel::color_multi(const char* label, ImVec4* color, std::span<const char* const> items, bool* selected)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::multi, .b = selected, .c = color, .items = items });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, selected, binds::Kind::multi, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row row = begin_row(label, true, 0.0f, &t);
    const ImVec2 box_min = combo_min(row);
    const float size = draw::snap(px(layout::swatch_size));
    const ImVec2 swatch_min(draw::snap(box_min.x - px(layout::swatch_gap)) - size,
                            draw::snap(row.center - size * 0.5f + px(layout::swatch_dy)));
    ui::color_swatch("##color", color, swatch_min, size);
    multi_box(row, items, selected, "Select");
    ImGui::PopID();
}

void Panel::color_combo(const char* label, ImVec4* color, int* current, std::span<const char* const> items)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::combo, .c = color, .i = current, .items = items });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, current, binds::Kind::combo, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row row = begin_row(label, true, 0.0f, &t);
    const ImVec2 box_min = combo_min(row);
    const ImVec2 box_max(box_min.x + ui::combo_size().x, box_min.y + ui::combo_size().y);
    const float size = draw::snap(px(layout::swatch_size));
    const ImVec2 swatch_min(draw::snap(box_min.x - px(layout::swatch_gap)) - size,
                            draw::snap(row.center - size * 0.5f + px(layout::swatch_dy)));
    ui::color_swatch("##color", color, swatch_min, size);
    ui::combo_box("##combo", box_min, box_max, items, current);
    ImGui::PopID();
}

void Panel::multi_box(const Row& row, std::span<const char* const> items, bool* selected, const char* placeholder)
{
    const ImVec2 box_min = combo_min(row);
    ui::multi_combo_box("##multi", box_min, ImVec2(box_min.x + ui::combo_size().x, box_min.y + ui::combo_size().y),
                        items, selected, placeholder);
}

void Panel::multi(const char* label, std::span<const char* const> items, bool* selected, const char* placeholder)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::multi, .b = selected, .items = items });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, selected, binds::Kind::multi, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row row = begin_row(label, true, 0.0f, &t);
    multi_box(row, items, selected, placeholder);
    ImGui::PopID();
}

void Panel::combo(const char* label, int* current, std::span<const char* const> items)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::combo, .i = current, .items = items });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, current, binds::Kind::combo, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row row = begin_row(label, true, 0.0f, &t);
    const ImVec2 box_min = combo_min(row);
    ui::combo_box("##combo", box_min, ImVec2(box_min.x + ui::combo_size().x, box_min.y + ui::combo_size().y), items,
                  current);
    ImGui::PopID();
}

float Panel::slider_x0() const { return draw::snap(control_right() - px(layout::slider_track_dx)); }

bool Panel::open_row(const char* label) { return chevron_row(label); }

void Panel::content_row(ImVec2* out_min, ImVec2* out_max)
{
    if (search::collecting())
        return;
    const Row r = row_geometry();
    if (out_min)
        *out_min = row_min(r);
    if (out_max)
        *out_max = row_max(r);
    ++row_;
}
bool Panel::list_row(const char* label, bool active, bool with_dots, bool* dots_pressed,
                     const char* row_image)
{
    if (search::collecting())
        return false;
    ImGui::PushID(label);
    const Row geom = row_geometry();
    const ImVec2 mn = row_min(geom), mx = row_max(geom);
    const ImGuiID id = ImGui::GetID("##row");
    const ui::ItemState s = ui::item_behavior(id, mn, mx);
    const float h = ui::animate(id + 1, (s.hovered || active) ? 1.0f : 0.0f, 18.0f);
    const Row r = begin_row(label, true, ImMax(h, active ? 0.55f : 0.0f), nullptr, row_image);
    bool dots_hit = false;
    if (with_dots)
        dots_hit = dots(r, true);
    ImGui::PopID();
    if (dots_pressed)
        *dots_pressed = dots_hit;
    return s.pressed;
}

void Panel::slider_controls(const Row& row, float* v, float lo, float hi, const char* format, float step)
{
    ui::slider("##slider", slider_x0(), control_right(), row.center, v, lo, hi, format, step);
}

void Panel::slider(const char* label, float* v, float lo, float hi, const char* format, float step)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::slider, .f = v, .lo = lo, .hi = hi, .step = step,
                      .format = format });
        return;
    }
    ImGui::PushID(label);
    const binds::Target t{ label, v, binds::Kind::slider, lo, hi, step, format };
    const Row row = begin_row(label, true, 0.0f, &t);
    slider_controls(row, v, lo, hi, format, step);
    ImGui::PopID();
}

} // namespace nl
