// Menu source watermark: @use_sberbank
#include <cstdio>
#include <cstdlib>

#include "imgui_internal.h"
#include "menu/binds.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl::ui::popup {
namespace {

using namespace nl::style;
using draw::snap;

constexpr float k_width = 208.0f;
constexpr float k_rounding = 8.0f;
constexpr float k_pad = 6.0f;
constexpr float k_pad_bottom = 4.8f;
constexpr float k_title_h = 20.4f;
constexpr float k_title_x = 5.0f;
constexpr float k_group_gap = 5.9f;
constexpr float k_group_pad_top = 5.8f;
constexpr float k_group_pad_bottom = 4.6f;
constexpr float k_group_rounding = 6.5f;
constexpr float k_row_h = 25.5f;
constexpr float k_row_inset = 3.0f;
constexpr float k_row_rounding = 5.0f;
// From the left edge of the group (or of the window content outside groups).
constexpr float k_text_x = 14.8f;
constexpr float k_icon_cx = 18.4f;
constexpr float k_icon_text_x = 35.4f;
constexpr float k_control_pad = 16.2f;
constexpr float k_chip_h = 21.0f;
constexpr float k_chip_pad = 7.5f;
constexpr float k_chip_gap = 5.0f;
// Settings windows open to the right of the cursor, a little above it.
constexpr float k_open_dx = 14.0f;
constexpr float k_open_dy = -76.0f;

ImGuiID key(ImGuiID id, int slot) { return ImHashData(&slot, sizeof(slot), id); }

struct GroupFrame {
    ImGuiWindow* window;
    float top;
};
ImVector<GroupFrame> g_groups;

// Nested settings windows start while a group of the parent window is open.
bool in_group() { return !g_groups.empty() && g_groups.back().window == ImGui::GetCurrentWindow(); }

struct Row {
    ImVec2 min, max; // highlight rect
    float cy;
    float right; // right edge of the control
    bool hovered;
};

// `target` makes the row bindable (right-click menu, keyboard icon after the label).
Row begin_row(const char* label, const char* icon, bool enabled, const binds::Target* target = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = snap(px(k_row_h));
    ImGui::ItemSize(ImVec2(w, h));

    Row r;
    const float inset = snap(px(k_row_inset));
    r.min = ImVec2(p.x + inset, p.y);
    r.max = ImVec2(p.x + w - inset, p.y + h);
    r.cy = p.y + h * 0.5f;
    r.right = snap(p.x + w - px(k_control_pad));
    r.hovered = enabled && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(r.min, r.max);
    const float hl = animate(window->GetID(label) + 7, r.hovered ? 1.0f : 0.0f, 18.0f);
    if (hl > 0.0f)
        dl->AddRectFilled(r.min, r.max, draw::with_alpha(col::window_row_hover, hl), px(k_row_rounding));

    const ImU32 text_col = enabled ? col::window_label : col::window_label_disabled;
    float text_x = p.x + px(k_text_x);
    if (icon) {
        draw::any_icon(dl, ImVec2(p.x + px(k_icon_cx), r.cy), enabled ? col::window_icon : col::window_label_disabled,
                       icon);
        text_x = p.x + px(k_icon_text_x);
    }
    ImFont* font = fonts().medium;
    const char* label_end = ImGui::FindRenderedTextEnd(label);
    const ImVec2 pen(snap(text_x), r.cy - draw::cap_height(font, font_size::popup_label) * 0.5f);
    draw::text(dl, font, font_size::popup_label, pen, text_col, label, label_end);
    if (target && enabled) {
        if (binds::bound(target->ptr))
            binds::bound_icon(dl, pen.x + draw::text_width(font, font_size::popup_label, label, label_end), r.cy,
                              col::bind_icon);
        ImGui::PushID(label);
        binds::row(*target, r.min, r.max);
        ImGui::PopID();
    }
    return r;
}

binds::Target toggle_target(const char* label, bool* v) { return { label, v, binds::Kind::toggle }; }

ImVec2 toggle_pos(const Row& r)
{
    const ImVec2 size = toggle_size();
    return ImVec2(snap(r.right - size.x), snap(r.cy - size.y * 0.5f));
}

ImVec2 swatch_min(float right, float cy, float size) { return ImVec2(snap(right - size), snap(cy - size * 0.5f)); }

} // namespace

bool collecting() { return search::collecting(); }
void collect_push(const char* label) { search::push(label); }
void collect_pop() { search::pop(); }

