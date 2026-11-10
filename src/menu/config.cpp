// Menu source watermark: @use_sberbank
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "menu/config.h"

#include "menu/fonts.h"

namespace nl {

namespace {
constexpr ImU32 k_white = IM_COL32(236, 238, 245, 255);
constexpr ImU32 k_pink = IM_COL32(240, 112, 146, 255);
} // namespace

const EspInfo esp_info[esp_element_count] = {
    { "Bounding Box", 0, EspKind::box, EspSide::top, false, k_white, nullptr },
    { "Name", 0, EspKind::text, EspSide::top, true, k_white, "камшако" },
    { "Distance", 0, EspKind::text, EspSide::bottom, false, IM_COL32(200, 203, 214, 255), "12 M" },
    { "Health Bar", 0, EspKind::bar, EspSide::left, true, IM_COL32(118, 226, 120, 255), nullptr },
    { "Ammo Bar", 0, EspKind::bar, EspSide::bottom, false, IM_COL32(110, 160, 250, 255), nullptr },
    { "Skeleton", 0, EspKind::skeleton, EspSide::top, false, k_white, nullptr },
    { "Unarmored", 1, EspKind::flag, EspSide::right, false, k_white, "NOARM" },
    { "Defuser", 1, EspKind::flag, EspSide::right, true, k_white, "KIT" },
    { "Blind", 1, EspKind::flag, EspSide::right, true, k_white, "BLIND" },
    { "Scoped", 1, EspKind::flag, EspSide::right, true, k_white, "ZOOM" },
    { "Reload", 1, EspKind::flag, EspSide::right, true, k_white, "RELOAD" },
    { "Immunity", 1, EspKind::flag, EspSide::right, true, k_white, "IMMUNE" },
    { "Slowed", 1, EspKind::flag, EspSide::right, false, k_white, "SLOW" },
    { "Vulnerable", 1, EspKind::flag, EspSide::right, false, k_pink, "VULN" },
    { "Bomb", 1, EspKind::flag, EspSide::right, false, k_pink, "C4" },
    { "Hostage", 1, EspKind::flag, EspSide::right, false, k_white, "HOST" },
    { "Defuse", 1, EspKind::flag, EspSide::right, true, k_pink, "DEFUSE" },
    { "Pin Pulled", 1, EspKind::flag, EspSide::right, true, k_pink, "PIN" },
    { "Money", 1, EspKind::flag, EspSide::right, false, IM_COL32(130, 222, 130, 255), "$4750" },
    { "Order Priority", 1, EspKind::flag, EspSide::right, false, k_white, "1ST" },
    { "Delay", 1, EspKind::flag, EspSide::right, false, k_white, "DELAY" },
    { "Text", 2, EspKind::text, EspSide::bottom, true, IM_COL32(214, 217, 228, 255), "AKR" },
    { "Icon", 2, EspKind::icon, EspSide::bottom, false, k_white, icon::gun },
    { "Readiness Bar", 2, EspKind::bar, EspSide::bottom, false, IM_COL32(236, 196, 96, 255), nullptr },
    { "Bomb", 2, EspKind::icon, EspSide::bottom, false, k_pink, icon::bomb },
    { "Defuser", 2, EspKind::icon, EspSide::bottom, false, k_white, icon::scissors },
    { "Taser", 2, EspKind::icon, EspSide::bottom, false, IM_COL32(250, 214, 90, 255), icon::bolt },
    { "Hitboxes", 3, EspKind::hitboxes, EspSide::top, false, IM_COL32(120, 170, 255, 255), nullptr },
    { "Armor Bar", 0, EspKind::bar, EspSide::bottom, false, IM_COL32(110, 160, 250, 255), nullptr },
};

// The MAP SELECTION list always starts with the Global fallback entry.
void init_ambience_maps(WorldSettings& w)
{
    if (w.map_count < 1) {
        w.map_count = 1;
        w.selected_map = 0;
    }
    if (w.maps[0].map[0] == 0)
        std::strncpy(w.maps[0].map, "Global", ambience_map_name_len - 1);
    w.maps[0].used = true;
    w.map_count = std::min(w.map_count, ambience_map_count);

    // Older builds seeded the whole competitive pool as copies of Global. The reference
    // list only holds Global plus the maps you configured, so untouched seeds are dropped.
    static constexpr const char* k_pool[] = {
        "de_mirage", "de_dust2", "de_inferno", "de_nuke", "de_overpass",
        "de_ancient", "de_anubis", "de_vertigo", "de_train", "cs_office",
        "de_cache", "de_cbble",
    };
    int kept = 1;
    for (int index = 1; index < w.map_count; ++index) {
        const AmbienceMap& entry = w.maps[static_cast<size_t>(index)];
        bool seeded = false;
        for (const char* name : k_pool)
            seeded = seeded || std::strcmp(entry.map, name) == 0;
        // A seed was a copy of Global taken back then; it is untouched when it still
        // equals Global or turns nothing on.
        const AmbienceSettings& a = entry.ambience;
        bool any = a.nightmode || a.fullbright || a.sky || a.sunlight || a.fog || a.dof;
        for (bool removal : a.removals)
            any = any || removal;
        seeded = seeded && (!any || std::memcmp(&a, &w.maps[0].ambience, sizeof(AmbienceSettings)) == 0);
        if (entry.map[0] == 0 || seeded)
            continue;
        if (kept != index)
            w.maps[static_cast<size_t>(kept)] = entry;
        ++kept;
    }
    for (int index = kept; index < w.map_count; ++index)
        w.maps[static_cast<size_t>(index)] = AmbienceMap{};
    w.map_count = kept;
    w.selected_map = std::clamp(w.selected_map, 0, w.map_count - 1);
}

EspLayout default_esp()
{
    EspLayout out{};
    for (int i = 0; i < esp_element_count; ++i) {
        const EspInfo& info = esp_info[i];
        EspElement& e = out[static_cast<size_t>(i)];
        e.on = info.on;
        e.side = info.side;
        e.order = static_cast<float>(i);
        e.color = ImGui::ColorConvertU32ToFloat4(info.color);
    }
    return out;
}

std::array<std::array<inv::Equip, inv::slot_count>, 2> default_loadout()
{
    std::array<std::array<inv::Equip, inv::slot_count>, 2> out{};
    for (int side = 0; side < 2; ++side)
        for (int slot = 0; slot < inv::slot_count; ++slot)
            out[static_cast<size_t>(side)][static_cast<size_t>(slot)] = inv::default_loadouts[side][slot];
    return out;
}

std::array<Config, preset_count> make_presets()
{
    std::array<Config, preset_count> presets{};

    // Index 0 is the state shown in the reference screenshots; the defaults already match it,
    // except for the weapon groups that have their own selection settings.
    Config& premier = presets[0];
    premier.visuals.world.window_watermark = true;
    for (int w : { 8, 9 }) { // SSG 08, AWP
        RageWeapon& s = premier.rage.weapons[static_cast<size_t>(w)];
        s.hit_chance = w == 8 ? 72.0f : 85.0f;
        s.min_damage = w == 8 ? 80.0f : 100.0f;
        s.hitboxes[1] = s.hitboxes[2] = true;
        s.hitboxes[0] = w == 8;
        s.custom = true;
    }

    Config& dt = presets[1];
    dt.rage.enabled = true;
    dt.rage.double_tap = true;
    dt.rage.remove_spread = 2;
    dt.misc.slow_walk = true;

    Config& removed_fl = presets[2];
    removed_fl.rage.enabled = true;
    removed_fl.rage.double_tap = true;
    removed_fl.rage.double_precision = true;
    removed_fl.misc.slow_walk = true;
    removed_fl.misc.no_fall_damage = true;

    Config& hvh = presets[3];
    hvh.rage.enabled = true;
    hvh.rage.history = 1;
    hvh.rage.weapons[0].hit_chance = 55.0f;
    hvh.misc.jump_bug = false;
    hvh.misc.edge_jump = true;
    hvh.misc.hit_sound = false;

    Config& legit = presets[4];
    legit.rage.enabled = false;
    legit.rage.anti_aim = false;
    legit.legit.enabled = true;
    legit.legit.triggerbot = true;
    legit.visuals.world.night_mode.on = false;
    legit.visuals.world.remove_scope_overlay = false;
    legit.misc.air_strafe = false;
    legit.misc.jump_bug = false;
    legit.misc.standalone_quick_stop = false;
    legit.misc.strafe_assist = false;
    legit.misc.automatic_grenade_release = false;

    Config& semi = presets[5];
    semi.rage.silent_aim = false;
    semi.rage.field_of_view = 30.0f;
    semi.rage.anti_aim = false;
    semi.legit.enabled = true;
    semi.misc.jump_bug = false;
    semi.misc.hit_sound = false;

    // Every preset starts with the Global entry of the MAP SELECTION list.
    for (Config& preset : presets)
        init_ambience_maps(preset.visuals.world);

    return presets;
}

} // namespace nl
