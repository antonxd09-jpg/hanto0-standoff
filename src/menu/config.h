// Menu source watermark: @use_sberbank
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "imgui.h"

#include "menu/inventory_catalog.h"

namespace nl {

std::array<std::array<inv::Equip, inv::slot_count>, 2> default_loadout();

// Weapon groups of the Rage tab; "Global" applies to every weapon without its own settings.
inline constexpr const char* rage_weapons[] = { "Global",  "Pistols", "Desert Eagle", "R8 Revolver",
                                                "SMG",     "Rifles",  "Shotguns",     "Machine Guns",
                                                "SSG 08",  "AWP",     "Auto Snipers", "Zeus x27" };
inline constexpr int rage_weapon_count = static_cast<int>(sizeof(rage_weapons) / sizeof(rage_weapons[0]));

// SELECTION panel, per weapon group.
struct RageWeapon {
    bool hitboxes[6] = { true, true, true, false, false, false }; // Head, Chest, Stomach, Arms, Legs, Feet
    float extrapolation = 50.0f;
    float hit_chance = 0.0f;
    float hit_chance_double_tap = 0.0f;
    bool hit_chance_strict = false;
    float min_damage = 40.0f;
    float min_damage_override = 100.0f;
    bool quick_stop = false;
    bool quick_stop_options[4] = { true, true, false, false }; // Early, Between Shots, Force Accuracy, In Air
    bool quick_scope = false;
    // Set once the group was edited away from Global; until then it shoots with Global's
    // settings. Sits in what was tail padding so saved configs keep their size.
    bool custom = false;
};
static_assert(sizeof(RageWeapon) == 40, "RageWeapon is stored byte-wise in saved configs");

// Everything but `custom`.
inline bool same_selection(const RageWeapon& a, const RageWeapon& b)
{
    for (int i = 0; i < 6; ++i)
        if (a.hitboxes[i] != b.hitboxes[i])
            return false;
    for (int i = 0; i < 4; ++i)
        if (a.quick_stop_options[i] != b.quick_stop_options[i])
            return false;
    return a.extrapolation == b.extrapolation && a.hit_chance == b.hit_chance
        && a.hit_chance_double_tap == b.hit_chance_double_tap && a.hit_chance_strict == b.hit_chance_strict
        && a.min_damage == b.min_damage && a.min_damage_override == b.min_damage_override
        && a.quick_stop == b.quick_stop && a.quick_scope == b.quick_scope;
}

inline void copy_selection(RageWeapon& to, const RageWeapon& from)
{
    const bool custom = to.custom;
    to = from;
    to.custom = custom;
}

struct RageSettings {
    // MAIN
    bool enabled = false;
    bool silent_aim = true;
    bool auto_fire = true;
    bool limiters = true;
    float aim_fov = 180.0f;
    float aim_speed = 100.0f;
    float reaction_time = 0.0f;
    float kill_delay = 0.0f;
    float max_walls = 4.0f;
    float backtrack = 200.0f;
    bool selftrack = false;
    bool spoof_fps = true;
    bool refine_shot = true;
    float field_of_view = 180.0f;

    int weapon = 0; // group edited in the SELECTION panel
    std::array<RageWeapon, rage_weapon_count> weapons{};

    // OTHER
    int history = 2;
    bool delay_shot[3] = {};
    float delay_shot_time = 50.0f;
    int remove_spread = 2;
    bool duck_peek_assist = false;
    bool quick_peek_assist = false;
    ImVec4 quick_peek_color{ 0.70f, 0.83f, 1.0f, 1.0f };
    bool quick_peek_retreat = true;
    bool quick_peek_auto_stop = false;
    bool double_precision = false;
    bool double_tap = false;

