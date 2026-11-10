// Menu source watermark: @use_sberbank
#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "menu/binds.h"
#include "menu/config.h"
#include "menu/style.h"

struct ImTextureData;

namespace nl {

// Global ImGui style tweaks used by stock widgets inside the menu's popups (color picker).
void apply_imgui_style();

inline constexpr const char* user_name = "Pyzirik";

// Fades (and optionally slides) everything drawn since vertex `from` in the draw list.
void fade_vertices(ImDrawList* dl, int from, float alpha, float dy = 0.0f);

// A config of the Presets list with its own binds.
struct Preset {
    std::string name;
    const char* icon = nullptr; // nl::icon glyph in front of the name
    Config config;
    Config defaults; // Reset in the bind menu
    binds::List binds;
};

class Menu {
public:
    void (*host_content)(Menu&,const ImVec2&,int,int)=nullptr;
    int active_tab()const{return tab_;}
    int active_preset()const{return preset_;}
    void select_host_preset(int value){if(value>=0&&value<static_cast<int>(presets_.size()))select_preset(value);}
    int active_subtab()const{return visuals_subtab_;}
    void render_host_esp(ImDrawList* draw,const ImVec2& origin,PlayersSettings& players,const ImVec2& min,const ImVec2& max){draw_esp_preview(draw,origin,players,min,max);}
    void render_host_inventory(const ImVec2& origin){tab_inventory(origin,config());}
    void render_host_watermark();
    static constexpr float min_scale = 0.45f;
    static constexpr float max_scale = 3.0f;

    // Call after ImGui::CreateContext() and load_fonts(). `pos` is the initial top-left corner and
    // `scale` the initial UI scale (1 = the size of the reference screenshot).
    void init(const ImVec2& pos, float scale = 1.0f);
    // Call after the renderer backend has shut down and before ImGui::DestroyContext().
    void shutdown();
    // Hosts call this before pausing or destroying their renderer.
    bool save_presets() const;
    // Call between ImGui::NewFrame() and ImGui::Render().
    void render();

    void toggle_visible();
    // Hidden without the fade-out (the menu starts this way while the cheat loads); shown with
    // the usual fade-in.
    void set_visible(bool visible);
    bool visible() const { return visible_; }
    // The host uses 1 Aim, 2 Visuals (Players, World, Misc), 3 Skins, 4 Settings.
    // The donor menu retains its original tab indices.
    void set_tab(int tab, int subtab = 0)
    {
        tab_ = tab;
        visuals_subtab_ = subtab;
    }
    // Opens the skin picker of an inventory slot (nl::inv::Slot), for previews and tests.
    // `item` selects a catalogue item in that slot's current weapon, which is how the
    // screenshot harness reaches one specific skin.
    void open_picker(int slot, int item = -1);
    void set_inspect_fullscreen(bool on) { inspect_fullscreen_ = on; }

    // The active preset's config; read by the cheat backend. Does not alter the menu.
    Config& config() { return presets_[static_cast<size_t>(preset_)]->config; }

    // UI scale, clamped to [min_scale, max_scale]; the menu keeps its center on screen. While the
    // menu is visible, Ctrl + mouse wheel and Ctrl +/- change it in 10% steps and Ctrl+0 restores
    // the initial scale.
    void set_scale(float scale);
    float scale() const;
    // Multiplies the current and the initial scale, e.g. after the window moved to a monitor with
    // a different DPI and was resized by the same factor.
    void rescale(float factor);

private:
    void handle_zoom();
    // Moves the menu so that, at `scale`, it is centered on its current center multiplied by `factor`.
    void recenter(float scale, float factor);
    void update_avatar(int side);
    void draw_avatar(ImDrawList* dl, const ImVec2& min, float d, float alpha = 1.0f) const;
    void draw_sidebar(ImDrawList* dl, const ImVec2& o);
    void draw_header(ImDrawList* dl, const ImVec2& o);
    void draw_content(const ImVec2& o);
    void draw_profile_popup(const ImVec2& o);
    void draw_preset_group(ImDrawList* dl, const ImVec2& o, float& right);
    // Returns the right edge of the dropdown.
    float draw_weapon_group(ImDrawList* dl, const ImVec2& o, float left);
    void select_preset(int preset);
    void toast(const char* text);

