// Menu source watermark: @use_sberbank
#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/esp_model.h"
#include "menu/fonts.h"
#include "menu/images.h"
#include "menu/inventory_catalog.h"
#include "menu/menu.h"
#include "menu/panel.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;
using ui::popup::check;
using ui::popup::chips;
using ui::popup::color;
using ui::popup::color_toggle;
using ui::popup::group;
using ui::popup::slider;
using ui::popup::toggle;
using ui::popup::value;

constexpr float k_top = layout::section_cap_y;

constexpr const char* k_materials[] = { "Disabled", "Flat", "Glossy", "Glow", "Metallic", "Wireframe",
    "Water Flow", "Glow Outline", "Iridescent" };
constexpr const char* k_targets[] = { "Enemies", "Teammates", "Local" };
constexpr const char* k_aspect_scaling[] = { "Horizontal", "Vertical", "Stretch" };
constexpr const char* k_spectating[] = { "Enemies", "Teammates", "Free Camera" };
constexpr const char* k_recoil[] = { "Remove Punch", "Remove Shake", "Remove Flinch" };
constexpr const char* k_scope_overlay[] = { "Default", "Remove", "Lines Only" };
constexpr const char* k_bomb_parts[] = { "Icon", "Text", "Progress Bar", "Time Remaining", "Damage", "Distance", "Glow" };
constexpr const char* k_weapon_parts[] = { "Icon", "Text", "Ammo", "Distance", "Glow" };
constexpr const char* k_hostage_parts[] = { "Icon", "Text", "Distance", "Glow" };
constexpr const char* k_grenade_parts[] = { "Icon",     "Text", "Progress Bar", "Time Remaining",
                                            "Distance", "Glow", "Trajectory" };
constexpr const char* k_removals[] = { "Smoke", "Flash", "Fog", "Post Processing", "Shadows", "Teammates",
                                       "Ragdolls", "Sleeves" };
constexpr const char* k_skyboxes[] = { "Default", "Night", "Cloudy", "Vertigo", "Daylight" };
constexpr const char* k_tracer_styles[] = { "Line", "Beam", "Glow" };

void color_toggle(const char* label, ColorFeature& f, const char* icon = nullptr, bool enabled = true)
{
    ui::popup::color_toggle(label, &f.color, &f.on, icon, enabled);
}

void esp_group(EspSettings& esp, std::span<const char* const> parts)
{
    group([&] {
        ui::popup::color_toggle("Enabled", &esp.color, &esp.enabled);
        chips(parts, esp.parts);
    });
}

// Small icon button of the preview toolbars.
bool icon_button(ImDrawList* dl, const char* id_str, const ImVec2& center, const char* glyph, bool active = false)
{
    const float hit = px(9.0f);
    // The label alone is not a unique ID: the same glyph button is used in more than
    // one place inside a window, and ImGui then reports two visible items fighting
    // over one ID. Folding the button's position in makes the ID unique by layout.
    ImGui::PushID(static_cast<int>(center.x * 4.0f) * 100000 + static_cast<int>(center.y * 4.0f));
    const ImGuiID id = ImGui::GetID(id_str);
    ImGui::PopID();
    const ui::ItemState s =
        ui::item_behavior(id, ImVec2(center.x - hit, center.y - hit), ImVec2(center.x + hit, center.y + hit));
    const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
    const ImU32 base = active ? col::tab_text_active : col::window_icon;
    draw::any_icon(dl, center, draw::lerp_color(base, col::tab_text_active, h), glyph, font_size::icon);
    return s.pressed;
}

} // namespace

void Menu::tab_visuals(const ImVec2& o, Config& c)
{
    if (visuals_subtab_ == 0)
        visuals_players(o, c.visuals.players);
    else
        visuals_world(o, c.visuals.world);
}

