// Menu source watermark: @use_sberbank
// The Ambience view: world lighting configured per map.
//
// It lies over the World panels the way the reference product shows it: the panels stay
// underneath, blurred and inert. MAP SELECTION on the left holds a card for Global and one
// per saved map (the map you are on is offered as a card of its own until it is added),
// EFFECTS on the right edits the selected card. The close button takes the place of the
// search icon; Esc and the sidebar's World entry close the view as well.

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <span>
#include <string>

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/esp_model.h"
#include "menu/fonts.h"
#include "menu/map_preview.h"
#include "menu/menu.h"
#include "menu/panel.h"
#include "menu/sky_list.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;
using ui::popup::color_toggle;
using ui::popup::group;
using ui::popup::slider;
using ui::popup::toggle;

// Grid of the view in menu coordinates, measured on the reference.
constexpr float k_header_cap_y = 35.6f; // MAP SELECTION / EFFECTS
constexpr float k_card_x = 147.5f, k_card_w = 210.0f;
constexpr float k_card_top = k_header_cap_y + (layout::panel_y - layout::section_cap_y);
constexpr float k_card_h = 28.0f, k_card_pitch = 36.4f, k_card_rounding = 6.0f;
constexpr float k_card_pad = 10.5f;        // card left edge -> label
constexpr float k_card_dots = 15.0f;       // card right edge -> dots center
constexpr float k_card_toggle_gap = 12.0f; // toggle right edge -> dots center
constexpr float k_card_toggle_scale = 0.68f;
constexpr ImU32 k_card_border_on = IM_COL32(62, 64, 78, 255);
constexpr float k_card_image_from = 0.38f; // share of the card left of the map picture
// Override Sky window: the picker on top, the skybox list under it.
constexpr float k_sky_w = 190.0f, k_sky_pad = 7.0f;
constexpr float k_sky_row_h = 30.0f, k_sky_row_gap = 4.0f, k_sky_rows_shown = 4.5f, k_sky_rounding = 5.0f;
constexpr float k_sky_image_from = 0.36f;
constexpr ImU32 k_sky_selected_border = IM_COL32(200, 204, 214, 150);
constexpr float k_effects_x = 366.0f, k_effects_w = 253.5f;
constexpr float k_close_x = 610.0f, k_close_y = 22.0f;

constexpr const char* k_removals[] = { "Fog", "Lighting", "Post Processing", "Shadows", "Skybox", "Particles" };

// Older presets stored the game's raw map string ("maps/de_mirage.vpk"); the list and the
// lookups need the bare id.
std::string bare_map(std::string name)
{
    const auto slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name.erase(0, slash + 1);
    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".vpk") == 0)
        name.resize(name.size() - 4);
    return name;
}

// "de_inferno" -> "Inferno", the way the reference labels its cards.
std::string pretty_map(const std::string& id)
{
    std::string out = id;
    const auto underscore = out.find('_');
    if (underscore != std::string::npos && underscore <= 3)
        out.erase(0, underscore + 1);
    if (!out.empty())
        out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    return out;
}

ImVec2 at(const ImVec2& o, float x, float y) { return ImVec2(o.x + snap(px(x)), o.y + snap(px(y))); }

} // namespace

void Menu::draw_ambience(const ImVec2& o, WorldSettings& w)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ambience_fade_ = ImMin(1.0f, ambience_fade_ + ImGui::GetIO().DeltaTime * 7.0f);
    ImGui::PushID("##ambience_view");
    const int v0 = dl->VtxBuffer.Size;

    // Frosted glass over the whole main area: the header and the World panels drawn so far
    // show through blurred.
    const ImVec2 main_min = at(o, layout::main_x, 0.0f);
    const ImVec2 main_max = at(o, layout::width, layout::height);
    ui::glass(dl, main_min, main_max, px(layout::main_rounding), draw::with_alpha(col::main_bg, 0.62f), false);
    draw::border(dl, main_min, main_max, col::main_border, px(layout::main_rounding), draw::hairline());

    // Close button in place of the search icon. Esc closes too, unless it is closing a
    // popup of this view (checked against the previous frame, popups are begun later).
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImGuiID popup_seen = ImGui::GetID("##popup_seen");
    bool close = ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !search_open_ && !storage->GetBool(popup_seen, false);
    {
        const ImVec2 c(o.x + px(k_close_x), o.y + px(k_close_y));
        const float hit = px(9.0f);
        const ImGuiID id = ImGui::GetID("##close");
        const ui::ItemState s = ui::item_behavior(id, ImVec2(c.x - hit, c.y - hit), ImVec2(c.x + hit, c.y + hit));
        const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
        if (h > 0.0f)
            dl->AddCircleFilled(c, px(10.0f) * (0.7f + 0.3f * h), draw::with_alpha(col::group_hovered, h), 24);
        draw::icon(dl, fonts().icons, 11.0f, c, draw::lerp_color(col::window_value, col::tab_text_active, h), icon::xmark);
        close |= s.pressed;
    }

    ambience_cards(o, w);
    ambience_effects(o, w);
    storage->SetBool(popup_seen, ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel));

    if (ambience_fade_ < 1.0f) {
        const float e = ambience_fade_ * (2.0f - ambience_fade_);
        fade_vertices(dl, v0, e);
    }
    ImGui::PopID();
    if (close) {
        world_view_ = 0;
        ambience_menu_ = -1;
    }
}