void open(const char* str_id)
{
    const ImGuiID id = ImGui::GetID(str_id);
    const ImVec2 m = ImGui::GetIO().MousePos;
    ImGuiStorage* storage = ImGui::GetStateStorage();
    storage->SetFloat(key(id, 0), m.x + px(k_open_dx));
    storage->SetFloat(key(id, 1), m.y + px(k_open_dy));
    ImGui::OpenPopup(id);
}

void place(const char* str_id, const ImVec2& pos)
{
    const ImGuiID id = ImGui::GetID(str_id);
    ImGuiStorage* storage = ImGui::GetStateStorage();
    storage->SetFloat(key(id, 0), pos.x);
    storage->SetFloat(key(id, 1), pos.y);
}

float last_height(const char* str_id) { return ImGui::GetStateStorage()->GetFloat(key(ImGui::GetID(str_id), 2), 0.0f); }

bool begin(const char* str_id, const char* title, float width)
{
    const ImGuiID id = ImGui::GetID(str_id);
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        return false;
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImVec2 pos(storage->GetFloat(key(id, 0), ImGui::GetIO().MousePos.x),
                     storage->GetFloat(key(id, 1), ImGui::GetIO().MousePos.y));
    if (!begin_popup_at(id, pos, px(width > 0.0f ? width : k_width), ImVec2(px(k_pad), px(k_pad_bottom)),
                        px(k_rounding)))
        return false;
    storage->SetFloat(key(id, 2), ImGui::GetCurrentWindow()->Size.y);
    ImGui::ItemSize(ImVec2(0.0f, snap(px(k_pad - k_pad_bottom))));
    if (!title)
        return true;

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float h = snap(px(k_title_h));
    ImGui::ItemSize(ImVec2(ImGui::GetContentRegionAvail().x, h));
    ImFont* font = fonts().semibold;
    draw::text(ImGui::GetWindowDrawList(), font, font_size::window_title,
               ImVec2(p.x + px(k_title_x), p.y + (h - draw::cap_height(font, font_size::window_title)) * 0.5f),
               col::window_title, title, ImGui::FindRenderedTextEnd(title));
    return true;
}

void end()
{
    IM_ASSERT(!in_group());
    end_popup();
}

void begin_group()
{
    if (search::collecting())
        return;
    // The first group sits right under the title; later ones are spaced by the same gap.
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->DC.CursorPos.y > window->DC.CursorStartPos.y + snap(px(k_pad - k_pad_bottom)) + 0.5f)
        ImGui::ItemSize(ImVec2(0.0f, snap(px(k_group_gap))));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->ChannelsSplit(2);
    dl->ChannelsSetCurrent(1);
    g_groups.push_back({ window, ImGui::GetCursorScreenPos().y });
    ImGui::ItemSize(ImVec2(0.0f, snap(px(k_group_pad_top))));
}

void end_group()
{
    if (search::collecting())
        return;
    // Same reasoning as end_popup(): never index a frame stack that has run dry.
    if (!in_group())
        return;
    ImGui::ItemSize(ImVec2(0.0f, snap(px(k_group_pad_bottom))));
    const GroupFrame g = g_groups.back();
    g_groups.pop_back();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    dl->ChannelsSetCurrent(0);
    const ImVec2 min(window->WorkRect.Min.x, g.top);
    const ImVec2 max(window->WorkRect.Max.x, ImGui::GetCursorScreenPos().y);
    dl->AddRectFilled(min, max, col::window_group_bg, px(k_group_rounding));
    draw::border(dl, min, max, col::window_group_border, px(k_group_rounding), draw::hairline());
    dl->ChannelsMerge();
}

bool toggle_row(const char* label, bool* v, const char* icon, bool enabled, bool with_dots, bool* dots_pressed)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::toggle, .b = v, .enabled = enabled });
        return false;
    }
    const binds::Target t = toggle_target(label, v);
    const Row r = begin_row(label, icon, enabled, &t);
    ImGui::PushID(label);
    const ImVec2 tp = toggle_pos(r);
    if (with_dots) {
        const bool pressed = dots_button("##dots", ImVec2(tp.x - px(layout::dots_gap), r.cy + px(layout::dots_dy)), enabled);
        if (dots_pressed)
            *dots_pressed = pressed;
    }
    const bool changed = ui::toggle("##toggle", v, tp, enabled);
    ImGui::PopID();
    return changed;
}

