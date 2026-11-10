// Menu source watermark: @use_sberbank
#pragma once

#include <cmath>

#include "imgui.h"

// Low-level drawing helpers shared by the menu and its widgets.
//
// Positions and lengths are screen pixels, as with ImDrawList. Font sizes are the unscaled
// style::font_size values: the text helpers convert them with font_px().
//
// Text helpers take a "pen" position where x is the pen origin and y is the top of
// capital letters. The reference layout was measured that way, and it keeps text
// independent of each font's ascent/line gap.
namespace nl::draw {

// Nearest whole pixel. Edges and text pens are snapped so they stay crisp at any UI scale.
inline float snap(float v) { return std::floor(v + 0.5f); }
// Thickness of 1 px borders and separators at the current UI scale.
float hairline();
// Pixel size of a font at the current UI scale. ImGui bakes glyphs at whole pixel sizes,
// so a fractional size would stretch them and blur the text.
float font_px(float size);

ImU32 with_alpha(ImU32 col, float alpha);
ImU32 lerp_color(ImU32 a, ImU32 b, float t);

void soft_shadow(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, float size, ImU32 col);
void rect_gradient_v(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 top, ImU32 bottom,
                     float rounding, ImDrawFlags flags = 0);
// Outline lying inside [min, max]. Unlike AddRect(), it stays on the pixel grid for any whole
// `thickness` when min/max are whole pixels.
void border(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 col, float rounding, float thickness);
// Hairline separators: rows [y, y + hairline()) and columns [x, x + hairline()), snapped.
void hline(ImDrawList* dl, float x0, float x1, float y, ImU32 col);
void vline(ImDrawList* dl, float x, float y0, float y1, ImU32 col);

float cap_top_offset(ImFont* font, float size);
float cap_height(ImFont* font, float size);
float text_width(ImFont* font, float size, const char* text, const char* text_end = nullptr);

void text(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col,
          const char* text, const char* text_end = nullptr);
void text_glow(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col, ImU32 glow,
               float radius, const char* text);
// Fades the text alpha to zero between fade_from and fade_to (screen x).
void text_fade_right(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col,
                     const char* text, float fade_from, float fade_to);

// Centers the ink box of a single glyph (icon font) on `center`.
void icon(ImDrawList* dl, ImFont* font, float size, const ImVec2& center, ImU32 col, const char* utf8);
// nl::icon glyph at the row icon size, including the few drawn with shapes (sparkles, rings...).
void any_icon(ImDrawList* dl, const ImVec2& center, ImU32 col, const char* icon, float size = 11.0f);

void chevron_down(ImDrawList* dl, const ImVec2& center, float half_w, float half_h, ImU32 col, float thickness);
void chevron_up(ImDrawList* dl, const ImVec2& center, float half_w, float half_h, ImU32 col, float thickness);
void chevron_right(ImDrawList* dl, const ImVec2& center, float half_w, float half_h, ImU32 col, float thickness);
// "•••" settings button; the dot size and spacing follow the UI scale.
void dots(ImDrawList* dl, const ImVec2& center, ImU32 col);
void check_mark(ImDrawList* dl, const ImVec2& center, float size, ImU32 col, float thickness);

// "Miscellaneous" tab icon: a rounded square with two opposite arrows cut out of it.
void icon_swap_square(ImDrawList* dl, const ImVec2& center, ImU32 fg, ImU32 cutout);

} // namespace nl::draw
