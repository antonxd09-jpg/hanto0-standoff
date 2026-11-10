// Menu source watermark: @use_sberbank
#include <algorithm>
#include <cmath>
#include <vector>

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/esp_model.h"
#include "menu/fonts.h"
#include "menu/menu.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;

constexpr float k_drag_threshold = 5.0f;
constexpr float k_zone = 56.0f;   // drop zones reach this far out of the box
constexpr float k_side_gap = 4.0f; // box -> first element
constexpr float k_text_size = 11.0f;
constexpr float k_flag_size = 9.6f;
constexpr const char* k_side_names[] = { "Left", "Top", "Right", "Bottom" };
constexpr const char* k_text_styles[] = { "AaA", "AAA", "aaa" };
constexpr const char* k_bar_styles[] = { "Normal", "Static Shade", "Gradient" };
constexpr const char* k_font_weights[] = { "Regular", "Medium", "SemiBold" };

bool stacked(EspKind k) { return k != EspKind::box && k != EspKind::skeleton && k != EspKind::hitboxes; }

// Weight of an element: the editor offers three, the flag rows default to the heavier one.
ImFont* element_font(EspKind k, int weight)
{
    switch (weight) {
    case 0: return fonts().medium;
    case 2: return fonts().black;
    default: break;
    }
    return k == EspKind::flag ? fonts().semibold : fonts().medium;
}
float element_size(EspKind k) { return k == EspKind::flag ? k_flag_size : k_text_size; }

ImU32 u32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

// Text elements render their sample through this so the per-element style (as-is,
// UPPER, lower) is visible in the preview exactly as it will be in game.
std::string element_text(const EspInfo& info, const EspElement& e)
{
    std::string text = info.sample ? info.sample : "";
    switch (e.text_style) {
    case EspText::upper:
        for (char& c : text)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        break;
    case EspText::lower:
        for (char& c : text)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        break;
    default: break;
    }
    return text;
}

// Size of an element on its side; bars span the box edge.
ImVec2 element_extent(const EspInfo& info, const EspElement& e, const ImVec2& box_min, const ImVec2& box_max)
{
    const bool vertical = e.side == EspSide::left || e.side == EspSide::right;
    const float scale = ImClamp(e.scale, 0.5f, 2.5f);
    switch (info.kind) {
    case EspKind::bar:
        return vertical ? ImVec2(snap(px(2.0f * scale)), box_max.y - box_min.y)
                        : ImVec2(box_max.x - box_min.x, snap(px(2.0f * scale)));
    case EspKind::icon: return ImVec2(snap(px(13.0f * scale)), snap(px(13.0f * scale)));
    default: {
        ImFont* font = element_font(info.kind, e.font);
        const float size = element_size(info.kind) * scale;
        const std::string text = element_text(info, e);
        return ImVec2(snap(draw::text_width(font, size, text.c_str()) + px(2.0f)),
                      snap(draw::cap_height(font, size) + px(5.0f)));
    }
    }
}

void draw_skeleton(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 c, float t)
{
    // With the live model in the preview the skeleton follows the real animated
    // bones; the placeholder below only covers the shipped picture.
    if (esp_model::frame_ready()) {
        const float w = max.x - min.x, h = max.y - min.y;
        const int segments = esp_model::skeleton_segments();
        for (int i = 0; i < segments; ++i) {
            float x0, y0, x1, y1;
            if (!esp_model::skeleton_segment(i, &x0, &y0, &x1, &y1))
                continue;
            dl->AddLine(ImVec2(min.x + x0 * w, min.y + y0 * h), ImVec2(min.x + x1 * w, min.y + y1 * h), c, t);
        }
        return;
    }
    const auto p = [&](float x, float y) { return ImVec2(ImLerp(min.x, max.x, x), ImLerp(min.y, max.y, y)); };
    const ImVec2 head = p(0.5f, 0.09f), neck = p(0.5f, 0.17f), pelvis = p(0.5f, 0.5f);
    const ImVec2 sl = p(0.33f, 0.21f), sr = p(0.67f, 0.21f), el = p(0.24f, 0.36f), er = p(0.76f, 0.36f);
    const ImVec2 hl = p(0.22f, 0.49f), hr = p(0.78f, 0.49f), kl = p(0.41f, 0.72f), kr = p(0.59f, 0.72f);
    const ImVec2 fl = p(0.39f, 0.96f), fr = p(0.61f, 0.96f);
    const ImVec2 lines[][2] = { { head, neck }, { neck, pelvis }, { neck, sl }, { neck, sr }, { sl, el }, { sr, er },
                                { el, hl },     { er, hr },       { pelvis, kl }, { pelvis, kr }, { kl, fl }, { kr, fr } };
    for (const auto& l : lines)
        dl->AddLine(l[0], l[1], c, t);
}