bool toggle(const char* label, bool* v, const char* icon, bool enabled)
{
    return toggle_row(label, v, icon, enabled, false, nullptr);
}

bool color_toggle(const char* label, ImVec4* color, bool* v, const char* icon, bool enabled)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::color_toggle, .b = v, .c = color, .enabled = enabled });
        return false;
    }
    const binds::Target t = toggle_target(label, v);
    const Row r = begin_row(label, icon, enabled, &t);
    ImGui::PushID(label);
    const ImVec2 tp = toggle_pos(r);
    const float size = snap(px(layout::swatch_size));
    color_swatch("##color", color, swatch_min(snap(tp.x - px(layout::swatch_gap)), r.cy, size), size, enabled);
    const bool changed = ui::toggle("##toggle", v, tp, enabled);
    ImGui::PopID();
    return changed;
}

bool color(const char* label, ImVec4* color, bool enabled, const char* icon, const char* extra_icon)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::color, .c = color, .enabled = enabled });
        return false;
    }
    const Row r = begin_row(label, icon, enabled);
    ImGui::PushID(label);
    const float size = snap(px(layout::swatch_size));
    const ImVec2 smin = swatch_min(r.right, r.cy, size);
    if (extra_icon)
        draw::any_icon(ImGui::GetWindowDrawList(), ImVec2(smin.x - px(13.0f), r.cy), col::window_value, extra_icon);
    const bool changed = color_swatch("##color", color, smin, size, enabled);
    ImGui::PopID();
    return changed;
}

bool slider(const char* label, float* v, float lo, float hi, const char* format, float step, bool enabled,
            const char* icon)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::slider, .f = v, .lo = lo, .hi = hi, .step = step,
                      .format = format, .enabled = enabled });
        return false;
    }
    const binds::Target t{ label, v, binds::Kind::slider, lo, hi, step, format };
    const Row r = begin_row(label, icon, enabled, &t);
    ImGui::PushID(label);
    const float x0 = snap(r.right - px(layout::bar_slider_w));
    const bool changed = ui::bar_slider("##slider", ImVec2(x0, r.min.y), ImVec2(r.right, r.max.y), v, lo, hi, format,
                                        step, enabled);
    ImGui::PopID();
    return changed;
}

bool combo(const char* label, int* current, std::span<const char* const> items, bool enabled)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::combo, .i = current, .items = items, .enabled = enabled });
        return false;
    }
    const binds::Target t{ label, current, binds::Kind::combo, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row r = begin_row(label, nullptr, enabled, &t);
    ImGui::PushID(label);
    const ImVec2 size = combo_size();
    const ImVec2 min(r.right - size.x, snap(r.cy - size.y * 0.5f));
    const bool changed = combo_box("##combo", min, ImVec2(min.x + size.x, min.y + size.y), items, current, enabled);
    ImGui::PopID();
    return changed;
}

bool multi_combo(const char* label, std::span<const char* const> items, bool* selected, bool enabled)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::multi, .b = selected, .items = items, .enabled = enabled });
        return false;
    }
    const binds::Target t{ label, selected, binds::Kind::multi, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row r = begin_row(label, nullptr, enabled, &t);
    ImGui::PushID(label);
    const ImVec2 size = combo_size();
    const ImVec2 min(r.right - size.x, snap(r.cy - size.y * 0.5f));
    const bool changed =
        multi_combo_box("##multi", min, ImVec2(min.x + size.x, min.y + size.y), items, selected, "Select", enabled);
    ImGui::PopID();
    return changed;
}

bool item(const char* label, const char* icon)
{
    if (search::collecting())
        return false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const Row r = begin_row(label, icon, true);
    return item_behavior(window->GetID(label), r.min, r.max).pressed;
}