void Menu::visuals_players(const ImVec2& o, PlayersSettings& p)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float next = 0.0f;
    {
        Panel panel(o, Column::left, k_top, "ENEMY", 3);
        panel.toggle("Enabled", &p.enabled);
        panel.color_toggle("Offscreen Arrow", &p.offscreen_arrow.color, &p.offscreen_arrow.on);
        panel.toggle("Sounds", &p.sounds.on, [&] {
            group([&] { color("Color", &p.sounds.color); });
            group([&] {
                slider("Duration", &p.sounds.duration, 0.2f, 6.0f, "%.1fs", 0.1f);
                slider("Radius", &p.sounds.radius, 4.0f, 200.0f, "%.0f", 1.0f);
            });
            group([&] { toggle("Local Sync", &p.sounds.local_sync); });
        });
        next = panel.next_header_cap_y();
    }
    {
        Panel panel(o, Column::left, next, "ENEMY MODEL", 7);
        panel.color_combo("Player", &p.player.color, &p.player.material, k_materials);
        panel.color_combo("Behind Walls", &p.behind_walls.color, &p.behind_walls.material, k_materials);
        panel.color_combo("On Shot", &p.on_shot.color, &p.on_shot.material, k_materials);
        panel.color_combo("History", &p.history.color, &p.history.material, k_materials);
        panel.color_combo("Ragdolls", &p.ragdolls.color, &p.ragdolls.material, k_materials);
        panel.color_toggle("Soul Particles", &p.soul_particles.color, &p.soul_particles.on);
        panel.color_toggle("Glow", &p.glow.color, &p.glow.on);
    }
    if (search::collecting())
        return;

    // Model preview: target switch and toolbar on top, the model below.
    const auto at = [&](float x, float y) { return ImVec2(o.x + snap(px(x)), o.y + snap(px(y))); };
    ImFont* font = fonts().medium;
    const char* target = k_targets[p.target];
    const float text_w = draw::text_width(font, font_size::text, target);
    const float pill_h = snap(px(18.0f));
    const ImVec2 pill_min = at(437.0f, 52.0f);
    const ImVec2 pill_max(snap(pill_min.x + px(27.0f) + text_w + px(8.0f)), pill_min.y + pill_h);
    const ImGuiID pill_id = ImGui::GetID("##target");
    const ui::ItemState ps = ui::item_behavior(pill_id, pill_min, pill_max);
    if (ps.pressed)
        p.target = (p.target + 1) % IM_ARRAYSIZE(k_targets);
    const float ph = ui::animate(pill_id + 1, ps.hovered ? 1.0f : 0.0f, 16.0f);
    dl->AddRectFilled(pill_min, pill_max, draw::lerp_color(col::chip_bg_selected, col::chip_bg_hovered, ph), px(4.0f));
    const float pcy = (pill_min.y + pill_max.y) * 0.5f;
    draw::icon(dl, fonts().icons, font_size::icon, ImVec2(pill_min.x + px(12.0f), pcy), col::window_label, icon::user);
    draw::text(dl, font, font_size::text, ImVec2(pill_min.x + px(23.0f), pcy - draw::cap_height(font, font_size::text) * 0.5f),
               col::tab_text_active, target);
    icon_button(dl, "##hide", ImVec2(pill_max.x + px(17.0f), pcy), icon::user_minus);
    const ImGuiID items_id = ImGui::GetID("##esp_items");
    const ImVec2 list_c(at(551.0f, 0.0f).x, pcy);
    if (icon_button(dl, "##list", list_c, icon::list, ImGui::IsPopupOpen(items_id, ImGuiPopupFlags_None)))
        ImGui::OpenPopup(items_id);
    // Universal scaling, as in the reference: on means the wheel resizes every element.
    if (icon_button(dl, "##expand", ImVec2(at(574.0f, 0.0f).x, pcy), icon::expand, esp_uniform_scale_))
        esp_uniform_scale_ = !esp_uniform_scale_;

    // The reference renders the enemy as a grey material preview; the ESP is laid out around it.
    // The live model (chams material included) replaces the shipped picture once the
    // offscreen renderer has a mesh; until then the picture stays as the fallback.
    const ImVec2 area_min = at(420.0f, 100.0f), area_max = at(568.0f, 388.0f);
    ImVec2 model_min, model_max;
    images::fit(inv::players_preview_image, area_min, area_max, true, &model_min, &model_max);
    // The ESP preview always shows the enemy agent: the inventory may have pointed the
    // shared renderer at another model.
    esp_model::set_model(nullptr);
    if (!draw_esp_model(dl, p, model_min, model_max))
        images::draw(dl, inv::players_preview_image, area_min, area_max, IM_COL32(205, 207, 214, 255), true);
    draw_esp_preview(dl, o, p, model_min, model_max);
    draw_esp_items(p, items_id, ImVec2(list_c.x + px(9.0f), list_c.y));
}

