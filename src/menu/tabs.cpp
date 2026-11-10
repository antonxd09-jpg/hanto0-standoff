// Menu source watermark: @use_sberbank
#include "imgui_internal.h"
#include "menu/menu.h"
#include "menu/panel.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using ui::popup::check;
using ui::popup::chips;
using ui::popup::combo;
using ui::popup::group;
using ui::popup::multi_combo;
using ui::popup::slider;
using ui::popup::toggle;
using ui::popup::value;

constexpr float k_top = style::layout::section_cap_y;

constexpr const char* k_hitboxes[] = { "Head", "Chest", "Stomach", "Arms", "Legs", "Feet" };
constexpr const char* k_quick_stop[] = { "Early", "Between Shots", "Force Accuracy", "In Air" };
constexpr const char* k_history[] = { "Disabled", "Minimum", "Maximum" };
constexpr const char* k_delay_shot[] = { "On Peek", "Unlag", "Lethal" };
constexpr const char* k_remove_spread[] = { "Disabled", "Partial", "Full" };
constexpr const char* k_yaw_base[] = { "Local View", "At Targets" };
constexpr const char* k_pitch[] = { "Disabled", "Down", "Up", "Zero", "Random" };
constexpr const char* k_yaw[] = { "Backward", "Static", "Spin", "Jitter" };
constexpr const char* k_yaw_modifiers[] = { "Disabled", "Center", "Offset", "Random", "Spin" };
constexpr float k_limiters_w = 225.0f;
constexpr const char* k_targets[] = { "Head", "Neck", "Chest", "Nearest" };
constexpr const char* k_wall_policy[] = { "Block", "Penetrate", "Ignore" };
constexpr const char* k_seed_modes[] = { "None", "Restricted", "Unrestricted" };
constexpr const char* k_jump_bug_modes[] = { "Always", "On Key" };
constexpr const char* k_hit_sounds[] = { "Bell", "Bubble", "Cod", "Fatality", "Switch" };
constexpr const char* k_primary[] = { "None", "AWP", "SSG 08", "Auto Sniper", "AK-47 / M4" };
constexpr const char* k_secondary[] = { "None", "Desert Eagle", "Dual Berettas", "Five-SeveN / Tec-9", "P250" };
constexpr const char* k_utility[] = { "Armor", "Helmet", "Defuse Kit", "Grenades", "Zeus x27" };
constexpr const char* k_log_events[] = { "Damage Dealt", "Damage Taken", "Purchases", "Bomb", "Misses" };

} // namespace