void Menu::ambience_cards(const ImVec2& o, WorldSettings& w)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* font = fonts().medium;
    draw::text(dl, font, font_size::header, at(o, k_card_x + layout::section_pad_left_l - 1.0f, k_header_cap_y),
               col::section_header, "MAP SELECTION");

    // The map the player is on gets a card of its own until it is added to the list.
    const std::string current = bare_map(esp_model::current_map_name());
    bool listed = current.empty();
    for (int i = 1; i < w.map_count; ++i)
        listed = listed || current == bare_map(w.map_name(i));
    const bool offer = !listed && w.map_count < ambience_map_count;
    const int cards = w.map_count + (offer ? 1 : 0);

    const auto add_current = [&] {
        AmbienceMap& entry = w.maps[static_cast<size_t>(w.map_count)];
        entry = AmbienceMap{};
        std::snprintf(entry.map, ambience_map_name_len, "%s", current.c_str());
        entry.ambience = w.maps[0].ambience;
        entry.used = true;
        w.selected_map = w.map_count;
        ++w.map_count;
    };

    const ImGuiID menu_id = ImGui::GetID("##card_menu");
    for (int i = 0; i < cards; ++i) {
        const bool virtual_card = i >= w.map_count; // the offered current map
        const bool selected = !virtual_card && i == w.selected_map;
        ImGui::PushID(i);

        const float top = k_card_top + k_card_pitch * static_cast<float>(i);
        const ImVec2 mn = at(o, k_card_x, top), mx = at(o, k_card_x + k_card_w, top + k_card_h);
        const float cy = (mn.y + mx.y) * 0.5f;
        const ImVec2 dots_c(snap(mx.x - px(k_card_dots)), cy);
        const ImVec2 tsize = ui::toggle_size(k_card_toggle_scale);
        const ImVec2 tpos(snap(dots_c.x - px(k_card_toggle_gap)) - tsize.x, snap(cy - tsize.y * 0.5f));

        // The card itself stops short of its controls, so they never fight over a click.
        const float hit_right = i == 0 ? dots_c.x - px(9.0f) : tpos.x - px(4.0f);
        const ImGuiID id = ImGui::GetID("##card");
        const ui::ItemState s = ui::item_behavior(id, mn, ImVec2(hit_right, mx.y));
        const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(mn, mx);
        const float h = ui::animate(id + 1, hovered ? 1.0f : 0.0f, 18.0f);
        const float on = ui::animate(id + 2, selected ? 1.0f : 0.0f, 18.0f);

        const ImU32 card_bg = draw::lerp_color(col::panel_bg, col::chip_bg_selected, ImMax(on, h * 0.55f));
        dl->AddRectFilled(mn, mx, card_bg, px(k_card_rounding));

        // The map's loading-screen picture fills the right of the card and fades into it
        // towards the label.
        const std::string map_id = i == 0 ? std::string() : virtual_card ? current : bare_map(w.map_name(i));
        if (!map_id.empty()) {
            const ImVec2 image_min(snap(mn.x + (mx.x - mn.x) * k_card_image_from), mn.y);
            const ImU32 tint = draw::with_alpha(IM_COL32_WHITE, virtual_card ? 0.45f : ImLerp(0.6f, 0.9f, ImMax(on, h)));
            maps::draw(dl, map_id.c_str(), image_min, mx, tint, px(k_card_rounding), ImDrawFlags_RoundCornersRight, 0.6f);
        }
        draw::border(dl, mn, mx, draw::lerp_color(col::panel_border, k_card_border_on, ImMax(on, h * 0.5f)),
                     px(k_card_rounding), draw::hairline());

        const std::string label = i == 0 ? std::string("Global") : pretty_map(map_id);
        const float size = font_size::label;
        draw::text(dl, font, size,
                   ImVec2(snap(mn.x + px(k_card_pad)), snap(cy - draw::cap_height(font, size) * 0.5f + px(layout::row_label_dy))),
                   virtual_card ? col::window_value : col::label, label.c_str());

        // Global always applies; a map card switches its override on and off.
        if (i > 0) {
            if (virtual_card) {
                bool off = false;
                if (ui::toggle("##use", &off, tpos, true, k_card_toggle_scale))
                    add_current();
            } else {
                ui::toggle("##use", &w.maps[static_cast<size_t>(i)].used, tpos, true, k_card_toggle_scale);
            }
        }
        if (ui::dots_button("##dots", dots_c)) {
            ambience_menu_ = i;
            ImGui::OpenPopup(menu_id);
            ImGuiStorage* st = ImGui::GetStateStorage();
            st->SetFloat(ImGui::GetID("##menu_x"), ImGui::GetIO().MousePos.x);
            st->SetFloat(ImGui::GetID("##menu_y"), ImGui::GetIO().MousePos.y);
        }
        if (s.pressed) {
            if (virtual_card)
                add_current();
            else
                w.selected_map = i;
        }
        ImGui::PopID();
    }

    // "•••" menu of a card. The position was stored inside the card's ID scope, so it is
    // looked up through the same IDs.
    const int target = ambience_menu_;
    ImGui::PushID(std::max(target, 0));
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImVec2 pos(st->GetFloat(ImGui::GetID("##menu_x")), st->GetFloat(ImGui::GetID("##menu_y")));
    ImGui::PopID();
    if (ui::begin_popup_id(menu_id, pos, pos, px(170.0f))) {
        if (target == 0) {
            if (ui::popup::item("Reset", icon::rotate_left)) {
                w.maps[0].ambience = AmbienceSettings{};
                ImGui::CloseCurrentPopup();
            }
        } else if (target > 0 && target < w.map_count) {
            if (ui::popup::item("Copy from Global", icon::rotate_left)) {
                w.maps[static_cast<size_t>(target)].ambience = w.maps[0].ambience;
                ImGui::CloseCurrentPopup();
            }
            if (ui::popup::item("Delete", icon::trash)) {
                // Later cards move up, so the list keeps its order.
                for (int k = target; k + 1 < w.map_count; ++k)
                    w.maps[static_cast<size_t>(k)] = w.maps[static_cast<size_t>(k + 1)];
                w.maps[static_cast<size_t>(w.map_count - 1)] = AmbienceMap{};
                --w.map_count;
                if (w.selected_map == target)
                    w.selected_map = 0;
                else if (w.selected_map > target)
                    --w.selected_map;
                ambience_menu_ = -1;
                ImGui::CloseCurrentPopup();
            }
        } else if (offer && target == w.map_count) {
            if (ui::popup::item("Add map", icon::check)) {
                add_current();
                ImGui::CloseCurrentPopup();
            }
        }
        ui::end_popup();
    }
}