namespace {

// The current choice and a chevron at the right of the row; clicking anywhere in
// [hit_min, r.max] opens the list. Returns the left edge of the text through `text_left`.
bool value_control(const Row& r, const ImVec2& hit_min, int* current, std::span<const char* const> items, bool enabled,
                   float* text_left = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID("##value");
    const ItemState s = item_behavior(id, hit_min, r.max, enabled);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    if (s.pressed && !open)
        ImGui::OpenPopup(id);

    *current = ImClamp(*current, 0, static_cast<int>(items.size()) - 1);
    const char* text = items[static_cast<size_t>(*current)];
    ImFont* font = fonts().medium;
    const float size = font_size::popup_label;
    const float chevron_x = r.right - px(2.0f);
    const float text_r = chevron_x - px(6.5f);
    const float text_x = text_r - draw::text_width(font, size, text);
    if (text_left)
        *text_left = text_x;
    const ImU32 c = enabled ? (open || s.hovered ? col::window_label : col::window_value) : col::window_label_disabled;
    draw::text(window->DrawList, font, size, ImVec2(text_x, r.cy - draw::cap_height(font, size) * 0.5f), c, text);
    draw::chevron_right(window->DrawList, ImVec2(chevron_x, r.cy), px(1.8f), px(3.6f), c, px(1.5f));

    bool changed = false;
    const float width = px(140.0f);
    if (begin_popup_id(id, ImVec2(r.max.x - width, r.min.y), r.max, width)) {
        const int i = dropdown_list(items, *current);
        if (i >= 0) {
            changed = *current != i;
            *current = i;
            ImGui::CloseCurrentPopup();
        }
        end_popup();
    }
    return changed;
}

// Dark box at the right of the row, the frame of the text and number fields.
void field_box(const Row& r, float width, bool enabled, ImVec2& min, ImVec2& max)
{
    const float h = snap(px(17.0f));
    max = ImVec2(r.right, snap(r.cy - h * 0.5f) + h);
    min = ImVec2(snap(r.right - width), max.y - h);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rounding = px(layout::slider_box_rounding);
    const float alpha = enabled ? 1.0f : 0.5f;
    dl->AddRectFilled(min, max, draw::with_alpha(col::combo_bg, alpha), rounding);
    draw::border(dl, min, max, draw::with_alpha(col::slider_box_border, alpha), rounding, draw::hairline());
}

constexpr float k_field_w = 62.0f;
constexpr float k_field_pad = 6.0f;

} // namespace

bool input(const char* label, char* buf, int size, const char* placeholder, bool enabled)
{
    if (search::collecting())
        return false;
    const Row r = begin_row(label, nullptr, enabled);
    ImGui::PushID(label);
    ImVec2 min, max;
    field_box(r, px(k_field_w), enabled, min, max);
    bool changed = false;
    if (enabled)
        changed = text_field("##input", buf, size, ImVec2(min.x + px(k_field_pad), min.y),
                             ImVec2(max.x - px(k_field_pad), max.y), placeholder);
    else
        draw::text(ImGui::GetWindowDrawList(), fonts().medium, font_size::combo,
                   ImVec2(min.x + px(k_field_pad), r.cy - draw::cap_height(fonts().medium, font_size::combo) * 0.5f),
                   col::window_label_disabled, buf[0] ? buf : (placeholder ? placeholder : ""));
    ImGui::PopID();
    return changed;
}

bool number(const char* label, int* v, int lo, int hi, bool enabled)
{
    if (search::collecting())
        return false;
    const Row r = begin_row(label, nullptr, enabled);
    ImGui::PushID(label);
    ImVec2 min, max;
    field_box(r, px(k_field_w), enabled, min, max);
    // ImGui keeps its own copy of the text while the field is active, so formatting
    // the value into a scratch buffer every frame does not fight the user's typing.
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", *v);
    bool changed = false;
    if (enabled) {
        if (text_field("##number", buf, sizeof(buf), ImVec2(min.x + px(k_field_pad), min.y),
                       ImVec2(max.x - px(k_field_pad), max.y), "0", false, 0, ImGuiInputTextFlags_CharsDecimal)) {
            const int next = ImClamp(static_cast<int>(std::strtol(buf, nullptr, 10)), lo, hi);
            changed = next != *v;
            *v = next;
        }
    } else {
        draw::text(ImGui::GetWindowDrawList(), fonts().medium, font_size::combo,
                   ImVec2(min.x + px(k_field_pad), r.cy - draw::cap_height(fonts().medium, font_size::combo) * 0.5f),
                   col::window_label_disabled, buf);
    }
    ImGui::PopID();
    return changed;
}