void draw_hitboxes(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 c)
{
    const float w = max.x - min.x, h = max.y - min.y;
    const ImU32 fill = draw::with_alpha(c, 0.22f), line = draw::with_alpha(c, 0.8f);

    // Live model: the capsules come from the projected hitbox bone sets, which is
    // what the in-game ESP draws as well.
    if (esp_model::frame_ready()) {
        const auto place = [&](esp_model::part which, ImVec2* lo, ImVec2* hi) {
            float x0, y0, x1, y1;
            if (!esp_model::part_bounds(which, &x0, &y0, &x1, &y1))
                return false;
            *lo = ImVec2(min.x + x0 * w, min.y + y0 * h);
            *hi = ImVec2(min.x + x1 * w, min.y + y1 * h);
            return true;
        };
        ImVec2 lo, hi;
        if (place(esp_model::part::head, &lo, &hi)) {
            const float radius = ImMax(2.0f, (hi.x - lo.x) * 0.5f);
            dl->AddCircleFilled(ImVec2((lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f), radius, fill, 24);
            dl->AddCircle(ImVec2((lo.x + hi.x) * 0.5f, (lo.y + hi.y) * 0.5f), radius, line, 24, 1.0f);
        }
        if (place(esp_model::part::torso, &lo, &hi)) {
            const float rounding = ImMax(2.0f, (hi.x - lo.x) * 0.28f);
            dl->AddRectFilled(lo, hi, fill, rounding);
            dl->AddRect(lo, hi, line, rounding, 1.0f);
        }
        for (const esp_model::part leg : { esp_model::part::left_leg, esp_model::part::right_leg }) {
            if (!place(leg, &lo, &hi))
                continue;
            const float rounding = ImMax(1.0f, (hi.x - lo.x) * 0.4f);
            dl->AddRectFilled(lo, hi, fill, rounding);
            dl->AddRect(lo, hi, line, rounding, 1.0f);
        }
        return;
    }

    const ImVec2 head(min.x + w * 0.5f, min.y + h * 0.09f);
    dl->AddCircleFilled(head, w * 0.13f, fill, 24);
    dl->AddCircle(head, w * 0.13f, line, 24, 1.0f);
    const ImVec2 cmin(min.x + w * 0.34f, min.y + h * 0.19f), cmax(min.x + w * 0.66f, min.y + h * 0.48f);
    dl->AddRectFilled(cmin, cmax, fill, w * 0.08f);
    dl->AddRect(cmin, cmax, line, w * 0.08f, 1.0f);
    for (int s = 0; s < 2; ++s) {
        const float x0 = s == 0 ? 0.38f : 0.52f;
        const ImVec2 lmin(min.x + w * x0, min.y + h * 0.52f), lmax(min.x + w * (x0 + 0.1f), min.y + h * 0.94f);
        dl->AddRectFilled(lmin, lmax, fill, w * 0.04f);
        dl->AddRect(lmin, lmax, line, w * 0.04f, 1.0f);
    }
}

} // namespace

bool Menu::draw_esp_model(ImDrawList* dl, PlayersSettings& p, const ImVec2& model_min, const ImVec2& model_max)
{
    const ImVec2 mn(snap(model_min.x), snap(model_min.y));
    const ImVec2 mx(snap(model_max.x), snap(model_max.y));
    const float w = ImMax(1.0f, mx.x - mn.x);
    const float h = ImMax(1.0f, mx.y - mn.y);

    // Dragging the model turns it. Elements keep priority: their own drag starts
    // first and marks the frame as busy.
    float orbit = 0.0f;
    const ImGuiIO& io = ImGui::GetIO();
    if (!esp_dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
        ImGui::IsMouseHoveringRect(mn, mx))
        orbit = io.MouseDelta.x;

    // The "Player" layer of ENEMY MODEL is the visible-material slot, so the preview
    // shows exactly the chams the game would draw.
    const esp_model::frame model = esp_model::render(w, h, orbit, p.player.material != 0);
    if (!model.ready)
        return false;

    dl->AddImage(ImTextureRef(model.texture), mn, mx);
    return true;
}