namespace {

enum class Mood { midnight, sunset, storm, neon, winter, clear };

void apply_mood(Mood mood, AmbienceSettings& a, WorldSettings& w)
{
    const bool removals[6] = { a.removals[0], a.removals[1], a.removals[2], a.removals[3], a.removals[4], a.removals[5] };
    a = AmbienceSettings{};
    std::copy(std::begin(removals), std::end(removals), std::begin(a.removals));
    w.weather_rain = w.weather_snow = w.weather_stars = false;
    w.weather_wind = w.weather_lightning = w.weather_model_wetness = false;

    switch (mood) {
    case Mood::midnight:
        a.nightmode = true; a.nightmode_color = { 0.20f, 0.23f, 0.38f, 1.0f }; a.exposure = 0.72f;
        a.sky = true; a.cloud_color = { 0.10f, 0.13f, 0.28f, 1.0f }; a.sky_brightness = 0.35f;
        a.fog = true; a.fog_color = { 0.06f, 0.08f, 0.16f, 1.0f }; a.fog_end = 4200.0f; a.fog_density = 0.35f;
        w.weather_stars = true;
        break;
    case Mood::sunset:
        a.sunlight = true; a.sunlight_color = { 1.0f, 0.62f, 0.36f, 1.0f }; a.sunlight_intensity = 1.35f;
        a.sky = true; a.cloud_color = { 1.0f, 0.52f, 0.34f, 1.0f }; a.sky_brightness = 1.25f;
        a.fog = true; a.fog_color = { 0.95f, 0.55f, 0.38f, 1.0f }; a.fog_end = 5200.0f; a.fog_density = 0.22f;
        a.bloom = true; a.bloom_amount = 0.45f;
        break;
    case Mood::storm:
        a.nightmode = true; a.nightmode_color = { 0.36f, 0.39f, 0.46f, 1.0f }; a.exposure = 0.48f;
        a.sky = true; a.cloud_color = { 0.42f, 0.45f, 0.52f, 1.0f }; a.sky_brightness = 0.5f;
        a.fog = true; a.fog_color = { 0.38f, 0.41f, 0.47f, 1.0f }; a.fog_end = 2600.0f; a.fog_density = 0.4f;
        w.weather_rain = true; w.weather_wind = true; w.wind_strength = 6.0f; w.wind_turbulence = 2.0f;
        w.weather_lightning = true; w.lightning_interval = 6.0f; w.lightning_flash = true;
        w.weather_model_wetness = true;
        break;
    case Mood::neon:
        a.nightmode = true; a.nightmode_color = { 0.45f, 0.22f, 0.85f, 1.0f }; a.exposure = 0.6f;
        a.sky = true; a.cloud_color = { 0.85f, 0.25f, 0.95f, 1.0f }; a.sky_brightness = 0.85f;
        a.fog = true; a.fog_color = { 0.35f, 0.12f, 0.55f, 1.0f }; a.fog_end = 3800.0f; a.fog_density = 0.3f;
        a.bloom = true; a.bloom_amount = 0.85f;
        w.weather_stars = true; w.stars_color = { 0.95f, 0.45f, 1.0f, 1.0f };
        break;
    case Mood::winter:
        a.sunlight = true; a.sunlight_color = { 0.82f, 0.9f, 1.0f, 1.0f }; a.sunlight_intensity = 1.1f;
        a.sky = true; a.cloud_color = { 0.86f, 0.92f, 1.0f, 1.0f }; a.sky_brightness = 1.15f;
        a.fog = true; a.fog_color = { 0.86f, 0.9f, 0.96f, 1.0f }; a.fog_end = 3400.0f; a.fog_density = 0.3f;
        w.weather_snow = true; w.weather_wind = true; w.wind_strength = 2.0f;
        break;
    case Mood::clear:
        break;
    }
}

// Skybox header and list under the Override Sky picker.
void skybox_list(AmbienceSettings& a)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    ImFont* font = fonts().medium;
    const float w = ImGui::GetContentRegionAvail().x;
    const float pad = px(k_sky_pad);

    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float gap = snap(px(7.0f));
        ImGui::ItemSize(ImVec2(w, gap * 2.0f));
        draw::hline(dl, p.x - pad + draw::hairline(), p.x + w + pad - draw::hairline(), p.y + gap, col::window_separator);
    }
    {
        // [image] Skybox                 [refresh] [folder]
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float h = snap(px(16.0f));
        ImGui::ItemSize(ImVec2(w, h));
        const float cy = p.y + h * 0.5f;
        const float size = font_size::popup_label;
        draw::any_icon(dl, ImVec2(snap(p.x + px(6.0f)), cy), col::window_value, icon::image);
        draw::text(dl, font, size, ImVec2(snap(p.x + px(16.5f)), snap(cy - draw::cap_height(font, size) * 0.5f)),
                   col::window_label, "Skybox");
        const auto button = [&](const char* str_id, float cx, const char* glyph) {
            const ImGuiID id = ImGui::GetID(str_id);
            const float hit = px(7.0f);
            const ui::ItemState s = ui::item_behavior(id, ImVec2(cx - hit, p.y), ImVec2(cx + hit, p.y + h));
            const float hv = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 18.0f);
            draw::any_icon(dl, ImVec2(cx, cy), draw::lerp_color(col::window_value, col::tab_text_active, hv), glyph);
            return s.pressed;
        };
        if (button("##sky_folder", snap(p.x + w - px(6.0f)), icon::folder_open))
            skies::open_folder();
        if (button("##sky_refresh", snap(p.x + w - px(22.0f)), icon::rotate))
            skies::refresh();
    }
    ImGui::ItemSize(ImVec2(w, snap(px(6.0f))));

    const std::vector<skies::Entry>& list = skies::list();
    const int count = static_cast<int>(list.size());
    const int selected = skies::find(a.skybox);
    const float row_h = snap(px(k_sky_row_h)), gap = snap(px(k_sky_row_gap));
    const float list_h = snap(row_h * k_sky_rows_shown + gap * std::floor(k_sky_rows_shown));
    const float rounding = px(k_sky_rounding);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    if (ImGui::BeginChild("##sky_list", ImVec2(w, list_h), ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar)) {
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            const ImVec2 mn = ImGui::GetCursorScreenPos();
            ImGui::ItemSize(ImVec2(w, row_h + (i + 1 < count ? gap : 0.0f)));
            const ImVec2 mx(mn.x + w, mn.y + row_h);
            if (ImGui::IsRectVisible(mn, mx)) {
                const ImGuiID id = ImGui::GetID("##sky");
                const ui::ItemState s = ui::item_behavior(id, mn, mx);
                const float hv = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 18.0f);
                const float on = ui::animate(id + 2, i == selected ? 1.0f : 0.0f, 18.0f);
                const ImU32 bg = draw::lerp_color(col::picker_field_bg, col::window_chip_hovered, hv * 0.6f);
                cdl->AddRectFilled(mn, mx, bg, rounding);

                // The unwrapped horizon fills the right of the row and fades into it.
                const ImVec2 image_min(snap(mn.x + w * k_sky_image_from), mn.y);
                const ImU32 tint = draw::with_alpha(IM_COL32_WHITE, ImLerp(0.75f, 1.0f, ImMax(hv, on)));
                skies::draw(cdl, i, image_min, mx, tint, rounding, ImDrawFlags_RoundCornersRight, 0.55f);

                const float size = font_size::popup_label;
                const ImU32 text = list[static_cast<size_t>(i)].custom ? col::accent : col::window_label;
                draw::text(cdl, font, size, ImVec2(snap(mn.x + px(9.0f)), snap((mn.y + mx.y - draw::cap_height(font, size)) * 0.5f)),
                           draw::lerp_color(text, col::tab_text_active, ImMax(hv, on)), list[static_cast<size_t>(i)].name.c_str());
                if (on > 0.0f)
                    draw::border(cdl, mn, mx, draw::with_alpha(k_sky_selected_border, on), rounding, px(1.0f));
                if (s.pressed)
                    std::snprintf(a.skybox, sizeof(a.skybox), "%s", list[static_cast<size_t>(i)].material.c_str());
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

// Override Sky: an untitled window with the sky picker and the skybox list.
void sky_window(bool pressed, AmbienceSettings& a)
{
    ImGuiStorage* st = ImGui::GetStateStorage();
    const ImGuiID id = ImGui::GetID("##sky_window");
    const ImGuiID x_key = ImGui::GetID("##sky_x"), y_key = ImGui::GetID("##sky_y");
    if (pressed) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        st->SetFloat(x_key, m.x + px(12.0f));
        st->SetFloat(y_key, m.y - px(30.0f));
        ImGui::OpenPopup(id);
        skies::warm();
    }
    const ImVec2 pos(st->GetFloat(x_key), st->GetFloat(y_key));
    if (ui::begin_popup_at(id, pos, px(k_sky_w), ImVec2(px(k_sky_pad), px(k_sky_pad)), px(8.0f))) {
        ui::PickerStyle style;
        style.title = "Sky";
        style.colors = 3;
        style.alpha = false;
        style.saved_row = true;
        ui::color_picker_embed("##sky_color", &a.cloud_color, style);
        skybox_list(a);
        ui::end_popup();
    }
}

} // namespace