    // Presets window (presets.cpp)
    void draw_presets_popup(ImGuiID id, const ImVec2& anchor_min, const ImVec2& anchor_max);
    void add_preset(const std::string& name, const Config& config, bool rename);
    void delete_preset(int index);
    bool export_preset(int index) const;
    bool import_preset();

    // Preset persistence (presets.cpp). The list used to live only in memory, so every
    // create/rename/delete was lost when the game closed. It is mirrored to a small
    // file under %APPDATA%\neverlose and reloaded on the next start.
    bool load_presets();
    void touch_presets() { presets_dirty_ = true; }
    void flush_presets(float dt);

    // Search (search_overlay.cpp)
    void open_search();
    void collect_search();
    void draw_search(const ImVec2& o);

    // Watermark (watermark.cpp)
    void draw_watermark();
    void watermark_settings();
    void watermark_items_list();
    void update_stats();

    void tab_rage(const ImVec2& o, Config& c);
    void tab_legit(const ImVec2& o, Config& c);
    void tab_visuals(const ImVec2& o, Config& c);
    void tab_inventory(const ImVec2& o, Config& c);
    void tab_misc(const ImVec2& o, Config& c);
    void visuals_players(const ImVec2& o, PlayersSettings& p);
    void visuals_world(const ImVec2& o, WorldSettings& w);
    // The Ambience view drawn over the blurred World panels (ambience.cpp).
    void draw_ambience(const ImVec2& o, WorldSettings& w);
    void ambience_cards(const ImVec2& o, WorldSettings& w);
    void ambience_effects(const ImVec2& o, WorldSettings& w);
    void draw_picker(const ImVec2& o, InventorySettings& inv);
    // ESP preview of the Players tab (esp_preview.cpp)
    void draw_esp_preview(ImDrawList* dl, const ImVec2& o, PlayersSettings& p, const ImVec2& model_min,
                          const ImVec2& model_max);
    // Renders the enemy model into that rect; false when the offscreen preview is
    // not ready yet and the caller should fall back to the shipped picture.
    bool draw_esp_model(ImDrawList* dl, PlayersSettings& p, const ImVec2& model_min, const ImVec2& model_max);
    void draw_esp_items(PlayersSettings& p, ImGuiID id, const ImVec2& anchor);

    std::vector<std::unique_ptr<Preset>> presets_;
    int preset_ = 0;
    float fps_ = 0.0f; // shown in the watermark, refreshed twice a second
    float fps_timer_ = 0.0f;
    int tab_ = 4;
    int visuals_subtab_ = 0;
    // 0 = the World panels, 1 = the Ambience view over them.
    int world_view_ = 0;
    int ambience_menu_ = -1;     // map card whose "..." menu is open
    float ambience_fade_ = 0.0f; // fade-in of the Ambience view
    // Yaw of the agent model shown in the inventory (mouse drag).
    float agent_orbit_ = 0.0f;
    // Universal DPI scaling: the button above the ESP preview scales every element at
    // once instead of only the one under the cursor.
    bool esp_uniform_scale_ = false;
    float visuals_open_ = -1.0f; // expansion of the Visuals subtabs in the sidebar, -1 until first frame
    float tab_marker_y_ = -1.0f; // sliding highlight of the selected tab, menu coordinates

    // Inventory skin picker
    int picker_slot_ = -1;   // slot being edited, -1 when closed
    int picker_weapon_ = -1; // weapon whose skins are listed
    float picker_anim_ = 0.0f;
    int rarity_filter_ = 0;
    char search_[64] = {};
    float grid_scroll_ = 0.0f;
    bool inspect_fullscreen_ = false;