void Menu::tab_rage(const ImVec2& o, Config& c)
{
    RageSettings& r = c.rage;
    const int weapon_index = ImClamp(r.weapon, 0, rage_weapon_count - 1);
    RageWeapon& w = r.weapons[static_cast<size_t>(weapon_index)];
    const RageWeapon& global = r.weapons[0];
    // A group shows Global's values until one of them is changed here; from then on it
    // keeps its own. Matching Global again hands it back.
    if (weapon_index != 0 && !w.custom)
        copy_selection(w, global);
    struct Inherit {
        RageWeapon& w;
        const RageWeapon& global;
        bool on;
        ~Inherit()
        {
            if (on)
                w.custom = !same_selection(w, global);
        }
    } inherit{ w, global, weapon_index != 0 };
    float next = 0.0f;
    {
        Panel p(o, Column::left, k_top, "MAIN", 6);
        p.toggle("Enabled", &r.enabled);
        p.toggle("Silent Aim", &r.silent_aim);
        p.toggle("Automatic Fire", &r.auto_fire);
        p.toggle(
            "Limiters", &r.limiters,
            [&] {
                group([&] {
                    slider("Aim FOV", &r.aim_fov, 0.0f, 180.0f, "%.0f\xc2\xb0");
                    slider("Aim Speed", &r.aim_speed, 0.0f, 100.0f, "%.0f%%");
                    slider("Reaction Time", &r.reaction_time, 0.0f, 500.0f, "%.0f ms", 5.0f);
                    slider("Kill Delay", &r.kill_delay, 0.0f, 1000.0f, "%.0f ms", 10.0f);
                    slider("Max Walls", &r.max_walls, 0.0f, 4.0f, "%.0f");
                    slider("Backtrack", &r.backtrack, 0.0f, 200.0f, "%.0f ms");
                    toggle("Selftrack", &r.selftrack);
                    toggle("Spoof FPS", &r.spoof_fps);
                });
            },
            true, k_limiters_w);
        p.toggle("Refine Shot", &r.refine_shot);
        p.slider("Field of View", &r.field_of_view, 0.0f, 180.0f, "%.1f\xc2\xb0", 0.1f);
        next = p.next_header_cap_y();
    }
    {
        Panel p(o, Column::left, next, weapon_index != 0 && !w.custom ? "SELECTION (GLOBAL)" : "SELECTION", 6);
        p.multi("Hitboxes", k_hitboxes, w.hitboxes, "None");
        p.slider("Extrapolation", &w.extrapolation, 0.0f, 100.0f, "%.0f%%");
        p.slider("Hit Chance", &w.hit_chance, 0.0f, 100.0f, "%.0f%%", 1.0f, [&] {
            group([&] {
                slider("Double Tap", &w.hit_chance_double_tap, 0.0f, 100.0f, "%.0f%%");
                toggle("Strict", &w.hit_chance_strict);
            });
        });
        p.slider("Min Damage", &w.min_damage, 0.0f, 100.0f, "%.0f");
        p.toggle("Quick Stop", &w.quick_stop, [&] {
            group([&] {
                for (int i = 0; i < IM_ARRAYSIZE(k_quick_stop); ++i)
                    check(k_quick_stop[i], &w.quick_stop_options[i]);
            });
        });
        p.toggle("Quick Scope", &w.quick_scope);
    }
    {
        Panel p(o, Column::right, k_top, "OTHER", 7);
        p.combo("History", &r.history, k_history);
        p.multi("Delay Shot", k_delay_shot, r.delay_shot, [&] {
            group([&] { slider("Delay", &r.delay_shot_time, 0.0f, 200.0f, "%.0f ms"); });
        });
        p.combo("Remove Spread", &r.remove_spread, k_remove_spread);
        p.toggle("Duck Peek Assist", &r.duck_peek_assist);
        p.toggle("Quick Peek Assist", &r.quick_peek_assist, [&] {
            group([&] {
                ui::popup::color("Color", &r.quick_peek_color);
                toggle("Retreat On Shot", &r.quick_peek_retreat);
                toggle("Auto Stop", &r.quick_peek_auto_stop);
            });
        });
        p.toggle("Double Precision", &r.double_precision);
        p.toggle("Double Tap", &r.double_tap);
        next = p.next_header_cap_y();
    }
    {
        Panel p(o, Column::right, next, "ANTI-AIM", 5);
        p.toggle("Enabled", &r.anti_aim, [&] {
            group([&] {
                toggle("Disable On Use", &r.anti_aim_on_use);
                toggle("Disable On Freeze Time", &r.anti_aim_freeze_time);
            });
        });
        p.combo("Yaw Base", &r.yaw_base, k_yaw_base);
        p.chevron("Pitch", [&] {
            group([&] {
                combo("Mode", &r.pitch, k_pitch);
                slider("Offset", &r.pitch_offset, -89.0f, 89.0f, "%.0f\xc2\xb0", 1.0f, r.pitch == 4);
            });
        });
        p.chevron("Yaw", [&] {
            group([&] {
                combo("Mode", &r.yaw, k_yaw);
                slider("Offset", &r.yaw_offset, -180.0f, 180.0f, "%.0f\xc2\xb0");
            });
            group([&] {
                combo("Modifier", &r.yaw_modifier, k_yaw_modifiers);
                slider("Degree", &r.yaw_modifier_degree, 0.0f, 180.0f, "%.0f\xc2\xb0", 1.0f, r.yaw_modifier != 0);
            });
        });
        p.chevron("Mouse Override", [&] {
            group([&] {
                toggle("Enabled", &r.mouse_override);
                slider("Speed", &r.mouse_override_speed, 0.0f, 100.0f, "%.0f%%", 1.0f, r.mouse_override);
                toggle("Return To Base", &r.mouse_override_return, nullptr, r.mouse_override);
            });
        });
    }
}

void Menu::tab_legit(const ImVec2& o, Config& c)
{
    LegitSettings& l = c.legit;
    {
        Panel p(o, Column::left, k_top, "AIMBOT", 11);
        p.toggle("Enabled", &l.enabled);
        p.slider("Field of View", &l.field_of_view, 0.0f, 30.0f, "%.0f\xc2\xb0");
        p.slider("Smoothing", &l.smoothing, 0.0f, 100.0f, "%.0f%%");
        p.slider("Humanize", &l.humanize, 0, 100, "%.0f%%");
        p.toggle("Recoil Control", &l.recoil_control, [&] {
            group([&] {
                slider("Horizontal", &l.recoil_x, 0.0f, 100.0f, "%.0f%%");
                slider("Vertical", &l.recoil_y, 0.0f, 100.0f, "%.0f%%");
            });
        });
        p.combo("Target", &l.target, k_targets);
        p.chevron("Hitboxes", [&] {
            group([&] { chips(k_hitboxes, l.hitboxes); });
        });
        p.chevron("Limits", [&] {
            group([&] {
                toggle("Auto Wall", &l.autowall);
                toggle("Multi Point", &l.multipoint);
                toggle("Lethal Only", &l.lethal_only);
                slider("Min Damage", &l.min_damage, 0.0f, 130.0f, "%.0f");
            });
            group([&] {
                value("Walls", &l.wall_policy, k_wall_policy);
                toggle("Airborne", &l.airborne);
                slider("Reaction Time", &l.reaction_time, 0.0f, 250.0f, "%.0f ms");
            });
        });
        p.toggle("Visible Only", &l.visible_only);
        p.toggle("Smoke Check", &l.smoke_check);
        p.toggle("Flash Check", &l.flash_check);
    }
    float next = 0.0f;
    {
        Panel p(o, Column::right, k_top, "TRIGGERBOT", 5);
        p.toggle("Enabled", &l.triggerbot);
        p.slider("Delay", &l.trigger_delay, 0.0f, 250.0f, "%.0f ms");
        p.slider("Hit Chance", &l.trigger_hit_chance, 0.0f, 100.0f, "%.0f%%");
        p.toggle("Magnet", &l.trigger_magnet);
        p.chevron("Checks", [&] {
            group([&] {
                toggle("Smoke Check", &l.trigger_smoke_check);
                toggle("Flash Check", &l.trigger_flash_check);
                toggle("Auto Wall", &l.trigger_autowall);
                toggle("Lethal Only", &l.trigger_lethal_only);
            });
            group([&] {
                toggle("Min Damage", &l.trigger_min_damage_enabled,
                       [&] { slider("Value", &l.trigger_min_damage, 0.0f, 130.0f, "%.0f"); });
                toggle("Auto Stop", &l.trigger_autostop);
                value("Seed", &l.trigger_seed_type, k_seed_modes);
            });
        });
        next = p.next_header_cap_y();
    }
    {
        Panel p(o, Column::right, next, "BACKTRACK", 2);
        p.toggle("Enabled", &l.backtrack);
        p.slider("Time", &l.backtrack_time, 0.0f, 400.0f, "%.0f ms");
    }
}