bool wear_bar(const char* str_id, float* wear)
{
    if (search::collecting())
        return false;
    // Condition bands of the float, Factory New to Battle-Scarred.
    struct Band {
        float end;
        ImU32 color;
    };
    static constexpr Band k_bands[] = {
        { 0.07f, IM_COL32(64, 196, 160, 255) },
        { 0.15f, IM_COL32(126, 200, 74, 255) },
        { 0.38f, IM_COL32(226, 200, 64, 255) },
        { 0.45f, IM_COL32(232, 140, 60, 255) },
        { 1.00f, IM_COL32(226, 78, 78, 255) },
    };

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = snap(px(22.0f));
    ImGui::ItemSize(ImVec2(w, h));

    const float x0 = snap(p.x + px(k_text_x));
    const float x1 = snap(p.x + w - px(k_control_pad));
    const float bar_h = ImMax(3.0f, snap(px(4.0f)));
    const float bar_y = snap(p.y + h - px(7.0f) - bar_h);
    const ImGuiID id = window->GetID(str_id);
    const ItemState s = item_behavior(id, ImVec2(x0 - px(4.0f), p.y), ImVec2(x1 + px(4.0f), p.y + h));

    bool changed = false;
    if (s.held) {
        const float next = ImSaturate((ImGui::GetIO().MousePos.x - x0) / (x1 - x0));
        changed = next != *wear;
        *wear = next;
    }
    if (s.hovered && ImGui::GetIO().MouseWheel != 0.0f) {
        *wear = ImSaturate(*wear + ImGui::GetIO().MouseWheel * 0.001f);
        changed = true;
    }

    const float gap = snap(px(2.0f));
    float from = 0.0f;
    for (const Band& band : k_bands) {
        const float a = ImLerp(x0, x1, from) + (from > 0.0f ? gap * 0.5f : 0.0f);
        const float b = ImLerp(x0, x1, band.end) - (band.end < 1.0f ? gap * 0.5f : 0.0f);
        dl->AddRectFilled(ImVec2(snap(a), bar_y), ImVec2(snap(b), bar_y + bar_h), band.color, bar_h * 0.5f);
        from = band.end;
    }

    const float hl = animate(id + 1, (s.hovered || s.held) ? 1.0f : 0.0f, 18.0f);
    const float mx = ImLerp(x0, x1, animate(id + 2, ImSaturate(*wear), 24.0f));
    const float tri = px(3.6f);
    const float ty = bar_y - px(2.5f);
    const ImU32 marker = draw::lerp_color(col::window_label, IM_COL32_WHITE, hl);
    dl->AddTriangleFilled(ImVec2(mx - tri, ty - tri * 1.25f), ImVec2(mx + tri, ty - tri * 1.25f), ImVec2(mx, ty),
                          marker);
    if (s.hovered || s.held) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.4f", *wear);
        value_tooltip(buf);
    }
    return changed;
}

bool material(const char* label, ImVec4* color, int* current, std::span<const char* const> items, bool* overlay,
              float* exponent, bool enabled)
{
    if (search::collecting())
        return false;
    const Row r = begin_row(label, nullptr, enabled);
    ImGui::PushID(label);
    // The text is measured before the swatch is placed, so the list opens from the
    // value only and a click on the swatch reaches the picker.
    ImFont* font = fonts().medium;
    const int shown = ImClamp(*current, 0, static_cast<int>(items.size()) - 1);
    const float text_x = r.right - px(8.5f) - draw::text_width(font, font_size::popup_label, items[static_cast<size_t>(shown)]);
    bool changed = value_control(r, ImVec2(text_x - px(4.0f), r.min.y), current, items, enabled);
    const float size = snap(px(layout::swatch_size));
    const ImVec2 smin = swatch_min(snap(text_x - px(layout::swatch_gap)), r.cy, size);
    const ImVec2 smax(smin.x + size, smin.y + size);
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID("##color");
    swatch_face(id, color, smin, smax, enabled);
    // Same placement and padding as the picker of a plain swatch, with the material's
    // own settings under it.
    if (begin_popup_at(id, ImVec2(smax.x + px(10.0f), smin.y - px(40.0f)), px(182.0f), ImVec2(px(7.0f), px(7.0f)),
                       px(8.0f))) {
        changed |= color_picker_embed("##picker", color);
        ImGui::ItemSize(ImVec2(0.0f, snap(px(4.0f))));
        changed |= toggle("Overlay", overlay);
        changed |= slider("Exponent", exponent, 0.0f, 100.0f, "%.0f");
        end_popup();
    }
    ImGui::PopID();
    return changed;
}

bool value(const char* label, int* current, std::span<const char* const> items, bool enabled, const char* icon)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::combo, .i = current, .items = items, .enabled = enabled });
        return false;
    }
    const binds::Target t{ label, current, binds::Kind::combo, 0.0f, 1.0f, 1.0f, nullptr, items };
    const Row r = begin_row(label, icon, enabled, &t);
    ImGui::PushID(label);
    const bool changed = value_control(r, r.min, current, items, enabled);
    ImGui::PopID();
    return changed;
}