    // Presets window
    char preset_filter_[64] = {};
    int preset_sort_ = 0; // 0 list order, 1 A-Z, 2 Z-A
    int rename_ = -1;     // preset whose name is being edited
    char rename_buf_[64] = {};
    bool rename_focus_ = false;
    int preset_menu_ = -1; // preset whose "•••" menu is open
    float preset_scroll_ = 0.0f, preset_scroll_target_ = 0.0f;
    const Preset* deleting_ = nullptr; // folds up, then is erased
    float delete_anim_ = 1.0f;
    std::string toast_text_;
    float toast_ = 0.0f;
    std::string shown_preset_, previous_preset_; // header name crossfade
    float preset_change_ = 1.0f;
    bool presets_dirty_ = false;   // structural change: write the file on the next frame
    float presets_flush_ = 0.0f;   // seconds since the last comparison with the file
    Config presets_saved_;         // last state written, to detect in-place edits

    // Search
    bool search_open_ = false;
    bool search_focus_ = false;
    float search_anim_ = 0.0f;
    char query_[64] = {};
    std::string last_query_;
    float results_anim_ = 1.0f;
    float search_scroll_ = 0.0f, search_scroll_target_ = 0.0f;
    int pending_tab_ = -1, pending_subtab_ = 0; // navigation requested from the results

    // Watermark
    WatermarkSettings wm_;
    bool host_watermark_initialized_ = false;
    std::array<float, wm_count> wm_shown_{}; // presence of each item, animated
    std::array<float, wm_count> wm_width_{}; // animated width of each item
    std::array<float, wm_count> wm_x_{};     // animated position of each item inside the bar
    ImVec2 wm_pos_{ -1.0f, -1.0f };          // animated top-left corner
    float wm_bar_w_ = 0.0f;
    float wm_avatar_ = 1.0f;
    int wm_drag_item_ = -1;    // item being reordered inside the bar
    float wm_drag_dx_ = 0.0f;  // grab point inside the dragged item
    bool wm_moving_ = false;   // the bar itself is being dragged between snap positions
    int wm_hover_anchor_ = -1; // snap position under the cursor while moving
    float wm_guides_ = 0.0f;   // fade of the snap guides
    int wm_list_drag_ = -1;    // row being dragged in the Items list
    float wm_list_grab_ = 0.0f;
    float stats_timer_ = 0.0f;
    float cpu_load_ = 0.0f, memory_load_ = 0.0f, gpu_load_ = 0.0f, cpu_mhz_ = 0.0f;
    float latency_ = 9.0f, var_ = 0.4f, speed_ = 0.0f;
    unsigned long long cpu_idle_ = 0, cpu_total_ = 0;

    // ESP preview
    int esp_drag_ = -1; // element being dragged
    ImVec2 esp_drag_offset_{};
    bool esp_dragging_ = false; // moved past the drag threshold
    float esp_zones_ = 0.0f;   // fade of the drop zones
    int esp_settings_ = -1;    // element whose settings window is open
    std::array<ImVec2, esp_element_count> esp_pos_{}; // animated positions in the preview
    std::array<float, esp_element_count> esp_shown_{};
    bool esp_pos_valid_ = false;
    ImVec2 esp_items_pos_{}; // top-left corner of the ESP Items window, dragged by its title
    bool esp_items_placed_ = false;

    // Profile settings
    ImVec4 style_color_ = ImGui::ColorConvertU32ToFloat4(style::col::accent_default);
    int language_ = 0;
    int esp_scale_ = 0, windows_scale_ = 0, units_ = 0, safe_mode_ = 0;
    bool sync_ = true;
    float pending_scale_ = 0.0f; // Menu Scale picked in the profile window, applied next frame
    bool visible_ = true;
    float visibility_ = 1.0f;
    float open_speed_ = 7.0f;
    float tab_switch_ = 1.0f;
    float save_flash_ = 0.0f;
    ImTextureData* avatar_ = nullptr;
    int avatar_side_ = 0; // size in pixels the avatar texture was last resampled to
    ImVec2 initial_pos_{};
    float default_scale_ = 1.0f;
    float zoom_wheel_ = 0.0f; // Ctrl + wheel movement not yet turned into a zoom step
    ImVec2 pos_{};            // top-left corner of the menu in the last frame
    bool placed_ = false;     // pos_ is valid
    bool reposition_ = false; // move the window to pos_ in the next frame
};

} // namespace nl