// Lighting is per card; Weather is shared by all maps.
void Menu::ambience_effects(const ImVec2& o, WorldSettings& w)
{
    AmbienceSettings& a = w.ambience();
    Panel p(o, Column::right, k_effects_x, k_effects_w, k_header_cap_y, "EFFECTS", 9);
    p.chevron("Presets", [&] {
        group([&] {
            const struct { const char* label; Mood mood; } moods[] = {
                { "Midnight", Mood::midnight }, { "Sunset", Mood::sunset }, { "Storm", Mood::storm },
                { "Neon", Mood::neon }, { "Winter", Mood::winter },
            };
            for (const auto& m : moods)
                if (ui::popup::item(m.label, icon::check))
                    apply_mood(m.mood, a, w);
        });
        group([&] {
            if (ui::popup::item("Clear Everything", icon::rotate_left))
                apply_mood(Mood::clear, a, w);
        });
    });
    p.toggle("Nightmode", &a.nightmode, [&] {
        group([&] {
            ui::popup::color("Color", &a.nightmode_color);
            slider("Darkness", &a.exposure, 0.0f, 1.0f, "%.2f", 0.01f);
        });
    });
    p.toggle("Fullbright", &a.fullbright, [&] {
        group([&] {
            ui::popup::color("Color", &a.fullbright_color);
            slider("Intensity", &a.fullbright_intensity, 1.0f, 6.0f, "%.1fx", 0.1f);
        });
    });
    p.toggle_window("Override Sky", &a.sky, [&](bool pressed) { sky_window(pressed, a); });
    p.toggle("Sunlight", &a.sunlight, [&] {
        group([&] {
            ui::popup::color("Color", &a.sunlight_color);
            slider("Intensity", &a.sunlight_intensity, 0.0f, 4.0f, "%.2f", 0.05f);
        });
    });
    p.toggle("Fog", &a.fog, [&] {
        group([&] {
            ui::popup::color("Color", &a.fog_color);
            slider("Distance", &a.fog_end, 100.0f, 8000.0f, "%.0f", 10.0f);
            slider("Density", &a.fog_density, 0.0f, 1.0f, "%.2f", 0.01f);
            slider("Falloff", &a.fog_exponent, 0.1f, 8.0f, "%.1f", 0.1f);
        });
    });
    p.chevron("Post Processing", [&] {
        group([&] {
            if (toggle("Bloom", &a.bloom) && a.bloom && a.bloom_amount < 0.0f)
                a.bloom_amount = 0.5f;
            slider("Amount", &a.bloom_amount, 0.0f, 1.0f, "%.2f", 0.01f, a.bloom);
        });
        group([&] {
            toggle("Exposure", &a.gamma);
            slider("Level", &a.gamma_amount, 0.2f, 3.0f, "%.2f", 0.01f, a.gamma);
        });
        group([&] {
            toggle("Depth of Field", &a.dof);
            slider("Near", &a.dof_near, 0.0f, 4000.0f, "%.0f", 10.0f, a.dof);
            slider("Far", &a.dof_far, 0.0f, 8000.0f, "%.0f", 10.0f, a.dof);
        });
    });
    p.chevron("Weather", [&] {
        group([&] {
            color_toggle("Rain", &w.rain_color, &w.weather_rain);
            color_toggle("Snow", &w.snow_color, &w.weather_snow);
            color_toggle("Stars", &w.stars_color, &w.weather_stars);
        });
        group([&] {
            toggle("Wind", &w.weather_wind);
            slider("Strength", &w.wind_strength, 0.0f, 10.0f, "%.1f", 0.1f, w.weather_wind);
            slider("Direction", &w.wind_direction, 0.0f, 360.0f, "%.0f\xc2\xb0", 1.0f, w.weather_wind);
            slider("Gusts", &w.wind_turbulence, 0.0f, 5.0f, "%.1f", 0.1f, w.weather_wind);
        });
        group([&] {
            color_toggle("Lightning", &w.lightning_color, &w.weather_lightning);
            slider("Interval", &w.lightning_interval, 1.0f, 30.0f, "%.1fs", 0.5f, w.weather_lightning);
            toggle("Strike on Kill", &w.lightning_spawn_on_kill, nullptr, w.weather_lightning);
            toggle("Sky Flash", &w.lightning_flash, nullptr, w.weather_lightning);
            toggle("Thunder", &w.lightning_sound, nullptr, w.weather_lightning);
            slider("Volume", &w.thunder_volume, 0.0f, 1.0f, "%.2f", 0.01f, w.weather_lightning && w.lightning_sound);
        });
        group([&] {
            toggle("Wet Surfaces", &w.weather_model_wetness);
            slider("Wetness", &w.wetness_density, 0.0f, 4.0f, "%.1f", 0.1f, w.weather_model_wetness);
        });
    });
    p.multi("Removals", k_removals, a.removals);
}

} // namespace nl
