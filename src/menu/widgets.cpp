// Menu source watermark: @use_sberbank
#include "menu/widgets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "imgui.h"
#include "imgui_internal.h"

#include "menu/blur.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/style.h"

namespace nl::ui {
namespace {

using namespace nl::style;
using draw::snap;

// Storage slots for one widget derive from (id, slot). ImGui's own hashing is
// CRC32-based and therefore linear in the seed: with different seeds the same
// slot keeps the same relative distance, so pairs from unrelated widgets land on
// one key. A float written by one widget was then read back as an int index by
// another and indexed the item list far out of bounds (crash in combo_box:
// 0x43C88000 = float 401.0 used as an index). Mix hard instead.
ImGuiID anim_key(ImGuiID id, int slot)
{
    std::uint64_t x = static_cast<std::uint64_t>(id )
        ^ ( static_cast<std::uint64_t>( static_cast<std::uint32_t>( slot ) )
            * 0x9E3779B97F4A7C15ull );
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBull;
    x ^= x >> 31;
    return static_cast<ImGuiID>( x ^ ( x >> 32 ) );
}

struct PopupFrame {
    float alpha;
    bool clip; // reveal animation: content clipped to the growing frame
};
ImVector<PopupFrame> g_popups;
bool g_next_reveal = false;
bool g_next_from_bottom = false;

constexpr float k_popup_pad_x = 10.0f;
constexpr float k_popup_pad_y = 6.0f;
constexpr float k_popup_gap = 4.0f;
constexpr float k_popup_rounding = 6.0f;
constexpr float k_dropdown_item_h = 26.0f;
constexpr int k_dropdown_visible = 6;
constexpr float k_scrollbar_w = 3.0f;
constexpr float k_combo_text_pad = 5.5f;
constexpr float k_combo_chevron_pad = 9.0f;

// Color picker (measured from the Snow settings of the reference).
constexpr float k_picker_w = 182.0f;
constexpr float k_picker_pad = 7.0f;
constexpr float k_picker_tag_h = 16.0f, k_picker_tag_pad = 4.5f, k_picker_tag_swatch = 9.3f, k_picker_tag_gap = 10.0f;
constexpr float k_picker_sep_gap = 5.5f;
constexpr float k_picker_sv_h = 170.0f, k_picker_sv_rounding = 5.0f, k_picker_ring_r = 5.0f;
constexpr float k_picker_gap = 7.3f;
constexpr float k_picker_hue_h = 7.0f, k_picker_hue_knob_w = 7.0f, k_picker_hue_knob_h = 12.5f;
constexpr float k_picker_alpha_h = 6.0f, k_picker_alpha_knob_r = 5.0f, k_picker_checker = 3.0f;
constexpr float k_picker_row_gap = 12.5f, k_picker_row_h = 15.0f;
constexpr float k_picker_mode_chevron_x = 6.5f, k_picker_mode_text_x = 12.5f;
constexpr float k_picker_field_x = 50.5f, k_picker_field_w = 66.4f; // from the content left edge
constexpr float k_picker_alpha_box_x = 121.0f, k_picker_alpha_box_w = 30.8f;
constexpr float k_picker_palette_cx = 162.0f;
constexpr const char* k_picker_modes[] = { "HEX", "RGB", "HSV" };

std::string join_selected(std::span<const char* const> items, const bool* selected, const char* placeholder)
{
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (!selected[i])
            continue;
        if (!out.empty())
            out += ", ";
        out += items[i];
    }
    return out.empty() ? std::string(placeholder) : out;
}

// Sets the alpha of the vertices [v0, v1) from a0 at y0 to a1 at y1, keeping their color.
void shade_alpha_v(ImDrawList* dl, int v0, int v1, float y0, float y1, float a0, float a1)
{
    for (int i = v0; i < v1; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float t = ImSaturate((v.pos.y - y0) / ImMax(y1 - y0, 1.0f));
        const float a = ImLerp(a0, a1, t) * static_cast<float>((v.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
    }
}

void shade_alpha_h(ImDrawList* dl, int v0, int v1, float x0, float x1, float a0, float a1)
{
    for (int i = v0; i < v1; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        const float t = ImSaturate((v.pos.x - x0) / ImMax(x1 - x0, 1.0f));
        const float a = ImLerp(a0, a1, t) * static_cast<float>((v.col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
        v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
    }
}

ImU32 hue_color(float h)
{
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(h, 1.0f, 1.0f, r, g, b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
}

ImU32 opaque_u32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 1.0f)); }

int byte(float v) { return static_cast<int>(ImSaturate(v) * 255.0f + 0.5f); }

// Colors saved with the palette button of the picker, shared by all pickers.
ImVector<ImVec4> g_palette = [] {
    ImVector<ImVec4> v;
    for (ImU32 c : { IM_COL32(255, 255, 255, 255), IM_COL32(0, 109, 255, 255), IM_COL32(160, 166, 240, 255),
                     IM_COL32(245, 72, 102, 255), IM_COL32(62, 214, 120, 255), IM_COL32(242, 182, 58, 255) })
        v.push_back(ImGui::ColorConvertU32ToFloat4(c));
    return v;
}();

void checkerboard(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float cell, float rounding)
{
    dl->AddRectFilled(min, max, IM_COL32(206, 206, 211, 255), rounding);
    dl->PushClipRect(ImVec2(min.x + rounding * 0.3f, min.y), ImVec2(max.x - rounding * 0.3f, max.y), true);
    int row = 0;
    for (float y = min.y; y < max.y; y += cell, ++row) {
        int i = row & 1;
        for (float x = min.x; x < max.x; x += cell, ++i)
            if (i & 1)
                dl->AddRectFilled(ImVec2(x, y), ImVec2(ImMin(x + cell, max.x), ImMin(y + cell, max.y)),
                                  IM_COL32(176, 177, 184, 255));
    }
    dl->PopClipRect();
}

// Square swatch with a 1 px dark ring.
void picker_swatch(ImDrawList* dl, const ImVec2& min, float size, const ImVec4& c)
{
    const ImVec2 max(min.x + size, min.y + size);
    dl->AddRectFilled(min, max, opaque_u32(c), px(2.2f));
}

void format_color(char* out, size_t size, const ImVec4& c, int mode, bool alpha = true)
{
    if (mode == 0 && !alpha) {
        std::snprintf(out, size, "%02X%02X%02X", byte(c.x), byte(c.y), byte(c.z));
        return;
    }
    if (mode == 1) {
        std::snprintf(out, size, "%d, %d, %d", byte(c.x), byte(c.y), byte(c.z));
    } else if (mode == 2) {
        float h, s, v;
        ImGui::ColorConvertRGBtoHSV(c.x, c.y, c.z, h, s, v);
        std::snprintf(out, size, "%d\xc2\xb0, %d%%, %d%%", static_cast<int>(h * 360.0f + 0.5f),
                      static_cast<int>(s * 100.0f + 0.5f), static_cast<int>(v * 100.0f + 0.5f));
    } else {
        std::snprintf(out, size, "%02X%02X%02X%02X", byte(c.x), byte(c.y), byte(c.z), byte(c.w));
    }
}

// Parses what format_color() writes; returns false for text it can't read.
bool parse_color(const char* text, int mode, ImVec4* c)
{
    if (mode == 0) {
        while (*text == '#' || *text == ' ')
            ++text;
        unsigned v = 0;
        int digits = 0;
        for (; *text && digits < 8; ++text, ++digits) {
            const char ch = *text;
            int d = -1;
            if (ch >= '0' && ch <= '9') d = ch - '0';
            else if (ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
            if (d < 0)
                break;
            v = (v << 4) | static_cast<unsigned>(d);
        }
        if (digits != 6 && digits != 8)
            return false;
        if (digits == 6)
            v = (v << 8) | 0xFFu;
        *c = ImVec4(((v >> 24) & 0xFF) / 255.0f, ((v >> 16) & 0xFF) / 255.0f, ((v >> 8) & 0xFF) / 255.0f,
                    (v & 0xFF) / 255.0f);
        return true;
    }
    float a = 0.0f, b = 0.0f, d = 0.0f;
    std::string clean(text);
    for (char& ch : clean)
        if (!(ch == '.' || ch == '-' || (ch >= '0' && ch <= '9')))
            ch = ' ';
    if (std::sscanf(clean.c_str(), "%f %f %f", &a, &b, &d) != 3)
        return false;
    if (mode == 1) {
        c->x = ImSaturate(a / 255.0f);
        c->y = ImSaturate(b / 255.0f);
        c->z = ImSaturate(d / 255.0f);
    } else {
        ImGui::ColorConvertHSVtoRGB(ImClamp(a / 360.0f, 0.0f, 0.9999f), ImSaturate(b / 100.0f), ImSaturate(d / 100.0f),
                                    c->x, c->y, c->z);
    }
    return true;
}

// Extra colors of every picker ("Secondary" and on), kept per picker.
ImVec4& secondary_color(ImGuiID id, int slot = 1)
{
    if (slot > 1)
        id = ImHashData(&slot, sizeof(slot), id);
    static ImGuiStorage keys;
    static ImVector<ImVec4> colors;
    int index = keys.GetInt(id, -1);
    if (index < 0) {
        index = colors.Size;
        colors.push_back(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        keys.SetInt(id, index);
    }
    return colors[index];
}

// Saturation/value square, hue and alpha bars and the value row of the color picker.
bool color_picker(ImGuiID id, ImVec4* primary, const PickerStyle& style)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImFont* font = fonts().medium;
    const float w = ImGui::GetContentRegionAvail().x;
    const float dt = ImGui::GetIO().DeltaTime;
    bool changed = false;

    // Up to three colors: the tag shows the one being edited, the swatches next to it
    // switch to the others. order[0] is the color in the tag, the rest sit beside it.
    const int colors = ImClamp(style.colors, 1, 3);
    int order[3];
    for (int i = 0; i < 3; ++i) {
        int* stored = storage->GetIntRef(anim_key(id, 263 + i), i);
        if (ImGui::IsWindowAppearing() || *stored < 0 || *stored >= colors)
            *stored = i;
        order[i] = *stored;
    }
    int* swapped = storage->GetIntRef(anim_key(id, 266), 1);
    const auto slot_color = [&](int slot) { return slot == 0 ? primary : &secondary_color(id, slot); };
    // Created up front: a new slot can grow the storage and move the ones already handed out.
    for (int slot = 1; slot < colors; ++slot)
        static_cast<void>(secondary_color(id, slot));
    ImVec4* color = slot_color(order[0]);
    float* swap_anim = storage->GetFloatRef(anim_key(id, 261), 1.0f);
    *swap_anim = ImMin(1.0f, *swap_anim + dt * 6.0f);
    const float swap_e = 1.0f - (1.0f - *swap_anim) * (1.0f - *swap_anim);

    // Tag row: [swatch Primary]  [other colors]
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float h = snap(px(k_picker_tag_h));
        ImGui::ItemSize(ImVec2(w, h));
        const float size = font_size::popup_label;
        const float sw = snap(px(k_picker_tag_swatch));
        const float side_gap = snap(px(6.0f));
        static constexpr const char* k_slot_names[] = { "Primary", "Secondary", "Tertiary" };
        const char* name = order[0] == 0 && style.title ? style.title : k_slot_names[order[0]];
        const float tag_target = snap(px(k_picker_tag_pad) * 2.5f + sw + px(k_picker_tag_pad) + draw::text_width(font, size, name));
        const float tag_w = snap(animate(anim_key(id, 262), tag_target, 20.0f));
        const float total = tag_w + (colors > 1 ? snap(px(k_picker_tag_gap)) + (colors - 1) * sw + (colors - 2) * side_gap : 0.0f);
        const ImVec2 tmin(snap(p.x + (w - total) * 0.5f), p.y), tmax(tmin.x + tag_w, p.y + h);
        const float cy = p.y + h * 0.5f;
        dl->AddRectFilled(tmin, tmax, col::picker_field_bg, px(4.5f));
        // The swatches trade places: the picked one grows into the tag, the tag's shrinks out to the side.
        const ImVec2 tag_sw(tmin.x + snap(px(k_picker_tag_pad)), snap(cy - sw * 0.5f));
        const auto side_sw = [&](int i) {
            return ImVec2(tmax.x + snap(px(k_picker_tag_gap)) + static_cast<float>(i - 1) * (sw + side_gap), tag_sw.y);
        };
        const float from_x = colors > 1 ? side_sw(*swapped).x : tag_sw.x;
        picker_swatch(dl, ImVec2(ImLerp(from_x, tag_sw.x, swap_e), tag_sw.y), sw, *color);
        dl->PushClipRect(tmin, tmax, true);
        draw::text(dl, font, size,
                   ImVec2(snap(tmin.x + px(k_picker_tag_pad) * 2.0f + sw),
                          cy - draw::cap_height(font, size) * 0.5f + (1.0f - swap_e) * px(6.0f)),
                   draw::with_alpha(col::window_label, swap_e), name);
        dl->PopClipRect();
        int picked = 0;
        for (int i = 1; i < colors; ++i) {
            const ImVec2 at = side_sw(i);
            const ItemState rs = item_behavior(anim_key(id, 280 + i), ImVec2(at.x - px(3.0f), at.y - px(3.0f)),
                                               ImVec2(at.x + sw + px(3.0f), at.y + sw + px(3.0f)));
            const float hv = animate(anim_key(id, 285 + i), rs.hovered ? 1.0f : 0.0f, 18.0f);
            const ImVec2 omin(i == *swapped ? ImLerp(tag_sw.x, at.x, swap_e) : at.x, at.y);
            const float grow = hv * px(1.0f);
            picker_swatch(dl, ImVec2(omin.x - grow, omin.y - grow), sw + grow * 2.0f, *slot_color(order[i]));
            if (hv > 0.0f)
                draw::border(dl, ImVec2(omin.x - px(2.5f), omin.y - px(2.5f)), ImVec2(omin.x + sw + px(2.5f), omin.y + sw + px(2.5f)),
                             draw::with_alpha(IM_COL32(255, 255, 255, 110), hv), px(3.5f), draw::hairline());
            if (rs.pressed)
                picked = i;
        }
        if (picked) {
            std::swap(order[0], order[picked]);
            storage->SetInt(anim_key(id, 263), order[0]);
            storage->SetInt(anim_key(id, 263 + picked), order[picked]);
            *swapped = picked;
            *swap_anim = 0.0f;
            color = slot_color(order[0]);
        }
    }
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float gap = snap(px(k_picker_sep_gap));
        ImGui::ItemSize(ImVec2(w, gap * 2.0f));
        draw::hline(dl, p.x - px(k_picker_pad) + draw::hairline(), p.x + w + px(k_picker_pad) - draw::hairline(), p.y + gap,
                    col::window_separator);
    }

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float sv_h = snap(px(k_picker_sv_h)), gap = snap(px(k_picker_gap));
    const float hue_h = snap(px(k_picker_hue_h)), alpha_h = snap(px(k_picker_alpha_h));
    ImGui::ItemSize(ImVec2(w, sv_h + gap + hue_h + (style.alpha ? gap + alpha_h : 0.0f)));

    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(color->x, color->y, color->z, h, s, v);
    // Hue is undefined for greys; keep the last one so the square doesn't jump back to red.
    float* stored_h = storage->GetFloatRef(anim_key(id, 200), h);
    if (s > 0.0f && v > 0.0f)
        *stored_h = h;
    h = *stored_h;

    const float rounding = px(k_picker_sv_rounding);
    const ImVec2 sv_min(p.x, p.y), sv_max(p.x + w, p.y + sv_h);
    const ItemState sv = item_behavior(anim_key(id, 201), sv_min, sv_max);
    if (sv.held) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        s = ImSaturate((m.x - sv_min.x) / (sv_max.x - sv_min.x));
        v = 1.0f - ImSaturate((m.y - sv_min.y) / (sv_max.y - sv_min.y));
    }
    const float hue_r = hue_h * 0.5f;
    const ImVec2 hue_min(p.x, sv_max.y + gap), hue_max(p.x + w, sv_max.y + gap + hue_h);
    const ItemState hue = item_behavior(anim_key(id, 202), ImVec2(hue_min.x, hue_min.y - px(3.5f)),
                                        ImVec2(hue_max.x, hue_max.y + px(3.5f)));
    if (hue.held) {
        h = ImClamp((ImGui::GetIO().MousePos.x - hue_min.x - hue_r) / (hue_max.x - hue_min.x - 2.0f * hue_r), 0.0f, 0.9999f);
        *stored_h = h;
    }
    const float alpha_r = alpha_h * 0.5f;
    const ImVec2 alpha_min(p.x, hue_max.y + gap), alpha_max(p.x + w, hue_max.y + gap + alpha_h);
    const ItemState alpha = style.alpha ? item_behavior(anim_key(id, 203), ImVec2(alpha_min.x, alpha_min.y - px(4.0f)),
                                                        ImVec2(alpha_max.x, alpha_max.y + px(4.0f)))
                                        : ItemState{};
    if (alpha.held) {
        color->w = ImSaturate((ImGui::GetIO().MousePos.x - alpha_min.x - alpha_r) / (alpha_max.x - alpha_min.x - 2.0f * alpha_r));
        changed = true;
    }
    if (sv.held || hue.held) {
        ImGui::ColorConvertHSVtoRGB(h, s, v, color->x, color->y, color->z);
        changed = true;
    }
    // Knobs glide to new values (switching colors, typed values, palette) but follow the mouse exactly.
    const bool dragging = sv.held || hue.held || alpha.held;
    const auto smooth = [&](int k, float target) {
        float* cur = storage->GetFloatRef(anim_key(id, k), target);
        if (dragging || ImGui::IsWindowAppearing())
            *cur = target;
        else
            *cur = ImLerp(*cur, target, 1.0f - std::exp(-20.0f * dt));
        return *cur;
    };
    const float ds = smooth(270, s), dv = smooth(271, v), dh = smooth(272, h), da = smooth(273, color->w);

    // White to the hue across, then transparent to black downwards.
    int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(sv_min, sv_max, IM_COL32_WHITE, rounding);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, sv_min, ImVec2(sv_max.x, sv_min.y),
                                                   IM_COL32_WHITE, hue_color(h));
    v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(sv_min, sv_max, IM_COL32_BLACK, rounding);
    shade_alpha_v(dl, v0, dl->VtxBuffer.Size, sv_min.y, sv_max.y, 0.0f, 1.0f);
    const ImVec2 knob(ImLerp(sv_min.x, sv_max.x, ds), ImLerp(sv_max.y, sv_min.y, dv));
    const float ring_r = px(k_picker_ring_r);
    dl->PushClipRect(sv_min, sv_max, true);
    dl->AddCircle(knob, ring_r + px(0.9f), IM_COL32(0, 0, 0, 70), 32, px(1.0f));
    dl->AddCircle(knob, ring_r, IM_COL32(246, 246, 250, 255), 32, px(1.4f));
    dl->PopClipRect();