void Menu::visuals_world(const ImVec2& o, WorldSettings& w)
{
    // The Ambience view lies over the World panels: they stay drawn underneath (and get
    // blurred by the view) but take no input.
    const bool overlay = world_view_ == 1 && !search::collecting();
    if (overlay)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    float next_left = 0.0f, next_right = 0.0f;
    {
        Panel p(o, Column::left, k_top, "VIEW", 6);
        p.chevron("View Options", [&] {
            group([&] {
                toggle("Override FOV", &w.override_fov);
                slider("Value", &w.fov, 60.0f, 130.0f, "%.0f\xc2\xb0", 1.0f, w.override_fov);
            });
            group([&] {
                toggle("Override Aspect Ratio", &w.override_aspect);
                value("Scaling", &w.aspect_scaling, k_aspect_scaling, w.override_aspect);
                slider("Value", &w.aspect, 50.0f, 200.0f, "%.0f%%", 1.0f, w.override_aspect);
            });
            group([&] { toggle("Disable Landing Bob", &w.disable_landing_bob); });
        });
        p.chevron("Scope Options", [&] {
            group([&] {
                toggle("Remove Overlay", &w.remove_scope_overlay);
                color_toggle("Custom Lines", w.scope_lines, nullptr, w.remove_scope_overlay);
                slider("Line Length", &w.scope_line_length, 5.0f, 100.0f, "%.0f%%", 1.0f,
                       w.remove_scope_overlay && w.scope_lines.on);
            });
            group([&] {
                toggle("Disable Zoom Blur", &w.disable_zoom_blur);
                toggle("Instant Unscope", &w.instant_unscope);
            });
        });
        p.chevron("Viewmodel Options", [&] {
            group([&] {
                toggle("Override Viewmodel", &w.override_viewmodel);
                slider("Offset X", &w.viewmodel_x, -10.0f, 10.0f, "%.0f", 1.0f, w.override_viewmodel);
                slider("Offset Y", &w.viewmodel_y, -10.0f, 10.0f, "%.0f", 1.0f, w.override_viewmodel);
                slider("Offset Z", &w.viewmodel_z, -10.0f, 10.0f, "%.0f", 1.0f, w.override_viewmodel);
                slider("Field of View", &w.viewmodel_fov, 54.0f, 90.0f, "%.0f\xc2\xb0", 1.0f, w.override_viewmodel);
            });
            group([&] { toggle("Show In Scope", &w.viewmodel_in_scope); });
        });
        p.chevron("Perspective Options", [&] {
            group([&] {
                toggle("Thirdperson", &w.thirdperson);
                slider("Distance", &w.thirdperson_distance, 30.0f, 300.0f, "%.0f", 1.0f, w.thirdperson);
                toggle("Collision", &w.thirdperson_collision, nullptr, w.thirdperson);
            });
            group([&] { toggle("While Dead", &w.thirdperson_dead); });
        });
        p.multi("Unlock Spectating", k_spectating, w.unlock_spectating);
        p.multi("Visual Recoil", k_recoil, w.visual_recoil);
        next_left = p.next_header_cap_y();
    }
    {
        // Six rows: Radar, Preserve Killfeed, Inaccuracy Overlay, Scope Overlay,
        // Scoreboard, Crosshairs. Ambience and Weather moved into their own view,
        // which the sidebar entry opens.
        Panel p(o, Column::right, k_top, "HUD", 6);
        p.toggle("Radar", &w.radar);
        p.toggle("Preserve Killfeed", &w.preserve_killfeed);
        p.color_toggle("Inaccuracy Overlay", &w.inaccuracy_overlay.color, &w.inaccuracy_overlay.on);
        p.combo("Scope Overlay", &w.scope_overlay, k_scope_overlay);
        p.chevron("Scoreboard", [&] {
            group([&] {
                toggle("Reveal Ranks", &w.reveal_ranks);
                toggle("Reveal Money", &w.reveal_money);
                toggle("Show Wins", &w.show_wins);
            });
        });
        p.chevron("Crosshairs", [&] {
            group([&] {
                toggle("Sniper Crosshair", &w.sniper_crosshair);
                color_toggle("Penetration Crosshair", w.penetration_crosshair);
                toggle("Recoil Crosshair", &w.recoil_crosshair);
            });
        });
        next_right = p.next_header_cap_y();
    }
    {
        // The Ambience view is a whole screen of the menu, not a dropdown: the row
        // below opens it and the sidebar's World entry brings the panels back.
        Panel p(o, Column::right, next_right, "EFFECTS", 1);
        if (p.open_row("Ambience")) {
            world_view_ = 1;
            ambience_fade_ = 0.0f;
            ambience_menu_ = -1;
        }
        next_right = p.next_header_cap_y();
    }
    {
        Panel p(o, Column::left, next_left, "WORLD ESP", 6);
        p.chevron("Bomb", [&] {
            group([&] {
                color_toggle("Colorize Effects", w.bomb_colorize, icon::sparkles);
                toggle("Bomb Indicator", &w.bomb_indicator,
                       [&] { group([&] { toggle("Show Timer", &w.bomb_indicator_timer); }); }, icon::bomb);
                color_toggle("Defuse Indicator", w.defuse_indicator, icon::scissors);
                color_toggle("Defuser", w.defuser, icon::scissors);
            });
            group([&] {
                ui::popup::color_toggle("Enabled", &w.bomb.color, &w.bomb.enabled);
                color("Bomb Defused", &w.bomb_defused_color);
                chips(k_bomb_parts, w.bomb.parts);
            });
        });
        p.chevron("Weapons", [&] {
            group([&] { color_toggle("Colorize Effects", w.weapons_colorize, icon::sparkles); });
            esp_group(w.weapons, k_weapon_parts);
        });
        p.chevron("Hostages", [&] { esp_group(w.hostages, k_hostage_parts); });
        p.chevron("Grenades", [&] {
            group([&] {
                color_toggle("Smoke Radius", w.smoke_radius, icon::dashed_circle);
                color_toggle("Molotov Radius", w.molotov_radius, icon::rings);
                color_toggle("Colorize Smoke", w.colorize_smoke, icon::cloud);
                color_toggle("Colorize Molotov", w.colorize_molotov, icon::flame);
                color_toggle("Colorize Explosion", w.colorize_explosion, icon::burst);
            });
            esp_group(w.grenades, k_grenade_parts);
        });
        p.toggle("Grenade Trajectory", &w.grenade_trajectory, [&] {
            group([&] {
                color("Line", &w.trajectory_color);
                toggle("Show Bounces", &w.trajectory_bounces);
                slider("Thickness", &w.trajectory_thickness, 1.0f, 5.0f, "%.0f px");
            });
        });
        p.toggle("Grenade Proximity Warnings", &w.grenade_proximity);
    }
    {
        // Three rows plus the header: Windows, Hit Marker, Bullet Tracers (Bullet
        // Impacts moved into that window so the column fits the menu height).
        Panel p(o, Column::right, next_right, "MISCELLANEOUS", 3);
        p.chevron("Windows", [&] {
            group([&] {
                toggle("Spectators", &w.window_spectators, icon::user);
                toggle("Keybinds", &w.window_keybinds, icon::list);
                toggle("Watermark", &w.window_watermark, [&] { watermark_settings(); }, icon::copyright);
                toggle("Bomb Timer", &w.window_bomb, icon::bomb);
            });
        });
        p.chevron("Hit Marker", [&] {
            group([&] {
                color_toggle("Crosshair", w.hit_marker_crosshair);
                color_toggle("World", w.hit_marker_world);
                color_toggle("Damage", w.hit_marker_damage);
            });
            group([&] { slider("Duration", &w.hit_marker_duration, 10.0f, 200.0f, "%.0f"); });
        });
        p.chevron("Bullet Tracers", [&] {
            group([&] {
                color_toggle("Local", w.tracers_local);
                color_toggle("Enemies", w.tracers_enemies);
                color_toggle("Teammates", w.tracers_team);
            });
            group([&] {
                value("Style", &w.tracer_style, k_tracer_styles);
                slider("Duration", &w.tracer_duration, 10.0f, 100.0f, "%.0f");
            });
            group([&] { color_toggle("Bullet Impacts", w.bullet_impacts); });
        });
    }

    if (search::collecting()) {
        // The Ambience rows are searchable from any view, under World > Ambience.
        search::push("Ambience");
        ambience_effects(o, w);
        search::pop();
        return;
    }
    if (overlay) {
        ImGui::PopItemFlag();
        draw_ambience(o, w);
    }
}

} // namespace nl