void Menu::draw_esp_preview(ImDrawList* dl, const ImVec2& o, PlayersSettings& p, const ImVec2& model_min,
                            const ImVec2& model_max)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    p.target = ImClamp(p.target, 0, 2);
    EspLayout& layout = p.esp[static_cast<size_t>(p.target)];
    const ImVec2 bmin(snap(model_min.x), snap(model_min.y)), bmax(snap(model_max.x), snap(model_max.y));
    ImGui::PushID("##esp");

    // Target rects of the stacked elements, side by side.
    std::array<ImVec2, esp_element_count> target_c{}, extent{};
    {
        std::array<float, 4> off{}, list_y{};
        off.fill(px(k_side_gap));
        list_y.fill(bmin.y);
        std::vector<int> ids;
        for (int i = 0; i < esp_element_count; ++i)
            if ((i >= 0 && i < esp_element_count)&&stacked(esp_info[i].kind))
                ids.push_back(i);
        std::stable_sort(ids.begin(), ids.end(),
                         [&](int a, int b) { return layout[static_cast<size_t>(a)].order < layout[static_cast<size_t>(b)].order; });
        // Bars hug the box, everything else stacks after them.
        for (int pass = 0; pass < 2; ++pass)
            for (int i : ids) {
                const EspElement& e = layout[static_cast<size_t>(i)];
                const EspInfo& info = esp_info[i];
                const bool bar = info.kind == EspKind::bar;
                const int s = static_cast<int>(e.side);
                const bool vertical = e.side == EspSide::left || e.side == EspSide::right;
                if ((pass == 0) != bar && vertical)
                    continue;
                if (pass == 1 && !vertical)
                    continue;
                const ImVec2 ext = element_extent(info, e, bmin, bmax);
                extent[static_cast<size_t>(i)] = ext;
                if (!e.on)
                    continue;
                ImVec2 c;
                if (vertical && !bar) {
                    const float x = e.side == EspSide::left ? bmin.x - off[s] - ext.x * 0.5f : bmax.x + off[s] + ext.x * 0.5f;
                    c = ImVec2(x, list_y[s] + ext.y * 0.5f);
                    list_y[s] += ext.y + px(1.0f);
                } else {
                    switch (e.side) {
                    case EspSide::left: c = ImVec2(bmin.x - off[s] - ext.x * 0.5f, (bmin.y + bmax.y) * 0.5f); break;
                    case EspSide::right: c = ImVec2(bmax.x + off[s] + ext.x * 0.5f, (bmin.y + bmax.y) * 0.5f); break;
                    case EspSide::top: c = ImVec2((bmin.x + bmax.x) * 0.5f, bmin.y - off[s] - ext.y * 0.5f); break;
                    default: c = ImVec2((bmin.x + bmax.x) * 0.5f, bmax.y + off[s] + ext.y * 0.5f); break;
                    }
                    off[s] += (vertical ? ext.x : ext.y) + px(2.0f);
                }
                target_c[static_cast<size_t>(i)] = c;
            }
    }

    // Drag state.
    const ImVec2 m = io.MousePos;
    // The top zone stops under the preview toolbar.
    const ImVec2 zmin(bmin.x - px(k_zone), ImMax(bmin.y - px(k_zone), o.y + px(74.0f))),
        zmax(bmax.x + px(k_zone), bmax.y + px(k_zone));
    int drop_side = -1; // -1 keeps, 4 removes
    if (esp_dragging_) {
        const bool inside_zone = m.x >= zmin.x && m.x <= zmax.x && m.y >= zmin.y && m.y <= zmax.y;
        const bool inside_box = m.x >= bmin.x && m.x <= bmax.x && m.y >= bmin.y && m.y <= bmax.y;
        if (!inside_zone) {
            drop_side = 4;
        } else if (!inside_box && stacked(esp_info[esp_drag_].kind)) {
            const float l = bmin.x - m.x, r = m.x - bmax.x, t = bmin.y - m.y, b = m.y - bmax.y;
            const float best = ImMax(ImMax(l, r), ImMax(t, b));
            drop_side = best == l ? 0 : best == t ? 1 : best == r ? 2 : 3;
        }
    }
    esp_zones_ = ImClamp(esp_zones_ + (esp_dragging_ ? dt : -dt) * 8.0f, 0.0f, 1.0f);

    // Drop zones
    if (esp_zones_ > 0.0f) {
        const float za = esp_zones_ * (2.0f - esp_zones_);
        const float g = px(k_side_gap);
        const ImVec2 zones[4][2] = {
            { ImVec2(zmin.x, bmin.y), ImVec2(bmin.x - g, bmax.y) },
            { ImVec2(bmin.x, zmin.y), ImVec2(bmax.x, bmin.y - g) },
            { ImVec2(bmax.x + g, bmin.y), ImVec2(zmax.x, bmax.y) },
            { ImVec2(bmin.x, bmax.y + g), ImVec2(bmax.x, zmax.y) },
        };
        for (int s = 0; s < 4; ++s) {
            const float h = ui::animate(ImGui::GetID("##zone") + static_cast<ImGuiID>(s), drop_side == s ? 1.0f : 0.0f, 18.0f);
            const ImU32 c = draw::lerp_color(IM_COL32(255, 255, 255, 40), col::accent, h);
            dl->AddRectFilled(zones[s][0], zones[s][1], draw::with_alpha(col::accent, za * (0.03f + 0.12f * h)), px(5.0f));
            draw::border(dl, zones[s][0], zones[s][1], draw::with_alpha(c, za), px(5.0f), draw::hairline());
            const ImVec2 zc((zones[s][0].x + zones[s][1].x) * 0.5f, (zones[s][0].y + zones[s][1].y) * 0.5f);
            const char* name = k_side_names[s];
            const float tw = draw::text_width(fonts().medium, font_size::small, name);
            draw::text(dl, fonts().medium, font_size::small, ImVec2(snap(zc.x - tw * 0.5f), snap(zc.y - px(4.0f))),
                       draw::with_alpha(c, za * (0.5f + 0.5f * h)), name);
        }
    }

    // Box-level elements: box, skeleton, hitboxes.
    for (int i = 0; i < esp_element_count; ++i) {
        if(!(i >= 0 && i < esp_element_count))continue;
        const EspInfo& info = esp_info[i];
        if (stacked(info.kind))
            continue;
        EspElement& e = layout[static_cast<size_t>(i)];
        float& shown = esp_shown_[static_cast<size_t>(i)];
        shown = ImClamp(shown + (e.on ? dt : -dt) * 7.0f, 0.0f, 1.0f);
        if (shown <= 0.0f)
            continue;
        const float a = shown * (2.0f - shown);
        const ImU32 c = draw::with_alpha(u32(e.color), a);
        const float grow = (1.0f - a) * px(6.0f);
        const ImVec2 mn(bmin.x - grow, bmin.y - grow), mx(bmax.x + grow, bmax.y + grow);
        if (info.kind == EspKind::box) {
            if (e.shadow)
                dl->AddRect(ImVec2(mn.x - 1.0f, mn.y - 1.0f), ImVec2(mx.x + 1.0f, mx.y + 1.0f),
                            draw::with_alpha(IM_COL32(0, 0, 0, 200), a), 0.0f, 3.0f);
            dl->AddRect(mn, mx, c, 0.0f, 1.0f);
        } else if (info.kind == EspKind::skeleton) {
            if (e.shadow)
                draw_skeleton(dl, mn, mx, draw::with_alpha(IM_COL32(0, 0, 0, 180), a), px(2.6f));
            draw_skeleton(dl, mn, mx, c, px(1.2f));
        } else {
            draw_hitboxes(dl, bmin, bmax, c);
        }
    }

    // Stacked elements.
    int hovered = -1;
    for (int i = 0; i < esp_element_count; ++i) {
        if(!(i >= 0 && i < esp_element_count))continue;
        const EspInfo& info = esp_info[i];
        if (!stacked(info.kind))
            continue;
        EspElement& e = layout[static_cast<size_t>(i)];
        const size_t si = static_cast<size_t>(i);
        float& shown = esp_shown_[si];
        const bool dragged = esp_dragging_ && esp_drag_ == i;
        shown = ImClamp(shown + ((e.on || dragged) ? dt : -dt) * 7.0f, 0.0f, 1.0f);
        // Positions are kept relative to the box, so moving the menu doesn't make them trail behind.
        const ImVec2 target(target_c[si].x - bmin.x, target_c[si].y - bmin.y);
        if (!esp_pos_valid_ || (esp_pos_[si].x == 0.0f && esp_pos_[si].y == 0.0f))
            esp_pos_[si] = target;
        if (shown <= 0.0f)
            continue;
        if (!dragged && e.on)
            esp_pos_[si] = ImLerp(esp_pos_[si], target, 1.0f - std::exp(-16.0f * dt));
        ImVec2 c(bmin.x + esp_pos_[si].x, bmin.y + esp_pos_[si].y);
        if (dragged)
            c = ImVec2(m.x - esp_drag_offset_.x, m.y - esp_drag_offset_.y);
        const ImVec2 ext = extent[si];
        const ImVec2 mn(snap(c.x - ext.x * 0.5f), snap(c.y - ext.y * 0.5f)), mx(mn.x + ext.x, mn.y + ext.y);

        ImGui::PushID(i);
        const ImGuiID id = ImGui::GetID("##el");
        const float hit = info.kind == EspKind::bar ? px(3.0f) : px(1.0f);
        const ui::ItemState s = ui::item_behavior(id, ImVec2(mn.x - hit, mn.y - hit), ImVec2(mx.x + hit, mx.y + hit), e.on);
        if (s.hovered)
            hovered = i;
        if (s.held && esp_drag_ < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            esp_drag_ = i;
            esp_drag_offset_ = ImVec2(io.MouseClickedPos[0].x - c.x, io.MouseClickedPos[0].y - c.y);
        }
        if (esp_drag_ == i && s.held && !esp_dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left, px(k_drag_threshold)))
            esp_dragging_ = true;
        // The wheel over an element resizes it, exactly like the reference: with the
        // universal scaling above the preview on, every element follows at once.
        if (s.hovered && !esp_dragging_ && io.MouseWheel != 0.0f) {
            const float delta = io.MouseWheel * 0.05f;
            if (esp_uniform_scale_) {
                for (EspElement& other : layout)
                    if (other.on)
                        other.scale = std::clamp(other.scale + delta, 0.5f, 2.5f);
            } else {
                e.scale = std::clamp(e.scale + delta, 0.5f, 2.5f);
            }
        }
        const float hv = ui::animate(id + 1, (s.hovered || dragged) ? 1.0f : 0.0f, 18.0f);
        const float rm = ui::animate(id + 2, dragged && drop_side == 4 ? 1.0f : 0.0f, 18.0f);
        ImGui::PopID();

        const float a = shown * (2.0f - shown);
        const float scale_in = 0.75f + 0.25f * a;
        const int v0 = dl->VtxBuffer.Size;
        if (hv > 0.0f)
            dl->AddRectFilled(ImVec2(mn.x - px(3.0f), mn.y - px(2.0f)), ImVec2(mx.x + px(3.0f), mx.y + px(2.0f)),
                              draw::with_alpha(draw::lerp_color(IM_COL32(255, 255, 255, 26), IM_COL32(232, 76, 76, 60), rm), hv),
                              px(3.0f));
        if (dragged)
            draw::border(dl, ImVec2(mn.x - px(3.0f), mn.y - px(2.0f)), ImVec2(mx.x + px(3.0f), mx.y + px(2.0f)),
                         draw::lerp_color(col::accent, col::window_danger, rm), px(3.0f), draw::hairline());
        const ImU32 color = draw::lerp_color(u32(e.color), col::window_danger, rm);
        if (e.background && info.kind != EspKind::bar)
            dl->AddRectFilled(ImVec2(mn.x - px(2.0f), mn.y), ImVec2(mx.x + px(2.0f), mx.y), IM_COL32(0, 0, 0, 140), px(2.0f));
        const float element_scale = ImClamp(e.scale, 0.5f, 2.5f);
        switch (info.kind) {
        case EspKind::bar: {
            const float fill = info.name[0] == 'H' ? 0.78f : info.name[0] == 'A' ? 0.55f : 0.9f;
            const ImVec2 fmin(mn.x - px(1.0f), mn.y - px(1.0f)), fmax(mx.x + px(1.0f), mx.y + px(1.0f));
            if (e.outline)
                dl->AddRectFilled(fmin, fmax, draw::with_alpha(u32(e.outline_color), 0.9f));
            else
                dl->AddRectFilled(fmin, fmax, IM_COL32(0, 0, 0, 170));
            const bool vertical_bar = ext.y > ext.x;
            const ImVec2 tip = vertical_bar ? ImVec2(mx.x, mx.y - (mx.y - mn.y) * fill)
                                            : ImVec2(mn.x + (mx.x - mn.x) * fill, mx.y);
            const ImU32 base = e.bar_style == EspBar::gradient
                ? draw::lerp_color(IM_COL32(232, 68, 68, 255), color, fill) : color;
            if (e.bar_style == EspBar::normal) {
                dl->AddRectFilled(vertical_bar ? ImVec2(mn.x, tip.y) : mn, vertical_bar ? mx : tip, base);
            } else {
                // Both other modes paint two tones along the bar: "shade" keeps them
                // fixed, "gradient" ties the low end to the value.
                const ImU32 low = e.bar_style == EspBar::gradient
                    ? IM_COL32(232, 68, 68, 255) : draw::with_alpha(base, 0.45f);
                if (vertical_bar)
                    dl->AddRectFilledMultiColor(ImVec2(mn.x, tip.y), mx, base, base, low, low);
                else
                    dl->AddRectFilledMultiColor(mn, tip, low, base, base, low);
            }
            if (e.outline)
                draw::border(dl, fmin, fmax, draw::with_alpha(u32(e.outline_color), 0.9f), px(1.0f), draw::hairline());
            break;
        }
        case EspKind::icon: {
            const int iv0 = dl->VtxBuffer.Size;
            const float icon_size = 11.0f * element_scale;
            if (e.shadow)
                draw::any_icon(dl, ImVec2((mn.x + mx.x) * 0.5f + 1.0f, (mn.y + mx.y) * 0.5f + 1.0f), IM_COL32(0, 0, 0, 200),
                               info.sample, icon_size);
            const int outline_v0 = dl->VtxBuffer.Size;
            if (e.outline)
                for (const ImVec2& d : { ImVec2(-1.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(0.0f, -1.0f), ImVec2(0.0f, 1.0f) })
                    draw::any_icon(dl, ImVec2((mn.x + mx.x) * 0.5f + d.x, (mn.y + mx.y) * 0.5f + d.y),
                                   u32(e.outline_color), info.sample, icon_size);
            const int fill_v0 = dl->VtxBuffer.Size;
            draw::any_icon(dl, ImVec2((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f), color, info.sample, icon_size);
            if (e.flip) {
                // Mirror everything the icon just emitted around its own centre.
                const float cx = (mn.x + mx.x) * 0.5f;
                for (int v = iv0; v < dl->VtxBuffer.Size; ++v) {
                    ImDrawVert& vert = dl->VtxBuffer[v];
                    vert.pos.x = cx - (vert.pos.x - cx);
                }
            }
            static_cast<void>(outline_v0);
            static_cast<void>(fill_v0);
            break;
        }
        default: {
            ImFont* font = element_font(info.kind, e.font);
            const float size = element_size(info.kind) * element_scale;
            const std::string text = element_text(info, e);
            const float text_w = draw::text_width(font, size, text.c_str());
            // The avatar sits in front of the nickname and shifts the text right.
            float avatar_w = 0.0f;
            const bool with_avatar = e.avatar && std::strcmp(info.name, "Name") == 0;
            if (with_avatar)
                avatar_w = snap(px(14.0f)) + px(3.0f);
            const ImVec2 pen(snap(mn.x + px(1.0f) + avatar_w),
                             snap((mn.y + mx.y) * 0.5f - draw::cap_height(font, size) * 0.5f));
            if (e.shadow)
                draw::text(dl, font, size, ImVec2(pen.x + 1.0f, pen.y + 1.0f), IM_COL32(0, 0, 0, 210), text.c_str());
            if (e.outline)
                for (const ImVec2& d : { ImVec2(-1.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(0.0f, -1.0f), ImVec2(0.0f, 1.0f) })
                    draw::text(dl, font, size, ImVec2(pen.x + d.x, pen.y + d.y), u32(e.outline_color), text.c_str());
            draw::text(dl, font, size, pen, color, text.c_str());
            if (with_avatar) {
                // The shipped picture of the local account stands in for the enemy's
                // Steam avatar, which the game only exposes through its own panorama.
                const ImVec2 amin(snap(mn.x + px(1.0f)), snap((mn.y + mx.y) * 0.5f - px(7.0f)));
                const float d = snap(px(14.0f));
                draw_avatar(dl, amin, d, a);
                draw::border(dl, amin, ImVec2(amin.x + d, amin.y + d),
                             draw::with_alpha(IM_COL32(255, 255, 255, 90), a), d * 0.5f, draw::hairline());
            }
            static_cast<void>(text_w);
            break;
        }
        }
        // Appearing elements grow out of their spot.
        if (scale_in < 1.0f || a < 1.0f) {
            const ImVec2 cc((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f);
            for (int v = v0; v < dl->VtxBuffer.Size; ++v) {
                ImDrawVert& vert = dl->VtxBuffer[v];
                vert.pos = ImVec2(cc.x + (vert.pos.x - cc.x) * scale_in, cc.y + (vert.pos.y - cc.y) * scale_in);
            }
            fade_vertices(dl, v0, a);
        }
    }
    esp_pos_valid_ = true;

    // Reorders the element being edited inside its own side. The orders of that side are
    // renumbered, so repeated moves cannot drift apart.
    const auto reorder = [ & ]( const int index, const int delta )
    {
        if ( index < 0 || index >= esp_element_count || delta == 0 )
            return;
        const EspSide side = layout[ static_cast<std::size_t>( index ) ].side;
        std::vector<int> ids;
        for ( int j = 0; j < esp_element_count; ++j )
            if ( j != index && stacked( esp_info[ j ].kind )
                && layout[ static_cast<std::size_t>( j ) ].side == side )
                ids.push_back( j );
        std::stable_sort( ids.begin( ), ids.end( ), [ & ]( const int a, const int b )
        {
            return layout[ static_cast<std::size_t>( a ) ].order
                < layout[ static_cast<std::size_t>( b ) ].order;
        } );
        const auto at = std::find_if( ids.begin( ), ids.end( ), [ & ]( const int j )
        {
            return layout[ static_cast<std::size_t>( j ) ].order
                > layout[ static_cast<std::size_t>( index ) ].order;
        } );
        const auto position = static_cast<int>( std::distance( ids.begin( ), at ) );
        const int target = ImClamp( position + delta, 0, static_cast<int>( ids.size( ) ) );
        ids.insert( ids.begin( ) + target, index );
        for ( int slot = 0; slot < static_cast<int>( ids.size( ) ); ++slot )
            layout[ static_cast<std::size_t>( ids[ static_cast<std::size_t>( slot ) ] ) ].order
                = static_cast<float>( slot );
    };

    // Remove hint next to the cursor.
    if (esp_dragging_ && drop_side == 4) {
        const char* t = "Release to remove";
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const float tw = draw::text_width(fonts().medium, font_size::small, t);
        const ImVec2 hmin(snap(m.x + px(14.0f)), snap(m.y + px(10.0f))), hmax(hmin.x + tw + px(26.0f), hmin.y + px(20.0f));
        fg->AddRectFilled(hmin, hmax, col::tooltip_bg, px(5.0f));
        draw::border(fg, hmin, hmax, draw::with_alpha(col::window_danger, 0.6f), px(5.0f), draw::hairline());
        draw::icon(fg, fonts().icons, 9.5f, ImVec2(hmin.x + px(10.0f), (hmin.y + hmax.y) * 0.5f), col::window_danger, icon::trash);
        draw::text(fg, fonts().medium, font_size::small,
                   ImVec2(hmin.x + px(19.0f), (hmin.y + hmax.y) * 0.5f - draw::cap_height(fonts().medium, font_size::small) * 0.5f),
                   col::tooltip_text, t);
    }

    // Release: drop on a side, remove, or open the settings of a clicked element.
    if (esp_drag_ >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        EspElement& e = layout[static_cast<size_t>(esp_drag_)];
        if (esp_dragging_) {
            if (drop_side == 4) {
                e.on = false;
            } else if (drop_side >= 0) {
                const EspSide side = static_cast<EspSide>(drop_side);
                const bool vertical_side = side == EspSide::left || side == EspSide::right;
                const float at = vertical_side ? m.y : m.x;
                // Insert where it was dropped: the neighbour it lands after on that side
                // takes it in between, instead of the element always jumping to an end.
                int before = -1;
                float best = -1e9f;
                float first = 1e9f;
                for (int j = 0; j < esp_element_count; ++j) {
                    if (j == esp_drag_ || !stacked(esp_info[j].kind) || layout[static_cast<size_t>(j)].side != side)
                        continue;
                    const float pos = vertical_side ? target_c[static_cast<size_t>(j)].y : target_c[static_cast<size_t>(j)].x;
                    first = ImMin(first, layout[static_cast<size_t>(j)].order);
                    if (pos <= at && pos > best) {
                        best = pos;
                        before = j;
                    }
                }
                e.side = side;
                e.order = before >= 0 ? layout[static_cast<size_t>(before)].order + 0.5f
                                      : (first < 1e9f ? first - 1.0f : 0.0f);
            }
            esp_pos_[static_cast<size_t>(esp_drag_)] =
                ImVec2(m.x - esp_drag_offset_.x - bmin.x, m.y - esp_drag_offset_.y - bmin.y);
        } else if (hovered == esp_drag_) {
            esp_settings_ = esp_drag_;
            ui::popup::open("##esp_element");
        }
        esp_drag_ = -1;
        esp_dragging_ = false;
    }

    // Element settings
    if (esp_settings_ >= 0) {
        const EspInfo& info = esp_info[esp_settings_];
        EspElement& e = layout[static_cast<size_t>(esp_settings_)];
        if (ui::popup::begin("##esp_element", info.name)) {
            ui::popup::group([&] {
                ui::popup::color("Color", &e.color, true, icon::fill_drip);
                if (ui::popup::item("Default", icon::rotate_left))
                    e.color = ImGui::ColorConvertU32ToFloat4(info.color);
            });
            ui::popup::group([&] {
                ui::popup::slider("Size", &e.scale, 0.5f, 2.5f, "%.2fx", 0.05f);
                if (stacked(info.kind)) {
                    int side = static_cast<int>(e.side);
                    if (ui::popup::value("Position", &side, k_side_names, true, icon::arrows)) {
                        const EspSide next = static_cast<EspSide>(side);
                        if (next != e.side) {
                            // Moving to another side appends to that side's stack instead of
                            // keeping the old order, which used to throw the element to the top.
                            float last = 0.0f;
                            for (int j = 0; j < esp_element_count; ++j)
                                if (layout[static_cast<size_t>(j)].side == next)
                                    last = ImMax(last, layout[static_cast<size_t>(j)].order);
                            e.side = next;
                            e.order = last + 1.0f;
                        }
                    }
                }
                if (info.kind == EspKind::text || info.kind == EspKind::flag) {
                    int style = static_cast<int>(e.text_style);
                    if (ui::popup::value("Text", &style, k_text_styles, true, icon::text_height))
                        e.text_style = static_cast<EspText>(style);
                    ui::popup::value("Font", &e.font, k_font_weights, true, icon::pen);
                }
                if (info.kind == EspKind::bar) {
                    int style = static_cast<int>(e.bar_style);
                    if (ui::popup::value("Fill", &style, k_bar_styles, true, icon::fill_drip))
                        e.bar_style = static_cast<EspBar>(style);
                }
                if (info.kind == EspKind::icon) {
                    ui::popup::toggle("Flip", &e.flip, icon::arrows);
                    // Weapon icon extras, straight from the reference: the magazine count
                    // and the colour the icon takes when a grenade pin is pulled.
                    ui::popup::toggle("Ammo Count", &e.ammo_count, icon::gun);
                    ui::popup::color("Pin Pulled", &e.pin_color, true, icon::bomb);
                }
                if (std::strcmp(info.name, "Name") == 0)
                    ui::popup::toggle("Avatar", &e.avatar, icon::user);
            });
            ui::popup::group([&] {
                ui::popup::toggle("Background", &e.background, icon::image);
                ui::popup::toggle("Shadow", &e.shadow, icon::layers);
                ui::popup::toggle("Outline", &e.outline, icon::expand);
                if (e.outline)
                    ui::popup::color("Outline Color", &e.outline_color, true, icon::droplet);
            });
            ui::popup::group([&] {
                // Reordering matters most for the elements that share a side, e.g. the
                // weapon icon and the weapon name on the bottom edge.
                const bool can_move = stacked(info.kind);
                if (can_move && ui::popup::item("Move Up", icon::upload))
                    reorder(esp_settings_, -1);
                if (can_move && ui::popup::item("Move Down", icon::arrow_down))
                    reorder(esp_settings_, 1);
            });
            ui::popup::group([&] {
                if (ui::popup::item("Remove", icon::trash)) {
                    e.on = false;
                    ImGui::CloseCurrentPopup();
                }
            });
            ui::popup::end();
        }
    }
    ImGui::PopID();
    (void)o;
}

void Menu::draw_esp_items(PlayersSettings& p, ImGuiID id, const ImVec2& anchor)
{
    if (!ImGui::IsPopupOpen(id, ImGuiPopupFlags_None)) {
        esp_items_placed_ = false;
        return;
    }
    if (!esp_items_placed_) {
        esp_items_pos_ = ImVec2(anchor.x - px(150.0f), anchor.y + px(9.0f) + px(4.0f));
        esp_items_placed_ = true;
    }
    if (!ui::begin_popup_at(id, esp_items_pos_, px(300.0f), ImVec2(px(10.0f), px(6.0f))))
        return;
    EspLayout& layout = p.esp[static_cast<size_t>(ImClamp(p.target, 0, 2))];
    const Fonts& f = fonts();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const bool appearing = ImGui::IsWindowAppearing();
    float y = p0.y;

    // The title line drags the window.
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImVec2 tmin(window->Pos.x, window->Pos.y), tmax(window->Pos.x + window->Size.x, p0.y + px(20.0f));
        const ImGuiID drag_id = ImGui::GetID("##drag");
        const ui::ItemState ds = ui::item_behavior(drag_id, tmin, tmax);
        if (ds.hovered || ds.held)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        if (ds.held) {
            const ImVec2 d = ImGui::GetIO().MouseDelta;
            esp_items_pos_ = ImVec2(window->Pos.x + d.x, window->Pos.y + d.y);
        }
        const float dh = ui::animate(drag_id + 1, (ds.hovered || ds.held) ? 1.0f : 0.0f, 16.0f);
        draw::icon(dl, f.icons, 9.0f, ImVec2(p0.x + w - px(6.0f), p0.y + px(8.0f)),
                   draw::lerp_color(col::window_label_disabled, col::window_value, dh), icon::arrows);
    }

    draw::text(dl, f.semibold, font_size::window_title, ImVec2(p0.x + px(2.0f), y + px(2.0f)), col::window_title, "ESP Items");
    y += px(22.0f);
    const float chip_h = snap(px(22.0f)), chip_gap = snap(px(5.0f)), chip_pad = px(9.0f);
    int n = 0;
    for (int g = 0; g < IM_ARRAYSIZE(esp_groups); ++g) {
        draw::text(dl, f.medium, font_size::header, ImVec2(p0.x + px(2.0f), y + px(4.0f)), col::section_header, esp_groups[g]);
        y += px(18.0f);
        float x = p0.x;
        for (int i = 0; i < esp_element_count; ++i) {
        if(!(i >= 0 && i < esp_element_count))continue;
            const EspInfo& info = esp_info[i];
            if (info.group != g)
                continue;
            EspElement& e = layout[static_cast<size_t>(i)];
            const float tw = draw::text_width(f.medium, font_size::popup_label, info.name);
            const float cw = snap(tw + chip_pad * 2.0f);
            if (x + cw > p0.x + w && x > p0.x) {
                x = p0.x;
                y += chip_h + chip_gap;
            }
            const ImVec2 cmin(snap(x), snap(y)), cmax(snap(x) + cw, snap(y) + chip_h);
            ImGui::PushID(i);
            const ImGuiID cid = ImGui::GetID("##chip");
            const ui::ItemState s = ui::item_behavior(cid, cmin, cmax);
            if (s.pressed)
                e.on = !e.on;
            const float hv = ui::animate(cid + 1, s.hovered ? 1.0f : 0.0f, 18.0f);
            const float on = ui::animate(cid + 2, e.on ? 1.0f : 0.0f, 16.0f);
            const float press = ui::animate(cid + 3, s.held ? 1.0f : 0.0f, 26.0f);
            float* appear = ImGui::GetStateStorage()->GetFloatRef(cid + 4, 1.0f);
            if (appearing)
                *appear = -0.02f * static_cast<float>(n);
            *appear = ImMin(1.0f, *appear + ImGui::GetIO().DeltaTime * 7.0f);
            ImGui::PopID();
            ++n;

            const int v0 = dl->VtxBuffer.Size;
            const float rounding = px(5.0f);
            dl->AddRectFilled(cmin, cmax,
                              draw::lerp_color(draw::lerp_color(col::window_chip_bg, col::window_chip_hovered, hv),
                                               draw::lerp_color(col::chip_bg_selected, col::chip_bg_hovered, hv), on),
                              rounding);
            if (on > 0.0f)
                draw::border(dl, cmin, cmax, draw::with_alpha(IM_COL32(255, 255, 255, 70), on), rounding, draw::hairline());
            const ImU32 tc = draw::lerp_color(draw::lerp_color(col::chip_text, col::window_label, hv), col::tab_text_active, on);
            draw::text(dl, f.medium, font_size::popup_label,
                       ImVec2(snap(cmin.x + chip_pad), (cmin.y + cmax.y) * 0.5f - draw::cap_height(f.medium, font_size::popup_label) * 0.5f),
                       tc, info.name);
            const float ap = ImSaturate(*appear);
            const float sc = (1.0f - 0.06f * press) * (0.92f + 0.08f * ap);
            const ImVec2 cc((cmin.x + cmax.x) * 0.5f, (cmin.y + cmax.y) * 0.5f);
            for (int v = v0; v < dl->VtxBuffer.Size; ++v) {
                ImDrawVert& vert = dl->VtxBuffer[v];
                vert.pos = ImVec2(cc.x + (vert.pos.x - cc.x) * sc, cc.y + (vert.pos.y - cc.y) * sc);
            }
            fade_vertices(dl, v0, ap * (2.0f - ap));
            x += cw + chip_gap;
        }
        y += chip_h + px(8.0f);
    }
    ImGui::SetCursorScreenPos(p0);
    ImGui::ItemSize(ImVec2(w, y - p0.y));
    ui::end_popup();
}

} // namespace nl