    // Hue bar: six segments. The rounded end ones reach under their neighbours so their
    // anti-aliased inner edge is covered by the square middle segments drawn after them.
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < 6; ++i) {
            const bool end = i == 0 || i == 5;
            if (end != (pass == 0))
                continue;
            const float x0 = ImLerp(hue_min.x, hue_max.x, static_cast<float>(i) / 6.0f);
            const float x1 = ImLerp(hue_min.x, hue_max.x, static_cast<float>(i + 1) / 6.0f);
            const float overlap = end ? px(2.0f) : 0.0f;
            const ImVec2 smin(i == 5 ? x0 - overlap : x0, hue_min.y), smax(i == 0 ? x1 + overlap : x1, hue_max.y);
            v0 = dl->VtxBuffer.Size;
            dl->AddRectFilled(smin, smax, IM_COL32_WHITE, end ? hue_r : 0.0f,
                              i == 0 ? ImDrawFlags_RoundCornersLeft : ImDrawFlags_RoundCornersRight);
            ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, ImVec2(x0, hue_min.y),
                                                           ImVec2(x1, hue_min.y), hue_color(i / 6.0f),
                                                           hue_color((i + 1) % 6 / 6.0f));
        }
    {
        // Upright pill knob filled with the hue.
        const float kx = ImLerp(hue_min.x + hue_r, hue_max.x - hue_r, dh);
        const float kw = snap(px(k_picker_hue_knob_w)), kh = snap(px(k_picker_hue_knob_h));
        const float cy = (hue_min.y + hue_max.y) * 0.5f;
        const ImVec2 kmin(snap(kx - kw * 0.5f), snap(cy - kh * 0.5f)), kmax(kmin.x + kw, kmin.y + kh);
        const float border = px(1.5f);
        dl->AddRectFilled(ImVec2(kmin.x - 1.0f, kmin.y - 1.0f), ImVec2(kmax.x + 1.0f, kmax.y + 1.0f), IM_COL32(0, 0, 0, 80),
                          kw * 0.5f + 1.0f);
        dl->AddRectFilled(kmin, kmax, IM_COL32(246, 246, 250, 255), kw * 0.5f);
        dl->AddRectFilled(ImVec2(kmin.x + border, kmin.y + border), ImVec2(kmax.x - border, kmax.y - border), hue_color(h),
                          kw * 0.5f - border);
    }

    // Alpha bar over a checkerboard.
    const ImU32 opaque = opaque_u32(*color);
    if (style.alpha) {
        checkerboard(dl, alpha_min, alpha_max, snap(px(k_picker_checker)), alpha_r);
        v0 = dl->VtxBuffer.Size;
        dl->AddRectFilled(alpha_min, alpha_max, opaque, alpha_r);
        shade_alpha_h(dl, v0, dl->VtxBuffer.Size, alpha_min.x + alpha_r, alpha_max.x - alpha_r, 0.0f, 1.0f);
        const ImVec2 kc(ImLerp(alpha_min.x + alpha_r, alpha_max.x - alpha_r, da), (alpha_min.y + alpha_max.y) * 0.5f);
        const float r = px(k_picker_alpha_knob_r);
        dl->AddCircleFilled(kc, r + 1.0f, IM_COL32(0, 0, 0, 80), 32);
        dl->AddCircleFilled(kc, r, IM_COL32(246, 246, 250, 255), 32);
        dl->AddCircleFilled(kc, r - px(1.6f), opaque, 32);
    }

    // Value row: mode, editable value, alpha in percent, saved colors.
    ImGui::ItemSize(ImVec2(w, snap(px(k_picker_row_gap))));
    const ImVec2 rp = ImGui::GetCursorScreenPos();
    const float row_h = snap(px(k_picker_row_h));
    ImGui::ItemSize(ImVec2(w, row_h));
    const float cy = rp.y + row_h * 0.5f;
    const float vsize = font_size::picker_value;
    // A local copy, written back after every change: the reference form dangled as soon
    // as another ImGuiStorage insert reallocated the vector, which is what made some
    // cham colour picks land on the wrong mode.
    const ImGuiID mode_key = anim_key(id, 220);
    int mode = ImClamp(storage->GetInt(mode_key, 0), 0, IM_ARRAYSIZE(k_picker_modes) - 1);

    {
        const ImGuiID mode_id = anim_key(id, 221);
        const char* name = k_picker_modes[mode];
        const float text_x = rp.x + px(k_picker_mode_text_x);
        const ImVec2 mmin(rp.x, rp.y), mmax(text_x + draw::text_width(font, vsize, name) + px(3.0f), rp.y + row_h);
        const ItemState ms = item_behavior(mode_id, mmin, mmax);
        const bool open = ImGui::IsPopupOpen(mode_id, ImGuiPopupFlags_None);
        if (ms.pressed && !open)
            ImGui::OpenPopup(mode_id);
        const float hl = animate(anim_key(id, 222), (ms.hovered || open) ? 1.0f : 0.0f, 18.0f);
        const ImU32 c = draw::lerp_color(col::picker_mode, col::tab_text_active, hl);
        draw::chevron_down(dl, ImVec2(rp.x + px(k_picker_mode_chevron_x), cy + px(0.3f)), px(2.6f), px(1.4f), c, px(1.3f));
        draw::text(dl, font, vsize, ImVec2(snap(text_x), cy - draw::cap_height(font, vsize) * 0.5f), c, name);
        if (begin_popup_id(mode_id, mmin, mmax, px(90.0f))) {
            for (int i = 0; i < IM_ARRAYSIZE(k_picker_modes); ++i) {
                ImGui::PushID(i);
                if (dropdown_item(k_picker_modes[i], i == mode, false)) {
                    mode = i;
                    storage->SetInt(mode_key, mode);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
            }
            end_popup();
        }
    }

    {
        // Value field; a click turns it into a text input.
        const float field_w = style.alpha ? k_picker_field_w : k_picker_alpha_box_x + k_picker_alpha_box_w - k_picker_field_x;
        const ImVec2 fmin(snap(rp.x + px(k_picker_field_x)), rp.y), fmax(fmin.x + snap(px(field_w)), rp.y + row_h);
        const ImGuiID field_id = anim_key(id, 230);
        int* editing = storage->GetIntRef(anim_key(id, 231), 0);
        const float sw = snap(px(8.0f));
        dl->AddRectFilled(fmin, fmax, col::picker_field_bg, px(4.0f));
        picker_swatch(dl, ImVec2(fmin.x + snap(px(3.5f)), snap(cy - sw * 0.5f)), sw, *color);
        float text_x = fmin.x + px(15.5f);
        if (mode == 0) {
            draw::text(dl, font, vsize, ImVec2(snap(text_x), cy - draw::cap_height(font, vsize) * 0.5f), col::picker_hash, "#");
            text_x += px(7.0f);
        }
        char buf[48];
        if (!*editing) {
            const ItemState fs = item_behavior(field_id, fmin, fmax);
            format_color(buf, sizeof(buf), *color, mode, style.alpha);
            draw::text(dl, font, vsize, ImVec2(snap(text_x), cy - draw::cap_height(font, vsize) * 0.5f),
                       fs.hovered ? col::tab_text_active : col::picker_value, buf);
            if (fs.pressed) {
                *editing = 1;
                storage->SetInt(anim_key(id, 232), 1); // focus on the next frame
            }
        } else {
            static char edit_buf[48];
            const bool first = storage->GetInt(anim_key(id, 232), 0) != 0;
            if (first) {
                format_color(edit_buf, sizeof(edit_buf), *color, mode, style.alpha);
                ImGui::SetKeyboardFocusHere();
                storage->SetInt(anim_key(id, 232), 0);
            }
            const ImVec2 backup = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(text_x - px(1.0f), fmin.y));
            ImGui::PushFont(font, draw::font_px(vsize) / ui_scale);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(px(1.0f), (row_h - draw::font_px(vsize)) * 0.5f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, col::tab_text_active);
            ImGui::SetNextItemWidth(fmax.x - text_x - px(2.0f));
            const bool enter = ImGui::InputText("##edit", edit_buf, sizeof(edit_buf),
                                                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
            const bool done = enter || (!first && !ImGui::IsItemActive());
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            ImGui::PopFont();
            ImGui::SetCursorScreenPos(backup);
            if (done) {
                ImVec4 parsed = *color;
                if (parse_color(edit_buf, mode, &parsed)) {
                    if (!style.alpha)
                        parsed.w = color->w;
                    *color = parsed;
                    changed = true;
                }
                *editing = 0;
            }
        }
    }

    if (style.alpha) {
        // Alpha in percent: drag sideways or scroll.
        const ImVec2 amin(snap(rp.x + px(k_picker_alpha_box_x)), rp.y), amax(amin.x + snap(px(k_picker_alpha_box_w)), rp.y + row_h);
        const ImGuiID box_id = anim_key(id, 240);
        const ItemState as = item_behavior(box_id, amin, amax);
        if (as.held && ImGui::GetIO().MouseDelta.x != 0.0f) {
            color->w = ImSaturate(color->w + ImGui::GetIO().MouseDelta.x * 0.005f / ui_scale);
            changed = true;
        }
        if (as.hovered && ImGui::GetIO().MouseWheel != 0.0f) {
            color->w = ImSaturate(std::round(color->w * 100.0f + ImGui::GetIO().MouseWheel) / 100.0f);
            changed = true;
        }
        if (as.hovered || as.held)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        dl->AddRectFilled(amin, amax, col::picker_field_bg, px(4.0f));
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(color->w * 100.0f + 0.5f));
        const float tw = draw::text_width(font, vsize, buf);
        draw::text(dl, font, vsize, ImVec2(snap((amin.x + amax.x - tw) * 0.5f), cy - draw::cap_height(font, vsize) * 0.5f),
                   (as.hovered || as.held) ? col::tab_text_active : col::picker_value, buf);
    }

    {
        // Saved colors: the button opens a grid of swatches with a "+" to save the current one.
        const ImVec2 c(snap(rp.x + px(k_picker_palette_cx)), cy);
        const float hit = px(7.0f);
        const ImGuiID pal_id = anim_key(id, 250);
        const ItemState ps = item_behavior(pal_id, ImVec2(c.x - hit, rp.y), ImVec2(c.x + hit, rp.y + row_h));
        const bool open = ImGui::IsPopupOpen(pal_id, ImGuiPopupFlags_None);
        if (ps.pressed && !open)
            ImGui::OpenPopup(pal_id);
        draw::any_icon(dl, c, (ps.hovered || open) ? col::tab_text_active : col::picker_value, icon::grid_plus);
        const int per_row = 6;
        const float cell = snap(px(16.0f)), cgap = snap(px(5.0f));
        const float grid_w = per_row * cell + (per_row - 1) * cgap;
        if (begin_popup_id(pal_id, ImVec2(c.x - px(60.0f), rp.y), ImVec2(c.x + hit, rp.y + row_h),
                           grid_w + 2.0f * px(k_popup_pad_x))) {
            ImDrawList* pdl = ImGui::GetWindowDrawList();
            const ImVec2 gp = ImGui::GetCursorScreenPos();
            const int n = g_palette.Size + 1;
            const int rows = (n + per_row - 1) / per_row;
            ImGui::ItemSize(ImVec2(grid_w, rows * cell + (rows - 1) * cgap + px(2.0f)));
            for (int i = 0; i < n; ++i) {
                const ImVec2 smin(gp.x + static_cast<float>(i % per_row) * (cell + cgap),
                                  gp.y + static_cast<float>(i / per_row) * (cell + cgap));
                const ImVec2 smax(smin.x + cell, smin.y + cell);
                const ItemState cs = item_behavior(anim_key(pal_id, 300 + i), smin, smax);
                if (i < g_palette.Size) {
                    pdl->AddRectFilled(smin, smax, opaque_u32(g_palette[i]), px(3.5f));
                    if (cs.pressed) {
                        *color = g_palette[i];
                        changed = true;
                    }
                } else {
                    pdl->AddRectFilled(smin, smax, cs.hovered ? col::window_chip_hovered : col::window_chip_bg, px(3.5f));
                    const ImVec2 pc((smin.x + smax.x) * 0.5f, (smin.y + smax.y) * 0.5f);
                    pdl->AddLine(ImVec2(pc.x - px(3.5f), pc.y), ImVec2(pc.x + px(3.5f), pc.y), col::window_label, px(1.4f));
                    pdl->AddLine(ImVec2(pc.x, pc.y - px(3.5f)), ImVec2(pc.x, pc.y + px(3.5f)), col::window_label, px(1.4f));
                    if (cs.pressed) {
                        if (g_palette.Size >= 18)
                            g_palette.erase(g_palette.begin());
                        g_palette.push_back(*color);
                    }
                }
                if (cs.hovered && i < g_palette.Size)
                    draw::border(pdl, ImVec2(smin.x - px(1.5f), smin.y - px(1.5f)), ImVec2(smax.x + px(1.5f), smax.y + px(1.5f)),
                                 IM_COL32(255, 255, 255, 90), px(4.5f), draw::hairline());
            }
            end_popup();
        }
    }

    if (style.saved_row && !g_palette.empty()) {
        // The saved colors, one click away under the value row.
        const float cell = snap(px(13.0f)), cgap = snap(px(5.0f));
        const int per_row = ImMax(1, static_cast<int>((w + cgap) / (cell + cgap)));
        const int shown = ImMin(g_palette.Size, per_row);
        ImGui::ItemSize(ImVec2(w, snap(px(8.0f))));
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        ImGui::ItemSize(ImVec2(w, cell));
        for (int i = 0; i < shown; ++i) {
            const ImVec4& saved = g_palette[g_palette.Size - shown + i];
            const ImVec2 smin(sp.x + static_cast<float>(i) * (cell + cgap), sp.y), smax(smin.x + cell, sp.y + cell);
            const ItemState cs = item_behavior(anim_key(id, 400 + i), smin, smax);
            const float hv = animate(anim_key(id, 430 + i), cs.hovered ? 1.0f : 0.0f, 18.0f);
            dl->AddRectFilled(smin, smax, opaque_u32(saved), px(3.0f));
            if (hv > 0.0f)
                draw::border(dl, ImVec2(smin.x - px(1.5f), smin.y - px(1.5f)), ImVec2(smax.x + px(1.5f), smax.y + px(1.5f)),
                             draw::with_alpha(IM_COL32(255, 255, 255, 110), hv), px(4.0f), draw::hairline());
            if (cs.pressed) {
                const float keep = color->w;
                *color = saved;
                if (!style.alpha)
                    color->w = keep;
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace

void glass(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImU32 fill, bool shadow,
           ImDrawFlags flags)
{
    // The frame is captured before the shadow is drawn, so the shadow doesn't darken the glass edges.
    const bool blurred = blur::available();
    if (blurred)
        blur::capture(dl);
    if (shadow) {
        dl->PushClipRectFullScreen();
        draw::soft_shadow(dl, min, max, rounding, px(12.0f), col::window_shadow);
        dl->PopClipRect();
    }
    if (blurred) {
        blur::image(dl, min, max, rounding, flags);
    } else {
        // Without the blurred frame the fill would let sharp details through; make it nearly opaque.
        const float a = ImMax(static_cast<float>((fill >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f, 0.01f);
        fill = draw::with_alpha(fill, ImMin(1.0f, a + 0.16f) / a);
    }
    dl->AddRectFilled(min, max, fill, rounding, flags);
}

void window_frame(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding)
{
    glass(dl, min, max, rounding, col::window_bg, true);
    draw::border(dl, min, max, col::window_border, rounding, draw::hairline());
}

bool begin_popup_at(ImGuiID id, const ImVec2& pos_in, float width, const ImVec2& padding, float rounding)
{
    const bool reveal = g_next_reveal, from_bottom = g_next_from_bottom;
    g_next_reveal = g_next_from_bottom = false;
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        return false;
    if (rounding < 0.0f)
        rounding = px(k_popup_rounding);

    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID size_key = anim_key(id, 100);
    const float last_h = storage->GetFloat(size_key, 0.0f);
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float margin = px(6.0f);
    width = snap(width);
    ImVec2 pos = pos_in;
    pos.x = ImClamp(pos.x, margin, ImMax(margin, display.x - margin - width));
    pos.y = ImClamp(pos.y, margin, ImMax(margin, display.y - margin - last_h));

    ImGui::SetNextWindowPos(ImVec2(std::floor(pos.x), std::floor(pos.y)));
    ImGui::SetNextWindowSize(ImVec2(width, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(snap(padding.x), snap(padding.y)));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
    const bool open = ImGui::BeginPopupEx(id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                                  ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
    if (!open)
        return false;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    storage->SetFloat(size_key, window->Size.y);
    ImGuiStorage* own = ImGui::GetStateStorage();
    float* alpha = own->GetFloatRef(anim_key(id, 101), 0.0f);
    if (ImGui::IsWindowAppearing())
        *alpha = 0.0f;
    *alpha = ImMin(1.0f, *alpha + ImGui::GetIO().DeltaTime * (reveal ? 7.0f : 9.0f));

    ImVec2 fmin = window->Pos, fmax(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y);
    if (reveal && *alpha < 1.0f) {
        // Dropdowns unroll from their anchor: the frame grows and clips the rows.
        const float e = 1.0f - (1.0f - *alpha) * (1.0f - *alpha) * (1.0f - *alpha);
        const float h = ImLerp(ImMin(px(24.0f), window->Size.y), window->Size.y, e);
        if (from_bottom)
            fmin.y = fmax.y - h;
        else
            fmax.y = fmin.y + h;
    }
    window_frame(window->DrawList, fmin, fmax, rounding);
    const bool clip = reveal && *alpha < 1.0f;
    if (clip)
        window->DrawList->PushClipRect(fmin, fmax, true);
    g_popups.push_back({ *alpha, clip });
    return true;
}

bool begin_popup_id(ImGuiID id, const ImVec2& anchor_min, const ImVec2& anchor_max, float width)
{
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        return false;
    // Open below the anchor; flip above it when the bottom of the display is too close.
    const float last_h = ImGui::GetStateStorage()->GetFloat(anim_key(id, 100), 0.0f);
    const float margin = px(6.0f);
    ImVec2 pos(anchor_min.x, anchor_max.y + px(k_popup_gap));
    bool above = false;
    if (last_h > 0.0f && pos.y + last_h > ImGui::GetIO().DisplaySize.y - margin) {
        pos.y = ImMax(margin, anchor_min.y - px(k_popup_gap) - last_h);
        above = true;
    }
    g_next_reveal = true;
    g_next_from_bottom = above;
    return begin_popup_at(id, pos, width, ImVec2(px(k_popup_pad_x), px(k_popup_pad_y)));
}

namespace {

// One line of a dropdown: the selected entry is bright and bold, the others dim until hovered.
bool dropdown_row(ImGuiID id, const char* label, const ImVec2& min, const ImVec2& max, bool selected, bool checkbox,
                  float alpha = 1.0f, const ImRect* hit = nullptr)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ItemState s = hit ? item_behavior(id, hit->Min, hit->Max) : item_behavior(id, min, max);
    const float h = animate(anim_key(id, 0), s.hovered ? 1.0f : 0.0f, 22.0f);
    const float sel = animate(anim_key(id, 1), selected ? 1.0f : 0.0f, 18.0f);
    if (h > 0.0f)
        dl->AddRectFilled(min, max, draw::with_alpha(col::popup_item_hover, h * alpha), px(5.0f));

    ImFont* font = sel > 0.5f ? fonts().semibold : fonts().medium;
    const float size = font_size::text;
    const float cy = (min.y + max.y) * 0.5f;
    const float slide = (h * 0.6f + sel * 0.4f) * px(2.0f);
    const ImU32 dim = rgb(146, 148, 160);
    const ImU32 text_col = draw::lerp_color(draw::lerp_color(dim, col::window_label, h), col::tab_text_active, sel);
    draw::text(dl, font, size, ImVec2(snap(min.x + px(8.0f) + slide), cy - draw::cap_height(font, size) * 0.5f),
               draw::with_alpha(text_col, alpha), label);
    if (checkbox && sel > 0.0f) {
        const ImVec2 mark(max.x - px(10.0f), cy);
        draw::check_mark(dl, mark, px(8.0f) * (0.6f + 0.4f * sel), draw::with_alpha(col::accent, sel * alpha), px(1.6f));
    }
    return s.pressed;
}

} // namespace

bool dropdown_item(const char* label, bool selected, bool checkbox)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(label);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float item_h = snap(px(k_dropdown_item_h));
    const float pad = snap(px(4.0f));
    ImGui::ItemSize(ImVec2(w, item_h));
    return dropdown_row(id, label, ImVec2(p.x - pad, p.y), ImVec2(p.x + w + pad, p.y + item_h), selected, checkbox);
}

int dropdown_list(std::span<const char* const> items, int current, const bool* checked)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    ImDrawList* dl = window->DrawList;
    const ImGuiID id = window->GetID("##list");
    const int n = static_cast<int>(items.size());
    const float item_h = snap(px(k_dropdown_item_h));
    const int visible = ImMin(n, k_dropdown_visible);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float list_h = item_h * static_cast<float>(visible);
    const float total_h = item_h * static_cast<float>(n);
    const float max_scroll = ImMax(0.0f, total_h - list_h);
    ImGui::ItemSize(ImVec2(w, list_h));

    float* target = storage->GetFloatRef(anim_key(id, 0), 0.0f);
    float* scroll = storage->GetFloatRef(anim_key(id, 1), 0.0f);
    if (ImGui::IsWindowAppearing()) {
        // Open with the selected entry in view, in the middle of the list when possible.
        *target = current >= 0 ? ImClamp(item_h * (static_cast<float>(current) + 0.5f) - list_h * 0.5f, 0.0f, max_scroll) : 0.0f;
        *scroll = *target;
    }
    const ImVec2 lmin(p.x - snap(px(4.0f)), p.y), lmax(p.x + w + snap(px(4.0f)), p.y + list_h);
    const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(lmin, ImVec2(lmax.x + px(8.0f), lmax.y));
    if (hovered && max_scroll > 0.0f && ImGui::GetIO().MouseWheel != 0.0f)
        *target -= ImGui::GetIO().MouseWheel * item_h * 1.5f;

    // Scrollbar thumb, draggable.
    const float bar_w = snap(px(k_scrollbar_w));
    const float bar_x = lmax.x + snap(px(3.0f));
    const float thumb_h = max_scroll > 0.0f ? ImMax(px(18.0f), list_h * list_h / total_h) : 0.0f;
    if (max_scroll > 0.0f) {
        const float thumb_y = p.y + (list_h - thumb_h) * (*scroll / max_scroll);
        const ImGuiID thumb_id = anim_key(id, 2);
        const ItemState ts = item_behavior(thumb_id, ImVec2(bar_x - px(3.0f), p.y), ImVec2(bar_x + bar_w + px(3.0f), p.y + list_h));
        if (ts.held)
            *target += ImGui::GetIO().MouseDelta.y * max_scroll / ImMax(1.0f, list_h - thumb_h);
        const float th = animate(anim_key(id, 3), (ts.hovered || ts.held || hovered) ? 1.0f : 0.0f, 14.0f);
        dl->AddRectFilled(ImVec2(bar_x, p.y), ImVec2(bar_x + bar_w, p.y + list_h), IM_COL32(255, 255, 255, 8), bar_w * 0.5f);
        dl->AddRectFilled(ImVec2(bar_x, thumb_y), ImVec2(bar_x + bar_w, thumb_y + thumb_h),
                          draw::with_alpha(IM_COL32(255, 255, 255, 255), 0.22f + 0.18f * th), bar_w * 0.5f);
    }
    *target = ImClamp(*target, 0.0f, max_scroll);
    *scroll = ImLerp(*scroll, *target, 1.0f - std::exp(-18.0f * ImGui::GetIO().DeltaTime));
    if (std::fabs(*scroll - *target) < 0.25f)
        *scroll = *target;

    int clicked = -1;
    dl->PushClipRect(lmin, lmax, true);
    for (int i = 0; i < n; ++i) {
        const float y = p.y + item_h * static_cast<float>(i) - *scroll;
        if (y + item_h < p.y || y > p.y + list_h)
            continue;
        // Rows half outside the list fade, so the list edge reads as a soft cut.
        const float edge = ImMin(y + item_h - p.y, p.y + list_h - y) / item_h;
        const ImRect hit(ImVec2(lmin.x, ImMax(y, p.y)), ImVec2(lmax.x, ImMin(y + item_h, p.y + list_h)));
        ImGui::PushID(i);
        const bool sel = checked ? checked[i] : i == current;
        if (dropdown_row(window->GetID("##row"), items[static_cast<size_t>(i)], ImVec2(lmin.x, y), ImVec2(lmax.x, y + item_h), sel,
                         checked != nullptr, ImSaturate(edge * 1.5f), &hit))
            clicked = i;
        ImGui::PopID();
    }
    dl->PopClipRect();
    return clicked;
}

namespace {

// Combo frame shared by the single and multi-select boxes.
// The value text slides in from below when it changes; `old_text` leaves upwards.
void draw_combo_frame(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float highlight, float open,
                      const char* text, bool enabled, const char* old_text = nullptr, float change = 1.0f)
{
    const float alpha = enabled ? 1.0f : disabled_alpha;
    dl->AddRectFilled(min, max, draw::lerp_color(col::combo_bg, col::combo_bg_hovered, highlight),
                      px(layout::multi_rounding));

    ImFont* font = fonts().medium;
    const float size = font_size::combo;
    const float cy = (min.y + max.y) * 0.5f;
    const float chevron_x = max.x - px(k_combo_chevron_pad);
    const float cap_y = cy - draw::cap_height(font, size) * 0.5f + px(layout::combo_text_dy);
    const float pen_x = min.x + px(k_combo_text_pad);
    const float fade_to = chevron_x - px(7.2f);
    const ImU32 text_col =
        draw::with_alpha(draw::lerp_color(col::combo_text, col::tab_text_active, highlight * 0.5f), alpha);
    const auto put = [&](const char* s, float dy, float a) {
        const ImU32 c = draw::with_alpha(text_col, a);
        if (pen_x + draw::text_width(font, size, s) > fade_to)
            draw::text_fade_right(dl, font, size, ImVec2(pen_x, cap_y + dy), c, s, fade_to - px(17.0f), fade_to);
        else
            draw::text(dl, font, size, ImVec2(pen_x, cap_y + dy), c, s);
    };
    if (old_text && change < 1.0f) {
        const float e = 1.0f - (1.0f - change) * (1.0f - change);
        dl->PushClipRect(min, max, true);
        put(old_text, -e * px(8.0f), 1.0f - e);
        put(text, (1.0f - e) * px(8.0f), e);
        dl->PopClipRect();
    } else {
        put(text, 0.0f, 1.0f);
    }

    // The chevron turns over while the list opens.
    const ImU32 chevron_col =
        draw::with_alpha(draw::lerp_color(col::combo_chevron, col::tab_text_active, highlight * 0.5f), alpha);
    const float flip = ImLerp(1.0f, -1.0f, open);
    const float hw = px(2.9f), hh = px(1.5f) * flip;
    const ImVec2 c(chevron_x, cy + px(layout::combo_chevron_dy) * (1.0f - open));
    const ImVec2 pts[3] = { { c.x - hw, c.y - hh }, { c.x, c.y + hh }, { c.x + hw, c.y - hh } };
    dl->AddPolyline(pts, 3, chevron_col, px(1.4f));
}

} // namespace

float animate(ImGuiID id, float target, float speed)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float* value = storage->GetFloatRef(id, FLT_MAX);
    if (*value == FLT_MAX) {
        *value = target;
        return target;
    }
    *value = ImLerp(*value, target, 1.0f - std::exp(-speed * ImGui::GetIO().DeltaTime));
    if (std::fabs(*value - target) < 0.002f)
        *value = target;
    return *value;
}

// One submission per (window, ID) per frame, plus a diagnostic line whenever a duplicate
// shows up: the message names the window and the ID so the offending widget can be found
// in the log instead of guessed at from a modal box.
bool claim_item_id(ImGuiID id)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    static std::vector<std::pair<ImGuiID, ImGuiID>> seen{};
    static int frame = -1;
    const int current = ImGui::GetFrameCount();
    if (current != frame) {
        frame = current;
        seen.clear();
    }
    const ImGuiID window_id = window ? window->ID : 0;
    const auto key = std::make_pair(window_id, id);
    if (std::find(seen.begin(), seen.end(), key) != seen.end()) {
        static int reported = 0;
        if (reported++ < 40)
            debug_log_id_conflict(window ? window->Name : "NULL", static_cast<unsigned>(id));
        return false;
    }
    seen.push_back(key);
    return true;
}

void debug_log_id_conflict(const char* window, unsigned id)
{
#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
    static std::mutex guard{};
    const std::scoped_lock lock{ guard };
    char path[512]{};
    if (!GetTempPathA(sizeof(path), path))
        return;
    std::string file = std::string{ path } + "vesta\\imgui_ids.log";
    CreateDirectoryA((std::string{ path } + "vesta").c_str(), nullptr);
    if (FILE* handle = std::fopen(file.c_str(), "a")) {
        std::fprintf(handle, "window=%s id=0x%08X\n", window ? window : "NULL", id);
        std::fclose(handle);
    }
#else
    static_cast<void>(window);
    static_cast<void>(id);
#endif
}
ItemState item_behavior(ImGuiID id, const ImVec2& min, const ImVec2& max, bool enabled)
{
    ItemState s;
    // ImGui still claims hover for disabled items, which would block items drawn over them later.
    enabled = enabled && (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled) == 0;
    const ImRect bb(min, max);

    // Two visible items sharing one ID is a programmer error that ImGui answers with a
    // modal box over the game ("N visible items with conflicting ID"). The menu draws a
    // lot of hand-rolled rows, so the guard lives here: the duplicate is refused and
    // written to the log, which keeps the overlay usable while the cause is hunted down.
    if (!claim_item_id(id))
        return s;

    if (!ImGui::ItemAdd(bb, id, nullptr, enabled ? ImGuiItemFlags_None : ImGuiItemFlags_Disabled))
        return s;
    if (enabled)
        s.pressed = ImGui::ButtonBehavior(bb, id, &s.hovered, &s.held);
    return s;
}

ImVec2 toggle_size(float scale)
{
    return ImVec2(snap(px(layout::toggle_w * scale)), snap(px(layout::toggle_h * scale)));
}

ImVec2 combo_size() { return ImVec2(snap(px(layout::multi_w)), snap(px(layout::multi_h))); }

bool toggle(const char* str_id, bool* v, const ImVec2& pos, bool enabled, float scale)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ImVec2 size = toggle_size(scale);
    const ImVec2 min(snap(pos.x), snap(pos.y));
    const ImVec2 max(min.x + size.x, min.y + size.y);
    const float hit = px(3.0f);
    const ItemState s = item_behavior(id, ImVec2(min.x - hit, min.y - hit), ImVec2(max.x + hit, max.y + hit), enabled);
    if (s.pressed) {
        *v = !*v;
        ImGui::MarkItemEdited(id);
    }

    const float t = animate(anim_key(id, 0), *v ? 1.0f : 0.0f, 20.0f);
    const float h = animate(anim_key(id, 1), s.hovered ? 1.0f : 0.0f, 16.0f);
    const float alpha = enabled ? 1.0f : disabled_alpha;
    const float r = (max.y - min.y) * 0.5f;
    const float ring = px(1.0f);

    ImDrawList* dl = window->DrawList;
    const ImU32 outline = draw::with_alpha(col::toggle_outline, alpha);
    dl->AddRectFilled(ImVec2(min.x - ring, min.y - ring), ImVec2(max.x + ring, max.y + ring), outline, r + ring);
    ImU32 track = draw::lerp_color(col::toggle_off_track, col::accent, t);
    track = draw::lerp_color(track, IM_COL32_WHITE, h * 0.07f);
    dl->AddRectFilled(min, max, enabled ? track : draw::lerp_color(col::panel_bg, track, 0.6f), r);

    const ImVec2 knob_c(ImLerp(min.x + px(layout::knob_inset_off * scale), max.x - px(layout::knob_inset_on * scale), t),
                        min.y + r);
    const float knob_r = ImLerp(px(layout::knob_r * scale), px(layout::knob_r_on * scale), t);
    ImU32 knob = draw::lerp_color(col::toggle_off_knob, col::toggle_on_knob, t);
    knob = draw::lerp_color(knob, IM_COL32_WHITE, h * 0.2f);
    if (!enabled)
        knob = draw::lerp_color(col::toggle_off_track, knob, alpha + 0.1f);
    // The ring separates the grey knob from the dark track; on the light track it disappears.
    dl->AddCircleFilled(knob_c, knob_r + ring, draw::with_alpha(outline, 1.0f - t), 48);
    dl->AddCircleFilled(knob_c, knob_r, knob, 48);
    return s.pressed;
}

bool dots_button(const char* str_id, const ImVec2& center, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const float hit = px(8.0f);
    const ItemState s = item_behavior(id, ImVec2(center.x - hit, center.y - hit), ImVec2(center.x + hit, center.y + hit),
                                      enabled);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    const float h = animate(anim_key(id, 0), (s.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const ImU32 c = draw::lerp_color(col::dots, col::accent, h);
    draw::dots(window->DrawList, center, enabled ? c : draw::lerp_color(col::panel_bg, c, disabled_alpha));
    return s.pressed;
}

bool color_swatch(const char* str_id, ImVec4* color, const ImVec2& min, float size, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ImVec2 max(min.x + size, min.y + size);
    swatch_face(id, color, min, max, enabled);

    bool changed = false;
    const float width = px(k_picker_w);
    if (begin_popup_at(id, ImVec2(max.x + px(10.0f), min.y - px(40.0f)), width,
                       ImVec2(px(k_picker_pad), px(k_picker_pad)), px(8.0f))) {
        changed = color_picker(id, color, PickerStyle{});
        end_popup();
    }
    return changed;
}

void swatch_face(ImGuiID id, const ImVec4* color, const ImVec2& min, const ImVec2& max, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ItemState s = item_behavior(id, min, max, enabled);
    if (s.pressed)
        ImGui::OpenPopup(id);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    const float h = animate(anim_key(id, 0), (s.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const float alpha = enabled ? 1.0f : disabled_alpha;

    ImDrawList* dl = window->DrawList;
    ImVec4 opaque = *color;
    opaque.w = 1.0f;
    const ImU32 base = ImGui::ColorConvertFloat4ToU32(opaque);
    // The reference swatch is slightly warmer on the left and cooler on the right.
    auto shift = [base, alpha](int dr, int dg, int db) {
        auto ch = [base](int shift_bits, int d) {
            return static_cast<ImU32>(ImClamp(static_cast<int>((base >> shift_bits) & 0xFF) + d, 0, 255)) << shift_bits;
        };
        return draw::with_alpha(ch(IM_COL32_R_SHIFT, dr) | ch(IM_COL32_G_SHIFT, dg) | ch(IM_COL32_B_SHIFT, db) |
                                    IM_COL32_A_MASK,
                                alpha);
    };
    const float rounding = px(layout::swatch_rounding);
    const float ring = px(1.0f);
    dl->AddRectFilled(ImVec2(min.x - ring, min.y - ring), ImVec2(max.x + ring, max.y + ring),
                      draw::with_alpha(col::toggle_outline, alpha), rounding + ring);
    const int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(min, max, IM_COL32_WHITE, rounding);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, min, ImVec2(max.x, min.y),
                                                   shift(0, 1, -10), shift(0, -3, 10));
    if (h > 0.0f) {
        const float gap = snap(px(2.0f));
        draw::border(dl, ImVec2(min.x - gap, min.y - gap), ImVec2(max.x + gap, max.y + gap),
                     draw::with_alpha(IM_COL32(255, 255, 255, 90), h), rounding + gap, draw::hairline());
    }
}

bool color_picker_embed(const char* str_id, ImVec4* color) { return color_picker(ImGui::GetID(str_id), color, PickerStyle{}); }
bool color_picker_embed(const char* str_id, ImVec4* color, const PickerStyle& style)
{
    return color_picker(ImGui::GetID(str_id), color, style);
}

bool combo_box(const char* str_id, const ImVec2& min, const ImVec2& max, std::span<const char* const> items,
               int* current, bool enabled)
{
    if (items.empty() || !current)
        return false;




    const int last = static_cast<int>(items.size()) - 1;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ItemState s = item_behavior(id, min, max, enabled);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    if (s.pressed && !open)
        ImGui::OpenPopup(id);
    const float h = animate(anim_key(id, 0), (s.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const float o = animate(anim_key(id, 4), open ? 1.0f : 0.0f, 16.0f);
    *current = ImClamp(*current, 0, last);
    ImGuiStorage* storage = ImGui::GetStateStorage();
    // Plain Get/Set instead of GetIntRef/GetFloatRef: those hand back pointers into the
    // storage vector, and the second insert can reallocate it, leaving the first pointer
    // dangling. Writing through it corrupted the heap and faulted inside the CRT.
    int shown = ImClamp(storage->GetInt(anim_key(id, 5), *current), 0, last);
    int previous = ImClamp(storage->GetInt(anim_key(id, 6), *current), 0, last);
    float change = storage->GetFloat(anim_key(id, 7), 1.0f);
    if (shown != *current) {
        previous = ImClamp(shown, 0, last);
        shown = *current;
        change = 0.0f;
    }
    change = ImMin(1.0f, change + ImGui::GetIO().DeltaTime * 6.0f);
    storage->SetInt(anim_key(id, 5), shown);
    storage->SetInt(anim_key(id, 6), previous);
    storage->SetFloat(anim_key(id, 7), change);
    draw_combo_frame(window->DrawList, min, max, h, o, items[static_cast<size_t>(*current)], enabled,
                     items[static_cast<size_t>(previous)], change);

    bool changed = false;
    if (begin_popup_id(id, min, max, ImMax(max.x - min.x, px(130.0f)))) {
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

bool multi_combo_box(const char* str_id, const ImVec2& min, const ImVec2& max, std::span<const char* const> items,
                     bool* selected, const char* placeholder, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ItemState s = item_behavior(id, min, max, enabled);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    if (s.pressed && !open)
        ImGui::OpenPopup(id);
    const float h = animate(anim_key(id, 0), (s.hovered || open) ? 1.0f : 0.0f, 18.0f);
    const float o = animate(anim_key(id, 4), open ? 1.0f : 0.0f, 16.0f);
    draw_combo_frame(window->DrawList, min, max, h, o, join_selected(items, selected, placeholder).c_str(), enabled);

    bool changed = false;
    if (begin_popup_id(id, min, max, ImMax(max.x - min.x, px(150.0f)))) {
        const int i = dropdown_list(items, -1, selected);
        if (i >= 0) {
            selected[i] = !selected[i];
            changed = true;
        }
        end_popup();
    }
    return changed;
}

namespace {

// Maps the mouse x between x0 and x1 onto [lo, hi] while `s` is held, in multiples of `step`.
bool drag_value(ImGuiID id, const ItemState& s, float x0, float x1, float* v, float lo, float hi, float step)
{
    if (!s.held)
        return false;
    const float t = ImSaturate((ImGui::GetIO().MousePos.x - x0) / ImMax(x1 - x0, 1.0f));
    float nv = lo + (hi - lo) * t;
    if (step > 0.0f)
        nv = lo + std::round((nv - lo) / step) * step;
    nv = ImClamp(nv, ImMin(lo, hi), ImMax(lo, hi));
    if (nv == *v)
        return false;
    *v = nv;
    ImGui::MarkItemEdited(id);
    return true;
}

// Wheel over a slider (track or value box) steps its value. Shift multiplies the step,
// Alt divides it, so the same widget covers coarse and fine tuning.
bool step_by_wheel(ImGuiID id, bool hovered, float* v, float lo, float hi, float step, bool enabled)
{
    if (!enabled || !hovered)
        return false;
    const float wheel = ImGui::GetIO().MouseWheel;
    if (wheel == 0.0f)
        return false;
    const ImGuiIO& io = ImGui::GetIO();
    float delta = step > 0.0f ? step : (hi - lo) / 100.0f;
    if (io.KeyShift)
        delta *= 5.0f;
    else if (io.KeyAlt)
        delta *= 0.2f;
    const float next = ImClamp(*v + wheel * delta,
                               ImMin(lo, hi), ImMax(lo, hi));
    const float snapped = step > 0.0f ? lo + std::round((next - lo) / step) * step : next;
    const float clamped = ImClamp(snapped, ImMin(lo, hi), ImMax(lo, hi));
    if (clamped == *v)
        return false;
    *v = clamped;
    ImGui::MarkItemEdited(id);
    return true;
}

} // namespace

void value_tooltip(const char* text)
{
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImFont* font = fonts().semibold;
    const float size = font_size::popup_label;
    const float w = snap(draw::text_width(font, size, text) + px(14.0f)), h = snap(px(21.0f));
    const ImVec2 m = ImGui::GetIO().MousePos;
    ImVec2 max(snap(m.x - px(2.0f)), snap(m.y - px(1.0f)));
    ImVec2 min(max.x - w, max.y - h);
    if (min.x < px(4.0f)) {
        min.x = snap(m.x + px(14.0f));
        max.x = min.x + w;
    }
    const float rounding = px(5.0f);
    dl->AddRectFilled(min, max, col::tooltip_bg, rounding);
    draw::border(dl, min, max, col::window_border, rounding, draw::hairline());
    draw::text(dl, font, size, ImVec2(min.x + px(7.0f), min.y + (h - draw::cap_height(font, size)) * 0.5f),
               col::tooltip_text, text);
}

bool text_field(const char* str_id, char* buf, int size, const ImVec2& min, const ImVec2& max,
                const char* placeholder, bool focus, ImU32 text_col, ImGuiInputTextFlags flags)
{
    const Fonts& f = fonts();
    const float h = max.y - min.y;
    const float font_px = draw::font_px(font_size::combo);
    ImGui::PushFont(f.medium, font_px / ui_scale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, std::floor((h - font_px) * 0.5f)));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, text_col ? text_col : col::tab_text_active);
    ImGui::SetCursorScreenPos(min);
    ImGui::SetNextItemWidth(max.x - min.x);
    if (focus)
        ImGui::SetKeyboardFocusHere();
    const bool changed = ImGui::InputText(str_id, buf, static_cast<size_t>(size), flags);
    const bool active = ImGui::IsItemActive();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    ImGui::PopFont();
    if (!buf[0] && !active && placeholder)
        draw::text(ImGui::GetWindowDrawList(), f.medium, font_size::combo,
                   ImVec2(min.x, (min.y + max.y) * 0.5f - draw::cap_height(f.medium, font_size::combo) * 0.5f),
                   col::window_value, placeholder);
    return changed;
}

bool slider(const char* str_id, float x0, float right, float cy, float* v, float lo, float hi, const char* format,
            float step, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    ImDrawList* dl = window->DrawList;

    char buf[32];
    std::snprintf(buf, sizeof(buf), format ? format : "%.0f", *v);
    ImFont* font = fonts().medium;
    const float size = font_size::value;
    const float text_w = draw::text_width(font, size, buf);
    // The box fits its text but keeps its width while dragging, so the track doesn't move under the cursor.
    float* box_w = ImGui::GetStateStorage()->GetFloatRef(anim_key(id, 3), 0.0f);
    if (ImGui::GetActiveID() != id || *box_w == 0.0f)
        *box_w = ImMax(snap(text_w + 2.0f * px(layout::slider_box_pad)), snap(px(layout::slider_box_min_w)));
    const float box_h = snap(px(layout::slider_box_h));
    const ImVec2 box_max(snap(right), snap(cy - box_h * 0.5f) + box_h);
    const ImVec2 box_min(box_max.x - *box_w, box_max.y - box_h);

    const float track_h = ImMax(2.0f, snap(px(layout::slider_track_h)));
    const float r = px(layout::slider_knob_r);
    const ImVec2 track_min(snap(x0), snap(cy - track_h * 0.5f));
    const ImVec2 track_max(snap(box_min.x - px(layout::slider_box_gap)), track_min.y + track_h);
    const float tcy = track_min.y + track_h * 0.5f;
    // The knob stays inside the track: its edge touches the ends at the extremes.
    const float k0 = track_min.x + r, k1 = track_max.x - r;

    const ItemState s = item_behavior(id, ImVec2(track_min.x - px(2.0f), cy - px(9.0f)),
                                      ImVec2(track_max.x + px(2.0f), cy + px(9.0f)), enabled);
    // The value box is part of the control even though it sits outside the track's item.
    const bool box_hovered = ImGui::IsMouseHoveringRect(box_min, box_max) && ImGui::IsWindowHovered();
    const bool changed = drag_value(id, s, k0, k1, v, lo, hi, step) | step_by_wheel(id, s.hovered || s.held || box_hovered, v, lo, hi, step, enabled);
    const float t = animate(anim_key(id, 0), ImSaturate((*v - lo) / (hi - lo)), 24.0f);
    const float h = animate(anim_key(id, 1), (s.hovered || s.held || box_hovered) ? 1.0f : 0.0f, 18.0f);
    const float alpha = enabled ? 1.0f : disabled_alpha;
    const float kx = ImLerp(k0, k1, t);

    dl->AddRectFilled(track_min, track_max, draw::with_alpha(col::slider_track, alpha), track_h * 0.5f);
    dl->AddRectFilled(track_min, ImVec2(ImMax(kx, track_min.x + track_h), track_max.y),
                      draw::with_alpha(col::accent, alpha), track_h * 0.5f);
    dl->AddCircleFilled(ImVec2(kx, tcy), r + px(0.5f) * h, draw::with_alpha(col::slider_knob, alpha), 32);

    const float rounding = px(layout::slider_box_rounding);
    dl->AddRectFilled(box_min, box_max, draw::with_alpha(col::combo_bg, alpha), rounding);
    draw::border(dl, box_min, box_max, draw::with_alpha(col::slider_box_border, alpha), rounding, draw::hairline());
    const ImU32 c = draw::with_alpha(draw::lerp_color(col::slider_value, col::tab_text_active, h * 0.5f), alpha);
    draw::text(dl, font, size,
               ImVec2(box_min.x + (*box_w - text_w) * 0.5f, cy - draw::cap_height(font, size) * 0.5f), c, buf);
    return changed;
}

bool bar_slider(const char* str_id, const ImVec2& min, const ImVec2& max, float* v, float lo, float hi,
                const char* format, float step, bool enabled)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(str_id);
    const ItemState s = item_behavior(id, ImVec2(min.x - px(3.0f), min.y), ImVec2(max.x + px(3.0f), max.y), enabled);
    const bool changed = drag_value(id, s, min.x, max.x, v, lo, hi, step) | step_by_wheel(id, s.hovered || s.held, v, lo, hi, step, enabled);
    const float t = animate(anim_key(id, 0), ImSaturate((*v - lo) / (hi - lo)), 24.0f);
    const float h = animate(anim_key(id, 1), (s.hovered || s.held) ? 1.0f : 0.0f, 18.0f);
    const float alpha = enabled ? 1.0f : disabled_alpha;

    ImDrawList* dl = window->DrawList;
    const float bh = ImMax(2.0f, snap(px(layout::bar_slider_h)));
    const ImVec2 bmin(snap(min.x), snap((min.y + max.y - bh) * 0.5f)), bmax(snap(max.x), bmin.y + bh);
    dl->AddRectFilled(bmin, bmax, draw::with_alpha(col::slider_track, alpha), bh * 0.5f);
    const float x = ImLerp(bmin.x, bmax.x, t);
    if (x > bmin.x + 0.5f) {
        const ImU32 fill = draw::lerp_color(col::accent, IM_COL32_WHITE, h * 0.25f);
        dl->AddRectFilled(bmin, ImVec2(ImMax(x, bmin.x + bh), bmax.y), draw::with_alpha(fill, alpha), bh * 0.5f);
    }
    if (format && (s.hovered || s.held)) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), format, *v);
        value_tooltip(buf);
    }
    return changed;
}

bool begin_popup(const char* str_id, const ImVec2& anchor_min, const ImVec2& anchor_max, float width)
{
    return begin_popup_id(ImGui::GetID(str_id), anchor_min, anchor_max, width);
}

void end_popup()
{
    // A popup that ImGui closed on its own must not pop an empty frame stack: indexing
    // it is undefined behaviour and was a plausible source of the crash reported when
    // opening a settings window.
    if (g_popups.empty()) {
        ImGui::EndPopup();
        return;
    }
    const PopupFrame frame = g_popups.back();
    g_popups.pop_back();
    if (frame.clip)
        ImGui::GetWindowDrawList()->PopClipRect();
    if (frame.alpha < 1.0f) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float ease = 1.0f - (1.0f - frame.alpha) * (1.0f - frame.alpha);
        const float slide = (1.0f - ease) * -px(4.0f);
        for (ImDrawVert& v : dl->VtxBuffer) {
            v.col = draw::with_alpha(v.col, ease);
            v.pos.y += slide;
        }
    }
    ImGui::EndPopup();
}

} // namespace nl::ui