bool check(const char* label, bool* v, bool enabled)
{
    if (search::collecting()) {
        search::add({ .label = label, .kind = search::Kind::toggle, .b = v, .enabled = enabled });
        return false;
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = snap(px(k_row_h));
    const float inset = snap(px(k_row_inset));
    const ImVec2 min(p.x + inset, p.y), max(p.x + w - inset, p.y + h);
    const ImGuiID id = window->GetID(label);
    ImGui::ItemSize(ImVec2(w, h));
    const ItemState s = item_behavior(id, min, max, enabled);
    if (s.pressed)
        *v = !*v;
    if (enabled) {
        ImGui::PushID(label);
        binds::row(toggle_target(label, v), min, max);
        ImGui::PopID();
    }

    ImDrawList* dl = window->DrawList;
    const float cy = p.y + h * 0.5f;
    const float hl = animate(id + 7, s.hovered ? 1.0f : 0.0f, 18.0f);
    if (hl > 0.0f)
        dl->AddRectFilled(min, max, draw::with_alpha(col::window_row_hover, hl), px(k_row_rounding));
    const float t = animate(id + 8, *v ? 1.0f : 0.0f, 22.0f);
    if (t > 0.0f)
        draw::check_mark(dl, ImVec2(p.x + px(k_icon_cx), cy), px(8.0f), draw::with_alpha(col::window_label, t),
                         px(1.6f));
    ImFont* font = fonts().medium;
    const ImU32 c = !enabled ? col::window_check_disabled
                             : draw::lerp_color(col::window_value, col::window_label, ImMax(t, hl * 0.6f));
    draw::text(dl, font, font_size::popup_label,
               ImVec2(p.x + px(k_icon_text_x), cy - draw::cap_height(font, font_size::popup_label) * 0.5f), c, label);
    return s.pressed;
}

bool chips(std::span<const char* const> items, bool* selected)
{
    if (search::collecting()) {
        search::add({ .label = "Elements", .kind = search::Kind::multi, .b = selected, .items = items });
        return false;
    }
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    ImGui::ItemSize(ImVec2(0.0f, snap(px(k_group_gap))));
    const float inset = in_group() ? snap(px(k_row_inset + 4.0f)) : 0.0f;
    const ImVec2 p(ImGui::GetCursorScreenPos().x + inset, ImGui::GetCursorScreenPos().y);
    const float right = window->WorkRect.Max.x - inset;
    const float h = snap(px(k_chip_h)), gap = snap(px(k_chip_gap)), pad = px(k_chip_pad);
    ImFont* font = fonts().medium;
    const float size = font_size::popup_label;
    float x = p.x, y = p.y;
    bool changed = false;
    for (size_t i = 0; i < items.size(); ++i) {
        const float w = snap(draw::text_width(font, size, items[i]) + pad * 2.0f);
        if (x > p.x && x + w > right) {
            x = p.x;
            y += h + gap;
        }
        const ImVec2 min(x, y), max(x + w, y + h);
        // Index alone collides when two chip rows are visible at once (the ESP items
        // list draws one row per element), which ImGui reports as conflicting IDs.
        ImGui::PushID(static_cast<int>(min.x * 4.0f) * 100000 + static_cast<int>(min.y * 4.0f));
        const ImGuiID id = window->GetID("##chip");
        ImGui::PopID();
        const ItemState s = item_behavior(id, min, max);
        if (s.pressed) {
            selected[i] = !selected[i];
            changed = true;
        }
        const float t = animate(id + 1, selected[i] ? 1.0f : 0.0f, 20.0f);
        const float hl = animate(id + 2, s.hovered ? 1.0f : 0.0f, 18.0f);
        ImU32 bg = draw::lerp_color(col::chip_bg, col::chip_bg_selected, t);
        bg = draw::lerp_color(bg, col::chip_bg_hovered, hl * 0.5f);
        dl->AddRectFilled(min, max, bg, px(4.0f));
        const ImU32 tc = draw::lerp_color(draw::lerp_color(col::chip_text, col::window_label, hl * 0.5f),
                                          col::tab_text_active, t);
        draw::text(dl, font, size, ImVec2(x + pad, y + (h - draw::cap_height(font, size)) * 0.5f), tc, items[i]);
        x += w + gap;
    }
    ImGui::ItemSize(ImVec2(right - p.x, y + h - p.y + snap(px(4.0f))));
    return changed;
}

} // namespace nl::ui::popup