    // ANTI-AIM
    bool anti_aim = true;
    bool anti_aim_on_use = false;
    bool anti_aim_freeze_time = true;
    int yaw_base = 1;
    int pitch = 1;
    float pitch_offset = 89.0f;
    int yaw = 0;
    float yaw_offset = 0.0f;
    int yaw_modifier = 1;
    float yaw_modifier_degree = 32.0f;
    bool mouse_override = false;
    float mouse_override_speed = 60.0f;
    bool mouse_override_return = true;
};

struct LegitSettings {
    bool enabled = false;
    float field_of_view = 4.0f;
    float smoothing = 35.0f;
    float humanize = 35; // aimbot humanizer strength, 0 = mechanical
    bool recoil_control = true;
    float recoil_x = 100.0f;
    float recoil_y = 100.0f;
    int target = 0;
    bool visible_only = true;
    bool smoke_check = true;
    bool flash_check = true;
    bool auto_pistol = false;

    // Aim limits. These used to be hard-coded in the bridge, so the bot ran with the
    // engine defaults no matter what the tab showed.
    bool autowall = true;
    bool multipoint = true;
    bool lethal_only = false;
    float min_damage = 65.0f;
    int wall_policy = 1; // 0 Block, 1 Penetrate, 2 Ignore
    bool airborne = false;
    float reaction_time = 0.0f;

    // Which body parts the aimbot and the trigger may hit (Head, Chest, Stomach, Arms,
    // Legs, Feet). The trigger used to inherit the engine default of "head only", which
    // made it look broken whenever the crosshair was not exactly on the head.
    bool hitboxes[6] = { true, true, true, false, false, false };

    bool triggerbot = false;
    float trigger_delay = 40.0f;
    float trigger_hit_chance = 70.0f;
    bool trigger_magnet = false;
    bool trigger_smoke_check = true;
    bool trigger_flash_check = true;
    bool trigger_autowall = true;
    bool trigger_lethal_only = false;
    bool trigger_min_damage_enabled = false;
    float trigger_min_damage = 40.0f;
    bool trigger_autostop = false;
    int trigger_seed_type = 0; // 0 None, 1 Restricted, 2 Unrestricted

