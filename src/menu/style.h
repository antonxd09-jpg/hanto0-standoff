// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

// Windows RPC headers define small as a macro.
#ifdef small
#undef small
#endif

// Palette and metrics measured from the reference screenshot (1:1 pixels).
// All positions are relative to the menu's top-left corner.
namespace nl::style {

// UI scale. The metrics below are unscaled (scale 1 is the reference screenshot);
// px() converts a metric to screen pixels. Set through Menu::set_scale().
inline float ui_scale = 1.0f;
inline float px(float v) { return v * ui_scale; }

constexpr ImU32 rgb(int r, int g, int b, int a = 255) { return IM_COL32(r, g, b, a); }

namespace col {
inline constexpr ImU32 shadow            = rgb(0, 0, 0, 120);

inline constexpr ImU32 sidebar_top       = rgb(8, 11, 20);
inline constexpr ImU32 sidebar_bottom    = rgb(11, 14, 23);
inline constexpr ImU32 sidebar_separator = rgb(255, 255, 255, 24);
inline constexpr ImU32 sidebar_header    = rgb(148, 156, 174, 215);
inline constexpr ImU32 tab_text          = rgb(150, 153, 161);
inline constexpr ImU32 tab_icon          = rgb(132, 136, 143);
inline constexpr ImU32 tab_text_hovered  = rgb(196, 199, 207);
inline constexpr ImU32 tab_text_active   = rgb(244, 245, 249);
inline constexpr ImU32 tab_icon_active   = rgb(206, 224, 252);
inline constexpr ImU32 tab_icon_cutout   = rgb(16, 34, 64);
inline constexpr ImU32 tab_bg_active     = rgb(34, 37, 46);
inline constexpr ImU32 tab_bg_hovered    = rgb(210, 225, 255, 22);

inline constexpr ImU32 sidebar_icon_hole = rgb(9, 12, 21);

inline constexpr ImU32 logo_bg           = rgb(14, 24, 44, 155);
inline constexpr ImU32 logo_glow         = rgb(0, 92, 190, 70);
inline constexpr ImU32 logo_text         = rgb(255, 255, 255);
inline constexpr ImU32 title             = rgb(246, 247, 250);
inline constexpr ImU32 subtitle          = rgb(156, 164, 182, 220);

inline constexpr ImU32 username          = rgb(228, 229, 233);
inline constexpr ImU32 subscription      = rgb(96, 99, 111);
inline constexpr ImU32 profile_chevron   = rgb(194, 198, 203);

inline constexpr ImU32 main_bg           = rgb(13, 14, 19);
inline constexpr ImU32 main_border       = rgb(25, 26, 31);
inline constexpr ImU32 header_separator  = rgb(22, 23, 28);
inline constexpr ImU32 group_bg          = rgb(10, 11, 16);
inline constexpr ImU32 group_border      = rgb(22, 23, 28);
inline constexpr ImU32 group_hovered     = rgb(18, 19, 25);
inline constexpr ImU32 save_icon         = rgb(240, 241, 245);
inline constexpr ImU32 preset_text       = rgb(202, 203, 207);
inline constexpr ImU32 preset_chevron    = rgb(188, 189, 194);
inline constexpr ImU32 search_icon       = rgb(182, 183, 187);
inline constexpr ImU32 preset_sun        = rgb(242, 182, 58);

inline constexpr ImU32 watermark_text        = rgb(206, 207, 214);
inline constexpr ImU32 watermark_logo        = rgb(214, 247, 255);
inline constexpr ImU32 watermark_logo_glow   = rgb(0, 34, 60, 150);
inline constexpr ImU32 watermark_icon        = rgb(204, 205, 212);
inline constexpr ImU32 watermark_icon_cutout = rgb(40, 41, 50);
inline constexpr ImU32 watermark_fps_top     = rgb(255, 170, 110);
inline constexpr ImU32 watermark_fps_bottom  = rgb(232, 104, 72);
inline constexpr ImU32 watermark_cloud       = rgb(216, 217, 224);
inline constexpr ImU32 watermark_arrow_tile  = rgb(52, 150, 232);
inline constexpr ImU32 watermark_check_tile  = rgb(74, 190, 96);

inline constexpr ImU32 section_header    = rgb(92, 93, 98);
inline constexpr ImU32 panel_bg          = rgb(18, 18, 26);
inline constexpr ImU32 panel_border      = rgb(25, 26, 31);
inline constexpr ImU32 row_separator     = rgb(26, 26, 35);
inline constexpr ImU32 panel_row_hover   = rgb(27, 28, 38);
inline constexpr ImU32 label             = rgb(240, 241, 245);
inline constexpr ImU32 dots              = rgb(242, 243, 247);

// Accent of toggles, sliders and check marks. Follows the Style color of the profile window.
inline ImU32 accent                      = rgb(179, 213, 255);
inline constexpr ImU32 toggle_on_knob    = rgb(254, 254, 251);
inline constexpr ImU32 toggle_off_track  = rgb(7, 7, 17);
inline constexpr ImU32 toggle_off_knob   = rgb(124, 135, 145);
inline constexpr ImU32 toggle_outline    = rgb(0, 3, 12, 235); // 1 px dark ring around toggles and swatches

inline constexpr ImU32 combo_bg          = rgb(27, 28, 37);
inline constexpr ImU32 combo_bg_hovered  = rgb(32, 33, 43);
inline constexpr ImU32 combo_text        = rgb(200, 200, 210);
inline constexpr ImU32 combo_chevron     = rgb(190, 190, 202);

inline constexpr ImU32 popup_bg          = rgb(18, 18, 26);
inline constexpr ImU32 popup_border      = rgb(32, 33, 40);
inline constexpr ImU32 popup_item_hover  = rgb(150, 170, 230, 30);
inline constexpr ImU32 accent_default    = rgb(179, 213, 255);

inline constexpr ImU32 log_events_color  = rgb(239, 196, 216);

// Floating windows (settings, dropdowns, binds, color picker): a translucent fill over the
// blurred frame, with groups that only differ by a faint outline.
inline constexpr ImU32 window_bg             = rgb(21, 20, 33, 222);
inline constexpr ImU32 window_border         = rgb(255, 255, 255, 13);
inline constexpr ImU32 window_shadow         = rgb(0, 0, 0, 110);
inline constexpr ImU32 window_group_bg       = rgb(255, 255, 255, 3);
inline constexpr ImU32 window_group_border   = rgb(255, 255, 255, 11);
inline constexpr ImU32 overlay_pill          = rgb(12, 12, 20, 168); // watermark and hotkey list
inline constexpr ImU32 window_row_hover      = rgb(150, 170, 230, 30);
inline constexpr ImU32 window_separator      = rgb(255, 255, 255, 14);
inline constexpr ImU32 window_danger         = rgb(232, 76, 76);
inline constexpr ImU32 window_chip_bg        = rgb(255, 255, 255, 13);
inline constexpr ImU32 window_chip_hovered   = rgb(255, 255, 255, 24);
inline constexpr ImU32 window_label          = rgb(226, 227, 233);
inline constexpr ImU32 window_label_disabled = rgb(82, 84, 94);
inline constexpr ImU32 window_icon           = rgb(206, 208, 216);
inline constexpr ImU32 window_title          = rgb(242, 243, 247);
inline constexpr ImU32 window_value          = rgb(170, 172, 183);
inline constexpr ImU32 window_check_disabled = rgb(98, 100, 111);
inline constexpr ImU32 chip_bg               = rgb(24, 25, 33);
inline constexpr ImU32 chip_bg_selected      = rgb(38, 40, 51);
inline constexpr ImU32 chip_bg_hovered       = rgb(44, 46, 58);
inline constexpr ImU32 chip_text             = rgb(160, 162, 173);
inline constexpr ImU32 slider_track          = rgb(130, 150, 220, 22);
inline constexpr ImU32 slider_knob           = rgb(255, 255, 255);
inline constexpr ImU32 slider_box_border     = rgb(255, 255, 255, 7);
inline constexpr ImU32 slider_value          = rgb(172, 174, 188);
inline constexpr ImU32 tooltip_bg            = rgb(14, 14, 20, 236);
inline constexpr ImU32 tooltip_text          = rgb(246, 247, 250);
inline constexpr ImU32 bind_icon             = rgb(132, 136, 150);

// Inventory
inline constexpr ImU32 card_bg               = rgb(20, 21, 29);
inline constexpr ImU32 card_bg_hovered       = rgb(26, 27, 37);
inline constexpr ImU32 card_border           = rgb(28, 29, 38);
inline constexpr ImU32 card_name             = rgb(208, 210, 218);
inline constexpr ImU32 card_skin             = rgb(124, 127, 138);
inline constexpr ImU32 overlay_bg            = rgb(13, 14, 19, 246);
inline constexpr ImU32 ct_color              = rgb(155, 197, 250);
inline constexpr ImU32 t_color               = rgb(234, 199, 112);
inline constexpr ImU32 picker_accent         = rgb(118, 148, 255);
inline constexpr ImU32 picker_field_bg       = rgb(255, 255, 255, 9);
inline constexpr ImU32 picker_mode           = rgb(222, 223, 230);
inline constexpr ImU32 picker_hash           = rgb(118, 120, 132);
inline constexpr ImU32 picker_value          = rgb(214, 215, 222);
} // namespace col

// Text alpha multiplier for disabled rows (Knife Bot in the reference).
inline constexpr float disabled_alpha = 0.45f;

namespace font_size {
inline constexpr float text      = 13.6f; // presets, combo values, profile name
inline constexpr float label     = 13.47f; // row labels
inline constexpr float tab       = 14.15f; // sidebar tabs, profile name
inline constexpr float combo     = 13.2f;  // value text inside combo boxes
inline constexpr float header    = 10.9f; // MOVEMENT / FEATURES
inline constexpr float sidebar_header = 10.8f; // AIMBOT / COMMON
inline constexpr float small     = 11.7f; // "Lifetime"
inline constexpr float title     = 15.25f; // Neverlose
inline constexpr float subtitle  = 8.45f; // Counter-Strike 2
inline constexpr float logo      = 16.8f; // NL
inline constexpr float icon      = 11.0f;
inline constexpr float icon_save = 11.5f;
inline constexpr float popup_label  = 13.2f; // rows of settings windows
inline constexpr float window_title = 14.0f; // settings window titles
inline constexpr float card    = 11.6f; // weapon / skin names on inventory cards
inline constexpr float value   = 11.6f; // slider value boxes
inline constexpr float picker_title = 17.0f;
inline constexpr float watermark = 12.4f;
inline constexpr float picker_value = 11.2f; // HEX / value / alpha row of the color picker
inline constexpr float watermark_logo = 13.4f;
} // namespace font_size

namespace layout {
inline constexpr float width  = 633.0f;
inline constexpr float height = 485.0f;
inline constexpr float rounding = 10.0f;
inline constexpr float shadow_size = 18.0f;

inline constexpr float sidebar_width = 134.0f;

// Sidebar
inline constexpr float logo_x = 12.0f, logo_y = 8.5f, logo_w = 27.0f, logo_h = 27.0f, logo_rounding = 6.5f;
inline constexpr float logo_text_cx = 25.6f, logo_text_cap_y = 17.1f, logo_glow_radius = 2.0f;
inline constexpr float title_x = 47.15f, title_cap_y = 12.6f;
inline constexpr float subtitle_x = 48.15f, subtitle_cap_y = 26.45f;
inline constexpr float sidebar_sep_x0 = 8.0f, sidebar_sep_x1 = 126.0f;
inline constexpr float sidebar_sep_top_y = 43.0f, sidebar_sep_bottom_y = 440.0f;

inline constexpr float sidebar_header_x = 15.4f;
inline constexpr float aimbot_header_cap_y = 55.27f, common_header_cap_y = 140.72f;
inline constexpr float host_common_shift = 34.72f;
inline constexpr float tab_top_y[5] = { 67.6f, 96.85f, 152.95f, 182.2f, 211.5f };
inline constexpr float tab_x0 = 8.0f, tab_x1 = 126.5f, tab_h = 25.0f, tab_rounding = 5.0f;
inline constexpr float tab_icon_cx = 21.7f, tab_icon_dy = 0.12f;
inline constexpr float misc_icon_cx = 22.0f;
inline constexpr float tab_text_x = 35.3f, tab_text_dy = -0.08f;
inline constexpr float subtab_indent = 11.5f;

inline constexpr float avatar_cx = 26.0f, avatar_cy = 463.0f, avatar_r = 14.0f;
inline constexpr float username_x = 47.1f, username_cap_y = 452.52f;
inline constexpr float days_x = 47.0f, days_cap_y = 467.2f;
inline constexpr float profile_chevron_cx = 120.0f, profile_chevron_cy = 463.0f;

// Main area
inline constexpr float main_x = 134.0f;
inline constexpr float main_rounding = 10.0f;

// Preset group (save button + preset dropdown)
inline constexpr float group_x = 143.25f, group_y = 10.5f, group_h = 23.8f, group_rounding = 4.0f;
inline constexpr float group_sep_x = 171.0f;
inline constexpr float save_icon_cx = 157.1f, save_icon_cy = 22.4f;
inline constexpr float preset_text_x = 182.12f, preset_cap_y = 18.45f;
inline constexpr float preset_chevron_dy = 0.3f;
inline constexpr float preset_chevron_gap = 15.0f;   // text advance end -> chevron center
inline constexpr float preset_right_pad = 12.6f;     // chevron center -> group right edge
inline constexpr float preset_group_max_right = 351.8f;
inline constexpr float preset_icon_dx = 5.3f, preset_icon_text_dx = 14.6f; // sun in front of the name
inline constexpr float preset_fade_to = 9.3f, preset_fade_len = 15.0f;     // left of the chevron
// Weapon group dropdown next to the preset group (Rage tab)
inline constexpr float weapon_group_gap = 14.0f, weapon_group_w = 66.7f;
inline constexpr float weapon_text_x = 9.3f, weapon_chevron_gap = 7.3f, weapon_right_pad = 13.0f;
inline constexpr float search_cx = 610.0f, search_cy = 22.0f;

inline constexpr float header_sep_y = 43.0f, header_sep_x0 = 142.5f, header_sep_x1 = 624.0f;

inline constexpr float section_cap_y = 55.36f;
inline constexpr float section_pad_left_l = 10.92f, section_pad_left_r = 10.17f;

inline constexpr float panel_y = 67.58f, panel_h = 278.3f, panel_rounding = 8.0f;
inline constexpr float panel_gap = 8.0f;
inline constexpr float left_panel_x = 143.3f, left_panel_w = 238.0f;
inline constexpr float right_panel_x = 389.4f, right_panel_w = 234.1f;
inline constexpr int   rows_per_panel = 9;
inline constexpr float row_h = (panel_h - 0.6f) / rows_per_panel;
inline constexpr float row_top_pad = 0.1f;
inline constexpr float row_pad_left = 11.8f;
inline constexpr float row_pad_left_r = 12.1f;
inline constexpr float row_pad_right_l = 11.0f, row_pad_right_r = 11.5f; // panel edge -> controls
inline constexpr float row_label_dy = 0.3f;
inline constexpr float dots_dy = 0.22f, swatch_dy = 0.0f, multi_dy = 0.1f;
inline constexpr float row_sep_pad_right_l = 10.5f;
inline constexpr float row_sep_pad_right_r = 13.0f;

inline constexpr float toggle_w = 25.2f, toggle_h = 15.55f, knob_r = 6.35f, knob_r_on = 7.2f;
inline constexpr float knob_inset_off = 7.75f, knob_inset_on = 7.2f; // track end -> knob center
inline constexpr float toggle_inset_right = 0.6f, toggle_dy = 0.11f;
inline constexpr float dots_gap = 13.85f;  // toggle left edge -> dots center
inline constexpr float multi_dots_gap = 14.6f, slider_dots_dx = 15.5f; // combo / slider track -> dots center
inline constexpr float swatch_size = 12.7f, swatch_rounding = 4.0f;
inline constexpr float swatch_gap = 7.4f;  // swatch right edge -> combo left edge
inline constexpr float multi_w = 105.0f, multi_h = 20.2f, multi_rounding = 4.0f;
inline constexpr float combo_text_dy = 0.0f, combo_chevron_dy = 0.5f;

// Watermark bar in the top-right corner of the screen.
namespace watermark {
inline constexpr float margin = 12.0f, height = 21.0f, rounding = 5.5f;
inline constexpr float pad_left = 6.5f, pad_right = 3.0f, item_gap = 9.5f, icon_text_gap = 4.5f;
inline constexpr float icon_w = 9.5f, cloud_w = 11.0f, cloud_gap = 5.0f, emoji = 8.5f, avatar = 13.0f;
} // namespace watermark

// Sliders of the main panels: a thin track with the knob inside it and a value box on the right.
inline constexpr float slider_track_dx = 105.0f; // track start, left of the control right edge
inline constexpr float slider_track_h = 3.0f, slider_knob_r = 4.8f;
inline constexpr float slider_box_h = 17.0f, slider_box_pad = 4.5f, slider_box_min_w = 18.0f;
inline constexpr float slider_box_gap = 9.75f, slider_box_rounding = 3.5f;
// Sliders of settings windows: a thicker bar without knob or value.
inline constexpr float bar_slider_w = 78.0f, bar_slider_h = 4.0f;
} // namespace layout

} // namespace nl::style
