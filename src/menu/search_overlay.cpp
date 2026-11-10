// Menu source watermark: @use_sberbank
#include <cctype>
#include <cmath>
#include <cstring>
#include <vector>

#include "imgui_internal.h"
#include "menu/binds.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/menu.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;

constexpr const char* k_tab_names[] = { "Misc", "Aim", "Visuals", "Skins", "Settings" };
constexpr const char* k_subtab_names[] = { "Players", "World", "Misc" };

constexpr float k_left = layout::left_panel_x;
constexpr float k_right = layout::right_panel_x + layout::right_panel_w;
constexpr float k_results_top = layout::header_sep_y + 6.0f;
constexpr float k_results_bottom = layout::height - 8.0f;
constexpr float k_crumb_h = 22.0f;
constexpr float k_group_gap = 10.0f;

ImVec2 at(const ImVec2& o, float x, float y) { return ImVec2(o.x + snap(px(x)), o.y + snap(px(y))); }

// Offset of the first case-insensitive occurrence of `query` in `text`, or -1.
// The query is folded first so a Cyrillic layout still matches the Latin labels.
int find_nocase(const char* text, const char* query)
{
    char folded[192]{};
    search::fold_query(query, folded, sizeof(folded));
    const char* needle = folded;
    const size_t n = std::strlen(needle);
    if (!n)
        return -1;
    for (const char* t = text; *t; ++t) {
        size_t i = 0;
        while (i < n && t[i] &&
               std::tolower(static_cast<unsigned char>(t[i])) == std::tolower(static_cast<unsigned char>(needle[i])))
            ++i;
        if (i == n)
            return static_cast<int>(t - text);
    }
    return -1;
}

// Last segment of a path ("BOMB" of "COMMON\x1fVISUALS\x1fWORLD\x1fWORLD ESP\x1fBOMB").
const char* last_segment(const std::string& path)
{
    const size_t sep = path.rfind('\x1f');
    return path.c_str() + (sep == std::string::npos ? 0 : sep + 1);
}

binds::Target bind_target(const search::Entry& e)
{
    switch (e.kind) {
    case search::Kind::toggle:
    case search::Kind::color_toggle: return { e.label, e.b, binds::Kind::toggle };
    case search::Kind::slider: return { e.label, e.f, binds::Kind::slider, e.lo, e.hi, e.step, e.format };
    case search::Kind::combo: return { e.label, e.i, binds::Kind::combo, 0.0f, 1.0f, 1.0f, nullptr, e.items };
    case search::Kind::multi: return { e.label, e.b, binds::Kind::multi, 0.0f, 1.0f, 1.0f, nullptr, e.items };
    default: return {};
    }
}

} // namespace

void Menu::open_search()
{
    if (search_open_)
        return;
    search_open_ = true;
    search_focus_ = true;
    query_[0] = '\0';
    last_query_.clear();
    search_scroll_ = search_scroll_target_ = 0.0f;
    results_anim_ = 0.0f;
    ImGui::ClosePopupsExceptModals();
}

void Menu::collect_search()
{
    Config& c = config();
    const ImVec2 o(0.0f, 0.0f);
    search::begin();
    ImGui::PushID("##search_collect");
    for (int tab = 0; tab < 5; ++tab) {
        if (tab == 3)
            continue; // the inventory has no setting rows
        if (host_content && tab == 0)
            continue;
        const int subtabs = tab == 2 ? (host_content ? 1 : 2) : 1;
        for (int sub = 0; sub < subtabs; ++sub) {
            search::set_tab(tab, sub);
            search::push(tab < 2 ? "Aimbot" : "Common");
            search::push(k_tab_names[tab]);
            if (tab == 2)
                search::push(k_subtab_names[sub]);
            ImGui::PushID(tab * 4 + sub);
            if(host_content)host_content(*this,o,tab,sub);
            else switch (tab) {
            case 0: tab_rage(o, c); break;
            case 1: tab_legit(o, c); break;
            case 2:
                if (sub == 0)
                    visuals_players(o, c.visuals.players);
                else
                    visuals_world(o, c.visuals.world);
                break;
            default: tab_misc(o, c); break;
            }
            ImGui::PopID();
            if (tab == 2)
                search::pop();
            search::pop();
            search::pop();
        }
    }
    ImGui::PopID();
    search::end();
}