void Menu::tab_misc(const ImVec2& o, Config& c)
{
    MiscSettings& m = c.misc;
    {
        Panel p(o, Column::left, k_top, "MOVEMENT", 10);
        p.toggle("Bunny Hop", &m.bunny_hop);
        p.toggle("Air Strafe", &m.air_strafe, [&] {
            group([&] {
                check("Avoid Collisions", &m.air_strafe_avoid_collisions, false);
                check("View Angles", &m.air_strafe_view_angles);
                check("Movement Keys", &m.air_strafe_movement_keys);
                check("Only While Holding Jump", &m.air_strafe_only_jump);
                check("Stop On Shift Key", &m.air_strafe_stop_shift);
                check("Stop On Key Release", &m.air_strafe_stop_release);
            });
            group([&] {
                toggle("Subtick Mode", &m.air_strafe_subtick);
                slider("Turn Speed", &m.air_strafe_turn_speed, 0.0f, 100.0f, "%.0f%%");
            });
        });
        p.toggle("Jump Bug", &m.jump_bug, [&] {
            group([&] {
                combo("Mode", &m.jump_bug_mode, k_jump_bug_modes);
                toggle("Only on Landing", &m.jump_bug_land);
            });
        });
        p.toggle("No Fall Damage", &m.no_fall_damage);
        p.toggle("Standalone Quick Stop", &m.standalone_quick_stop);
        p.toggle("Strafe Assist", &m.strafe_assist);
        p.toggle("Edge Jump", &m.edge_jump);
        p.toggle("Slow Walk", &m.slow_walk, [&] {
            group([&] {
                slider("Speed", &m.slow_walk_speed, 5.0f, 100.0f, "%.0f%%");
                toggle("Randomize", &m.slow_walk_random);
            });
        });
        p.toggle("Fast Ladder", &m.fast_ladder);
        p.toggle("Air Duck", &m.air_duck);
    }
    {
        Panel p(o, Column::right, k_top, "FEATURES", 9);
        p.toggle("Quick Switch", &m.quick_switch);
        p.toggle("Super Toss", &m.super_toss);
        p.toggle("Knife Bot", &m.knife_bot, [] {}, false);
        p.toggle("Prevent AFK Kick", &m.prevent_afk_kick);
        p.toggle("Hit Sound", &m.hit_sound, [&] {
            group([&] {
                combo("Sound", &m.hit_sound_kind, k_hit_sounds);
                slider("Volume", &m.hit_sound_volume, 0.0f, 100.0f, "%.0f%%");
            });
        });
        p.toggle("Automatic Purchase", &m.automatic_purchase, [&] {
            group([&] {
                // combo_box() inside a settings popup crashed in the text drawing path
                // (fault landed in ucrtbase during the label measure). The value row is
                // the same control the weapon panels use and opens its list as a nested
                // popup, so the buy list keeps working while the combo bug is chased.
                combo("Primary", &m.buy_primary, k_primary);
                combo("Secondary", &m.buy_secondary, k_secondary);

                multi_combo("Utility", k_utility, m.buy_utility);
            });
        });
        p.toggle("Automatic Grenade Release", &m.automatic_grenade_release, [&] {
            group([&] { slider("Minimum Damage", &m.grenade_release_damage, 1.0f, 100.0f, "%.0f"); });
        });
        p.toggle("Auto-Accept Matchmaking", &m.auto_accept_matchmaking);
        p.color_multi("Log Events", &m.log_events_color, k_log_events, m.log_events);
    }
}

} // namespace nl