    bool backtrack = true;
    float backtrack_time = 200.0f;
};

// A toggle with a color, the most common row of the Visuals settings windows.
struct ColorFeature {
    bool on = false;
    ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

inline constexpr ImVec4 rgba(int r, int g, int b, int a = 255)
{
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

struct ModelLayer {
    ImVec4 color;
    int material = 0;
};

// ESP elements of the player preview (ESP Items). Text, bars and icons sit on a side of the box
// and stack outwards in `order`; the box, skeleton and hitboxes are drawn on the model itself.
enum class EspKind : std::uint8_t { box, skeleton, hitboxes, text, bar, flag, icon };
enum class EspSide : std::uint8_t { left, top, right, bottom };
// How a text element renders its sample.
enum class EspText : std::uint8_t { as_is, upper, lower };
// How a bar fills: one colour, a fixed two-tone shade, or a value gradient.
enum class EspBar : std::uint8_t { normal, shade, gradient };

struct EspInfo {
    const char* name;
    int group; // 0 Main, 1 Flags, 2 Weapon, 3 Aimbot
    EspKind kind;
    EspSide side;
    bool on;
    ImU32 color;
    const char* sample; // text shown in the preview, or an nl::icon glyph for icons
};
extern const EspInfo esp_info[];
inline constexpr int esp_element_count = 29;
inline constexpr const char* esp_groups[] = { "Main", "Flags", "Weapon", "Aimbot" };

struct EspElement {
    bool on = false;
    EspSide side = EspSide::top;
    float order = 0.0f;
    ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool background = false;
    bool shadow = true;
    // Size multiplier, applied to the element extent and to its text or icon.
    float scale = 1.0f;
    // Border drawn around the element: text and icons get an outline, bars get an edge.
    bool outline = false;
    ImVec4 outline_color{ 0.0f, 0.0f, 0.0f, 0.85f };
    EspText text_style = EspText::as_is;
    // Text elements can pick their weight: 0 Regular, 1 Medium, 2 SemiBold.
    int font = 1;
    // Bars only.
    EspBar bar_style = EspBar::normal;
    // Icons only: mirror the glyph.
    bool flip = false;
    // Weapon icon extras: the ammo count next to it and the colour it takes when the
    // enemy pulls a grenade pin.
    bool ammo_count = false;
    ImVec4 pin_color{ 235.0f / 255.0f, 65.0f / 255.0f, 70.0f / 255.0f, 1.0f };
    // Names only: show the player picture in front of the nickname.
    bool avatar = false;
};
using EspLayout = std::array<EspElement, esp_element_count>;
EspLayout default_esp();

struct PlayersSettings {
    int target = 0; // Enemies / Teammates / Local
    std::array<EspLayout, 3> esp = { default_esp(), default_esp(), default_esp() };
    bool enabled = false;
    ColorFeature offscreen_arrow{ false, rgba(250, 250, 252) };
    // Sound ESP: the ring an enemy leaves when they make a noise. Duration, radius and
    // local sync are the engine's own knobs, so the menu exposes them too.
    struct SoundsFeature : ColorFeature {
        float duration = 1.6f;
        float radius = 34.0f;
        bool local_sync = false;
        SoundsFeature(bool on, ImVec4 color) : ColorFeature(on, color) {}
    };
    SoundsFeature sounds{ false, rgba(240, 94, 132) };
    ModelLayer player{ rgba(160, 166, 240), 1 };        // Flat by default
    ModelLayer behind_walls{ rgba(102, 226, 240), 1 }; // Flat
    ModelLayer on_shot{ rgba(214, 214, 220), 0 };      // Disabled
    ModelLayer history{ rgba(190, 190, 198), 0 };
    ModelLayer ragdolls{ rgba(150, 150, 166), 0 };
    ColorFeature soul_particles{ false, rgba(98, 108, 240) };
    ColorFeature glow{ false, rgba(196, 196, 226) };
};

struct EspSettings {
    bool enabled = false;
    ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool parts[8] = {};
};

// Map atmosphere, configured per map. "Global" is the fallback the cheat applies when
// the current map has no entry of its own.
struct AmbienceSettings {
    bool on = false;

    // Nightmode: recolours the world. The colour is applied as fog tint plus a
    // brightness scale, which are the levers CS2 actually keeps.
    bool nightmode = false;
    ImVec4 nightmode_color{ 0.25f, 0.27f, 0.35f, 1.0f };
    float exposure = 0.55f;          // 0..1 towards the night colour
    float overlay_transparency = 0.35f;
    bool colorize_props = false;

    bool fullbright = false;
    ImVec4 fullbright_color{ 1.0f, 1.0f, 1.0f, 1.0f };

    // Sky
    bool sky = false;
    // Sky material picked in Override Sky ("materials/skybox/sky_de_dust2.vmat"), empty for
    // the map's own.
    char skybox[96] = {};
    ImVec4 cloud_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    float sky_brightness = 1.0f;

    // Sunlight
    bool sunlight = false;
    ImVec4 sunlight_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    int sunlight_mode = 0;           // 0 Modulation, 1 Override
    float sunlight_rotation = 0.0f;
    float volumetric_brightness = 1.0f;
    float reflection = 1.0f;

    // Fog
    bool fog = false;
    ImVec4 fog_color{ 0.5f, 0.55f, 0.65f, 1.0f };
    float fog_start = 500.0f;
    float fog_end = 3000.0f;
    float fog_exponent = 2.0f;
    float fog_density = 0.4f;

    // Screen effects
    bool vignette = false;
    float vignette_amount = 0.0f;
    bool screen_tint = false;
    ImVec4 screen_tint_color{ 1.0f, 1.0f, 1.0f, 0.0f };
    bool tonemapping = false;
    int tonemap_mode = 0;
    float tonemap_exposure = 1.0f;
    bool contrast = false;
    float contrast_amount = 1.0f;
    float contrast_brightness = 0.0f;
    bool bloom = false;
    float bloom_amount = -1.0f;      // < 0 leaves the engine value alone
    bool dof = false;
    float dof_near = 0.0f;
    float dof_far = 0.0f;

    // Removals: Fog, Lighting, Post Processing, Shadows, Skybox, Particles.
    bool removals[6] = {};

    float fullbright_intensity = 2.5f;
    float sunlight_intensity = 1.0f;
    bool night_props = true;          // nightmode tints props and models too
    ImVec4 sun_color{ 1.0f, 0.85f, 0.6f, 1.0f };
    bool gamma = false;
    float gamma_amount = 1.0f;
};

inline constexpr int ambience_map_count = 8;
inline constexpr int ambience_map_name_len = 32;

// One entry of the MAP SELECTION list.
struct AmbienceMap {
    char map[ambience_map_name_len] = {};
    AmbienceSettings ambience{};
    bool used = false;
};

struct WorldSettings {
    // MAP SELECTION. Index 0 is always "Global"; the cheat resolves the current map
    // against this list and falls back to Global.
    std::array<AmbienceMap, ambience_map_count> maps{};
    int selected_map = 0;
    int map_count = 1;

    AmbienceSettings& ambience_for(int index)
    {
        return maps[static_cast<size_t>(std::clamp(index, 0, map_count - 1))].ambience;
    }
    const AmbienceSettings& ambience_for(int index) const
    {
        return maps[static_cast<size_t>(std::clamp(index, 0, map_count - 1))].ambience;
    }
    AmbienceSettings& ambience() { return ambience_for(selected_map); }
    const char* map_name(int index) const
    {
        return maps[static_cast<size_t>(std::clamp(index, 0, map_count - 1))].map;
    }

    // Weather: wind/rain/lightning effects
    bool weather_wind = false;
    float wind_strength = 3.0f;
    float wind_direction = 0.0f;  // degrees 0-360
    float wind_turbulence = 1.0f;

    bool weather_rain = false;
    ImVec4 rain_color{ 0.8f, 0.85f, 0.9f, 0.7f };
    float rain_speed = 1.0f;
    float rain_density = 1.0f;
    float rain_radius = 500.0f;
    bool weather_splashes = true; // ground splashes under the drops
    bool weather_snow = false;     // snow instead of rain
    float snow_speed = 0.35f;

    bool weather_lightning = false;
    ImVec4 lightning_color{ 0.8f, 0.9f, 1.0f, 1.0f };
    float lightning_interval = 5.0f;
    float lightning_duration = 0.3f;
    bool lightning_spawn_on_kill = false;
    bool lightning_sound = true;

    bool weather_model_wetness = false;
    float wetness_density = 1.8f;
    float wetness_speed = 0.8f;

    ImVec4 snow_color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool weather_stars = false;
    ImVec4 stars_color{ 0.75f, 0.85f, 1.0f, 1.0f };
    bool lightning_flash = true;      // the sky and the world light up with the bolt
    float thunder_volume = 0.7f;

    // View
    bool override_fov = false;
    float fov = 90.0f;
    bool override_aspect = false;
    int aspect_scaling = 0;
    float aspect = 133.0f;
    bool disable_landing_bob = false;
    bool remove_scope_overlay = true;
    ColorFeature scope_lines{ false, rgba(240, 240, 245) };
    float scope_line_length = 40.0f;
    bool disable_zoom_blur = true;
    bool instant_unscope = false;
    bool override_viewmodel = false;
    float viewmodel_x = 2.0f, viewmodel_y = 2.0f, viewmodel_z = -2.0f, viewmodel_fov = 68.0f;
    bool viewmodel_in_scope = true;
    bool thirdperson = false;
    float thirdperson_distance = 120.0f;
    bool thirdperson_collision = true;
    bool thirdperson_dead = false;
    bool unlock_spectating[3] = {};
    bool visual_recoil[3] = {};

    // HUD
    bool radar = false;
    bool preserve_killfeed = false;
    ColorFeature inaccuracy_overlay{ false, rgba(128, 128, 134) };
    int scope_overlay = 0;
    bool reveal_ranks = true, reveal_money = false, show_wins = true;
    bool sniper_crosshair = true;
    ColorFeature penetration_crosshair{ false, rgba(120, 220, 140) };
    bool recoil_crosshair = false;

    // World ESP
    ColorFeature bomb_colorize{ false, rgba(122, 120, 236) };
    bool bomb_indicator = false;
    bool bomb_indicator_timer = true;
    ColorFeature defuse_indicator{ false, rgba(238, 92, 100) };
    ColorFeature defuser{ false, rgba(238, 92, 100) };
    EspSettings bomb{ false, rgba(124, 222, 132), { false, false, false, false, true, false, false, false } };
    ImVec4 bomb_defused_color = rgba(214, 222, 240);
    ColorFeature weapons_colorize{ false, rgba(140, 96, 222) };
    EspSettings weapons{};
    EspSettings hostages{};
    ColorFeature smoke_radius{ false, rgba(110, 160, 240) };
    ColorFeature molotov_radius{ false, rgba(226, 110, 84) };
    ColorFeature colorize_smoke{ false, rgba(150, 146, 100) };
    ColorFeature colorize_molotov{ false, rgba(226, 110, 84) };
    ColorFeature colorize_explosion{ false, rgba(140, 170, 240) };
    EspSettings grenades{};
    bool grenade_trajectory = false;
    ImVec4 trajectory_color = rgba(214, 218, 240);
    bool trajectory_bounces = true;
    float trajectory_thickness = 2.0f;
    bool grenade_proximity = false;

    // Miscellaneous
    bool window_spectators = true, window_keybinds = true, window_watermark = false, window_bomb = false;
    bool removals[8] = { true, true, false, false, true, false, false, false };
    ColorFeature night_mode{ true, rgba(64, 66, 84) };
    ColorFeature props{ false, rgba(140, 140, 150) };
    ColorFeature sky{ false, rgba(70, 90, 160) };
    int skybox = 0;
    float brightness = 40.0f;
    ColorFeature hit_marker_crosshair{ true, rgba(240, 240, 245) };
    ColorFeature hit_marker_world{ false, rgba(240, 240, 245) };
    ColorFeature hit_marker_damage{ true, rgba(180, 212, 255) };
    float hit_marker_duration = 50.0f;
    ColorFeature tracers_local{ true, rgba(180, 212, 255) };
    ColorFeature tracers_enemies{ false, rgba(238, 92, 100) };
    ColorFeature tracers_team{ false, rgba(124, 222, 132) };
    int tracer_style = 0;
    float tracer_duration = 40.0f;
    ColorFeature bullet_impacts{ false, rgba(232, 160, 220) };
};

struct VisualsSettings {
    PlayersSettings players;
    WorldSettings world;
};

// How the finish of one loadout slot is rolled. Kept per slot rather than per catalogue
// item, so changing the finish keeps the wear and pattern the user dialled in.
struct SlotFinish {
    float wear = 0.01f;
    int seed = 0;
    bool stat_trak = false;
    int kills = 0;
    char name[32]{}; // name tag, empty = none
    bool colors = false;
    // Chams on the model itself, in nl::inv::k_model_materials order (0 = Disabled).
    int model_material = 0;
    ImVec4 model_color = rgba(255, 255, 255);
    bool model_overlay = false;
    float model_exponent = 50.0f;
};

struct InventorySettings {
    int side = 0; // 0 = CT, 1 = T
    // Applies the selected finishes to the local player's weapons in game (the check
    // button of the picker toolbar).
    bool apply = true;
    std::array<std::array<inv::Equip, inv::slot_count>, 2> loadout = default_loadout();
    std::array<std::array<SlotFinish, inv::slot_count>, 2> finish{};
};

struct MiscSettings {
    bool bunny_hop = true;
    bool air_strafe = true;
    bool air_strafe_avoid_collisions = false;
    bool air_strafe_view_angles = true;
    bool air_strafe_movement_keys = true;
    bool air_strafe_only_jump = false;
    bool air_strafe_stop_shift = true;
    bool air_strafe_stop_release = false;
    bool air_strafe_subtick = false;
    float air_strafe_turn_speed = 100.0f;
    bool jump_bug = true;
    int jump_bug_mode = 1;
    bool jump_bug_land = false;
    bool no_fall_damage = false;
    bool standalone_quick_stop = true;
    bool strafe_assist = true;
    bool edge_jump = false;
    bool slow_walk = false;
    float slow_walk_speed = 34.0f;
    bool slow_walk_random = false;
    bool fast_ladder = true;
    bool air_duck = false;

    bool quick_switch = true;
    bool super_toss = false;
    bool knife_bot = false;
    bool prevent_afk_kick = true;
    bool hit_sound = true;
    int hit_sound_kind = 0;
    float hit_sound_volume = 70.0f;
    bool automatic_purchase = false;
    int buy_primary = 1;
    int buy_secondary = 1;
    bool buy_utility[5] = { true, true, true, true, false };
    bool automatic_grenade_release = true;
    float grenade_release_damage = 25.0f;
    bool auto_accept_matchmaking = true;
    ImVec4 log_events_color{ 239 / 255.0f, 196 / 255.0f, 216 / 255.0f, 1.0f };
    bool log_events[5] = { true, true, false, false, false };
};

struct Config {
    RageSettings rage;
    LegitSettings legit;
    VisualsSettings visuals;
    InventorySettings inventory;
    MiscSettings misc;
};

// Presets are exported as raw bytes, so Config must stay plain data.
static_assert(std::is_trivially_copyable_v<Config>);
// UI presets contain only local widget values.
inline bool decodeHostConfig(Config& output,const void* bytes,std::size_t size){
    if(!bytes||size!=sizeof(Config))return false;
    std::memcpy(&output,bytes,size);return true;
}

// Watermark items, in the order of the Items list.
enum WatermarkItem : int {
    wm_logo, wm_kd, wm_speed, wm_framerate, wm_gpu, wm_cpu, wm_cpu_freq, wm_memory, wm_latency, wm_var, wm_loss,
    wm_connection, wm_server, wm_preset, wm_username, wm_time, wm_count
};
inline constexpr const char* watermark_item_names[] = { "Logo",     "KD Ratio",    "Speed",        "Framerate",
                                                        "GPU Load", "CPU Load",    "CPU Frequency", "Memory Load",
                                                        "Latency",  "Var",         "Loss",         "Connectivity Issues",
                                                        "Server Address", "Preset", "Username",    "Time" };
// Snap positions of the watermark on screen.
inline constexpr const char* watermark_anchors[] = { "Top Left",    "Top Center",    "Top Right",
                                                     "Bottom Left", "Bottom Center", "Bottom Right" };

struct WatermarkSettings {
    std::array<int, wm_count> order{}; // items in display order
    std::array<bool, wm_count> enabled{};
    int anchor = 2;
    bool avatar = true;
    bool background = true;
    bool shadow = true;
    bool custom_color = false; // icons in `color` instead of the style color
    ImVec4 color{ 0.93f, 0.33f, 0.62f, 1.0f };

    WatermarkSettings()
    {
        // The reference bar: NL, time, FPS, latency, speed, user.
        const int shown[] = { wm_logo, wm_time, wm_framerate, wm_latency, wm_speed, wm_username };
        int n = 0;
        for (int item : shown) {
            order[static_cast<size_t>(n++)] = item;
            enabled[static_cast<size_t>(item)] = true;
        }
        for (int i = 0; i < wm_count; ++i) {
            bool listed = false;
            for (int item : shown)
                listed |= item == i;
            if (!listed)
                order[static_cast<size_t>(n++)] = i;
        }
    }
};

inline constexpr const char* preset_names[] = {
    "PREMIER CFG by gra1ncheats",
    "HvH NoSpread Double Tap",
    "HvH NoSpread Removed FL",
    "HvH Default",
    "Legit",
    "Semi-Rage",
};
inline constexpr int preset_count = static_cast<int>(sizeof(preset_names) / sizeof(preset_names[0]));

std::array<Config, preset_count> make_presets();
// Makes sure the MAP SELECTION list starts with Global and drops the untouched copies of
// it that older builds seeded for the whole map pool. Call after loading a preset.
void init_ambience_maps(WorldSettings& w);

} // namespace nl