void Menu::draw_search(const ImVec2& o)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    search_anim_ = ImClamp(search_anim_ + (search_open_ ? dt * 4.5f : -dt * 6.0f), 0.0f, 1.0f);
    if (search_anim_ <= 0.0f)
        return;
    const float e = 1.0f - (1.0f - search_anim_) * (1.0f - search_anim_) * (1.0f - search_anim_);
    const float drop = ImSaturate((search_anim_ - 0.25f) / 0.75f);
    const float de = 1.0f - (1.0f - drop) * (1.0f - drop);

    collect_search();
    const Fonts& f = fonts();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushID("##search_overlay");
    if (!search_open_)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

    // The menu behind is blurred and darkened.
    const ImVec2 main_min = at(o, layout::main_x, 0.0f), main_max = at(o, layout::width, layout::height);
    {
        const int v0 = dl->VtxBuffer.Size;
        ui::glass(dl, main_min, main_max, px(layout::main_rounding), draw::with_alpha(col::main_bg, 0.78f), false);
        draw::border(dl, main_min, main_max, col::main_border, px(layout::main_rounding), draw::hairline());
        fade_vertices(dl, v0, e);
    }

    // Search bar: grows out of the search icon to the left edge.
    bool close = false;
    {
        const int v0 = dl->VtxBuffer.Size;
        const float bar_l = ImLerp(layout::search_cx - 12.0f, layout::group_x, e);
        const ImVec2 bmin = at(o, bar_l, layout::group_y);
        const ImVec2 bmax = at(o, layout::search_cx + 14.0f, layout::group_y + layout::group_h);
        const float rounding = px(6.0f);
        const bool active = ImGui::GetActiveID() == ImGui::GetID("##query");
        const float fa = ui::animate(ImGui::GetID("##query_focus"), active ? 1.0f : 0.0f, 14.0f);
        dl->AddRectFilled(bmin, bmax, col::combo_bg, rounding);
        draw::border(dl, bmin, bmax, draw::lerp_color(col::slider_box_border, col::accent, 0.25f + 0.35f * fa), rounding,
                     draw::hairline());
        const float cy = (bmin.y + bmax.y) * 0.5f;
        draw::icon(dl, f.icons, font_size::icon, ImVec2(bmin.x + px(13.0f), cy), draw::lerp_color(col::search_icon, col::accent, fa),
                   icon::search);
        if (e > 0.2f) {
            const ImVec2 fmin(bmin.x + px(26.0f), bmin.y), fmax(bmax.x - px(26.0f), bmax.y);
            dl->PushClipRect(fmin, fmax, true);
            ui::text_field("##query", query_, sizeof(query_), fmin, fmax, "Search settings...", search_focus_ && search_open_);
            dl->PopClipRect();
            search_focus_ = false;
        }

        // Close button
        const ImVec2 xc(bmax.x - px(13.0f), cy);
        const ImGuiID xid = ImGui::GetID("##close");
        const float hit = px(9.0f);
        const ui::ItemState xs = ui::item_behavior(xid, ImVec2(xc.x - hit, xc.y - hit), ImVec2(xc.x + hit, xc.y + hit));
        const float xh = ui::animate(xid + 1, xs.hovered ? 1.0f : 0.0f, 18.0f);
        if (xh > 0.0f)
            dl->AddCircleFilled(xc, hit, draw::with_alpha(col::window_chip_hovered, xh), 24);
        draw::icon(dl, f.icons, 10.5f, xc, draw::lerp_color(col::window_value, col::tab_text_active, xh), icon::xmark);
        close |= xs.pressed;
        fade_vertices(dl, v0, ImSaturate(e * 1.5f));
    }
    if (search_open_ && ImGui::IsKeyPressed(ImGuiKey_Escape, false) &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        close = true;

    // Results restart their animation whenever the query changes.
    if (last_query_ != query_) {
        last_query_ = query_;
        results_anim_ = 0.0f;
        search_scroll_target_ = 0.0f;
    }
    results_anim_ += dt;

    const auto& entries = search::entries();
    const auto& paths = search::paths();
    struct Group {
        int path;
        std::vector<int> rows;
    };
    std::vector<Group> groups;
    if (query_[0]) {
        for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
            const search::Entry& en = entries[static_cast<size_t>(i)];
            const std::string& path = paths[static_cast<size_t>(en.path)].text;
            if (!search::matches(en.label, query_) && !search::matches(last_segment(path), query_))
                continue;
            Group* g = nullptr;
            for (Group& existing : groups)
                if (existing.path == en.path)
                    g = &existing;
            if (!g) {
                groups.push_back({ en.path, {} });
                g = &groups.back();
            }
            g->rows.push_back(i);
        }
    }

    const ImVec2 rmin = at(o, k_left - 4.0f, k_results_top), rmax = at(o, k_right + 6.0f, k_results_bottom);
    const float row_h = px(layout::row_h);
    const float crumb_h = snap(px(k_crumb_h)), gap = snap(px(k_group_gap));
    float content_h = 0.0f;
    for (const Group& g : groups)
        content_h += crumb_h + snap(row_h * static_cast<float>(g.rows.size())) + gap;
    const float view_h = rmax.y - rmin.y;
    const float max_scroll = ImMax(0.0f, content_h - view_h);
    if (search_open_ && ImGui::IsMouseHoveringRect(rmin, rmax) && io.MouseWheel != 0.0f &&
        !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId))
        search_scroll_target_ -= io.MouseWheel * row_h * 2.5f;
    search_scroll_target_ = ImClamp(search_scroll_target_, 0.0f, max_scroll);
    search_scroll_ = ImLerp(search_scroll_, search_scroll_target_, 1.0f - std::exp(-16.0f * dt));
    if (std::fabs(search_scroll_ - search_scroll_target_) < 0.3f)
        search_scroll_ = search_scroll_target_;

    const int results_v0 = dl->VtxBuffer.Size;
    ImGui::PushClipRect(rmin, rmax, true);
    if (groups.empty()) {
        // Hint or "nothing found", centered in the results area.
        const bool empty = !query_[0];
        const char* line = empty ? "Type to search through every setting" : "Nothing found";
        const float cx = (rmin.x + rmax.x) * 0.5f, cy = (rmin.y + rmax.y) * 0.5f - px(20.0f);
        const float bob = std::sin(static_cast<float>(ImGui::GetTime()) * 2.2f) * px(2.0f);
        const float ha = ImSaturate(results_anim_ * 5.0f);
        dl->AddCircleFilled(ImVec2(cx, cy - px(18.0f) + bob), px(19.0f), draw::with_alpha(col::window_chip_bg, ha), 40);
        draw::icon(dl, f.icons, 15.0f, ImVec2(cx, cy - px(18.0f) + bob), draw::with_alpha(col::window_value, ha),
                   empty ? icon::search : icon::xmark);
        const float tw = draw::text_width(f.medium, font_size::text, line);
        draw::text(dl, f.medium, font_size::text, ImVec2(snap(cx - tw * 0.5f), snap(cy + px(12.0f))),
                   draw::with_alpha(col::window_label, ha), line);
        if (empty) {
            const char* sub = "Ctrl+F opens the search, Esc closes it";
            const float sw = draw::text_width(f.medium, font_size::small, sub);
            draw::text(dl, f.medium, font_size::small, ImVec2(snap(cx - sw * 0.5f), snap(cy + px(30.0f))),
                       draw::with_alpha(col::window_value, ha * 0.8f), sub);
        }
    }

    float y = rmin.y - search_scroll_;
    const float x0 = at(o, k_left, 0.0f).x, x1 = at(o, k_right, 0.0f).x;
    int navigate = -1;
    for (size_t gi = 0; gi < groups.size(); ++gi) {
        const Group& g = groups[gi];
        const search::Path& path = paths[static_cast<size_t>(g.path)];
        const float group_h = crumb_h + snap(row_h * static_cast<float>(g.rows.size()));
        const float gy = y;
        y += group_h + gap;
        if (gy > rmax.y || gy + group_h < rmin.y)
            continue;

        // Groups come in one after another, sliding up.
        const float t = ImSaturate((results_anim_ - 0.035f * static_cast<float>(gi)) * 7.0f);
        const float ge = 1.0f - (1.0f - t) * (1.0f - t);
        const int v0 = dl->VtxBuffer.Size;
        ImGui::PushID(g.path);

        // Breadcrumbs: AIMBOT > RAGE > MAIN > SILENT AIM, then a link that opens the tab.
        {
            float x = x0 + px(2.0f);
            const float cy = gy + crumb_h * 0.5f;
            const char* s = path.text.c_str();
            while (*s) {
                const char* end = std::strchr(s, '\x1f');
                const bool last = !end;
                if (!end)
                    end = s + std::strlen(s);
                const ImU32 c = last ? col::window_label : col::section_header;
                draw::text(dl, f.medium, font_size::header, ImVec2(snap(x), cy - draw::cap_height(f.medium, font_size::header) * 0.5f),
                           c, s, end);
                x += draw::text_width(f.medium, font_size::header, s, end);
                if (!last) {
                    draw::chevron_right(dl, ImVec2(x + px(7.0f), cy), px(1.4f), px(2.8f), col::section_header, px(1.2f));
                    x += px(14.0f);
                    s = end + 1;
                } else {
                    s = end;
                }
            }
            const ImVec2 lc(x + px(11.0f), cy);
            const ImGuiID lid = ImGui::GetID("##link");
            const float hit = px(8.0f);
            const ui::ItemState ls = ui::item_behavior(lid, ImVec2(lc.x - hit, lc.y - hit), ImVec2(lc.x + hit, lc.y + hit));
            const float lh = ui::animate(lid + 1, ls.hovered ? 1.0f : 0.0f, 18.0f);
            draw::icon(dl, f.icons, 9.5f * (1.0f + 0.12f * lh), lc,
                       draw::lerp_color(col::section_header, col::accent, 0.35f + 0.65f * lh), icon::link);
            if (ls.pressed)
                navigate = g.path;
        }

        // Panel with the matching rows.
        const ImVec2 pmin(x0, snap(gy + crumb_h)), pmax(x1, snap(gy + group_h));
        dl->AddRectFilled(pmin, pmax, col::panel_bg, px(layout::panel_rounding));
        draw::border(dl, pmin, pmax, col::panel_border, px(layout::panel_rounding), draw::hairline());
        for (size_t ri = 0; ri < g.rows.size(); ++ri) {
            const search::Entry& en = entries[static_cast<size_t>(g.rows[ri])];
            const float top = snap(pmin.y + row_h * static_cast<float>(ri));
            const float bottom = snap(pmin.y + row_h * static_cast<float>(ri + 1));
            const float cy = (top + bottom) * 0.5f;
            if (ri + 1 < g.rows.size())
                draw::hline(dl, pmin.x + px(layout::row_pad_left), pmax.x - px(layout::row_sep_pad_right_l), bottom,
                            col::row_separator);
            ImGui::PushID(en.label);
            ImGui::PushID(en.b ? static_cast<const void*>(en.b)
                               : en.f ? static_cast<const void*>(en.f)
                                      : en.i ? static_cast<const void*>(en.i) : static_cast<const void*>(en.c));
            const ImVec2 row_min(pmin.x, top), row_max(pmax.x, bottom);
            const binds::Target target = bind_target(en);
            if (target.ptr && search_open_)
                binds::row(target, row_min, row_max);
            const bool hovered = ImGui::IsMouseHoveringRect(row_min, row_max) && search_open_;
            const float hv = ui::animate(ImGui::GetID("##hover"), hovered ? 1.0f : 0.0f, 16.0f);
            if (hv > 0.0f)
                dl->AddRectFilled(ImVec2(row_min.x + px(4.0f), top + px(2.0f)), ImVec2(row_max.x - px(4.0f), bottom - px(2.0f)),
                                  draw::with_alpha(col::panel_row_hover, hv), px(5.0f));

            // Label with the matching part in the style color.
            const float alpha = en.enabled ? 1.0f : disabled_alpha;
            const ImVec2 pen(snap(pmin.x + px(layout::row_pad_left)), cy - draw::cap_height(f.medium, font_size::label) * 0.5f);
            const int m = find_nocase(en.label, query_);
            const ImU32 base = draw::with_alpha(col::label, alpha);
            if (m >= 0) {
                const char* mb = en.label + m;
                const char* me = mb + std::strlen(query_);
                draw::text(dl, f.medium, font_size::label, pen, base, en.label, mb);
                const float mx = pen.x + draw::text_width(f.medium, font_size::label, en.label, mb);
                const float mw = draw::text_width(f.medium, font_size::label, mb, me);
                dl->AddRectFilled(ImVec2(mx - px(1.0f), cy - px(8.0f)), ImVec2(mx + mw + px(1.0f), cy + px(8.0f)),
                                  draw::with_alpha(col::accent, 0.14f * alpha), px(3.0f));
                draw::text(dl, f.medium, font_size::label, ImVec2(mx, pen.y), draw::with_alpha(col::accent, alpha), mb, me);
                draw::text(dl, f.medium, font_size::label, ImVec2(mx + mw, pen.y), base, me);
            } else {
                draw::text(dl, f.medium, font_size::label, pen, base, en.label);
            }
            if (target.ptr && binds::bound(target.ptr))
                binds::bound_icon(dl, pen.x + draw::text_width(f.medium, font_size::label, en.label), cy, col::bind_icon);

            // Control on the right, as in the panels.
            const float right = pmax.x - px(layout::row_pad_right_l);
            const ImVec2 tsize = ui::toggle_size();
            const float swatch = snap(px(layout::swatch_size));
            const ImVec2 csize = ui::combo_size();
            switch (en.kind) {
            case search::Kind::toggle:
                ui::toggle("##toggle", en.b, ImVec2(right - tsize.x, cy - tsize.y * 0.5f), en.enabled);
                break;
            case search::Kind::color_toggle:
                ui::color_swatch("##color", en.c, ImVec2(snap(right - tsize.x - px(layout::swatch_gap) - swatch), snap(cy - swatch * 0.5f)),
                                 swatch, en.enabled);
                ui::toggle("##toggle", en.b, ImVec2(right - tsize.x, cy - tsize.y * 0.5f), en.enabled);
                break;
            case search::Kind::color:
                ui::color_swatch("##color", en.c, ImVec2(snap(right - swatch), snap(cy - swatch * 0.5f)), swatch, en.enabled);
                break;
            case search::Kind::slider:
                ui::slider("##slider", right - px(layout::slider_track_dx + 40.0f), right, cy, en.f, en.lo, en.hi, en.format,
                           en.step, en.enabled);
                break;
            case search::Kind::combo:
            case search::Kind::multi: {
                const ImVec2 cmin(snap(right - csize.x), snap(cy - csize.y * 0.5f)), cmax(cmin.x + csize.x, cmin.y + csize.y);
                if (en.c)
                    ui::color_swatch("##color", en.c,
                                     ImVec2(snap(cmin.x - px(layout::swatch_gap) - swatch), snap(cy - swatch * 0.5f)), swatch,
                                     en.enabled);
                if (en.kind == search::Kind::combo)
                    ui::combo_box("##combo", cmin, cmax, en.items, en.i, en.enabled);
                else
                    ui::multi_combo_box("##multi", cmin, cmax, en.items, en.b, "Select", en.enabled);
                break;
            }
            }
            ImGui::PopID();
            ImGui::PopID();
        }

        ImGui::PopID();
        fade_vertices(dl, v0, ge, (1.0f - ge) * px(10.0f));
    }

    // Scrollbar
    if (max_scroll > 0.0f) {
        const float bar_w = snap(px(3.0f));
        const float bar_x = rmax.x - bar_w;
        const float thumb_h = ImMax(px(24.0f), view_h * view_h / content_h);
        const float thumb_y = rmin.y + (view_h - thumb_h) * (search_scroll_ / max_scroll);
        const ImGuiID sid = ImGui::GetID("##scrollbar");
        const ui::ItemState ss = ui::item_behavior(sid, ImVec2(bar_x - px(3.0f), rmin.y), ImVec2(rmax.x + px(2.0f), rmax.y));
        if (ss.held)
            search_scroll_target_ = search_scroll_ = ImClamp(
                search_scroll_ + io.MouseDelta.y * max_scroll / ImMax(1.0f, view_h - thumb_h), 0.0f, max_scroll);
        const float sh = ui::animate(sid + 1, (ss.hovered || ss.held) ? 1.0f : 0.0f, 14.0f);
        dl->AddRectFilled(ImVec2(bar_x, rmin.y), ImVec2(bar_x + bar_w, rmax.y), IM_COL32(255, 255, 255, 8), bar_w * 0.5f);
        dl->AddRectFilled(ImVec2(bar_x, thumb_y), ImVec2(bar_x + bar_w, thumb_y + thumb_h),
                          draw::with_alpha(IM_COL32_WHITE, 0.22f + 0.2f * sh), bar_w * 0.5f);
    }
    ImGui::PopClipRect();
    fade_vertices(dl, results_v0, de, (1.0f - de) * -px(8.0f));

    if (!search_open_)
        ImGui::PopItemFlag();
    ImGui::PopID();

    if (navigate >= 0) {
        const search::Path& p = paths[static_cast<size_t>(navigate)];
        pending_tab_ = p.tab;
        pending_subtab_ = p.subtab;
        close = true;
    }
    if (close && search_open_) {
        search_open_ = false;
        ImGui::ClearActiveID();
    }
}

} // namespace nl
