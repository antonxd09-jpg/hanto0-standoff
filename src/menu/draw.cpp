// Menu source watermark: @use_sberbank
#include "menu/draw.h"
#include "menu/localization.h"

#include <cmath>

#include "imgui_internal.h"
#include "menu/fonts.h"
#include "menu/style.h"

namespace nl::draw {
namespace {

using style::px;

ImU32 set_alpha(ImU32 col, float alpha01)
{
    const ImU32 a = static_cast<ImU32>(ImClamp(alpha01, 0.0f, 1.0f) * 255.0f + 0.5f);
    return (col & ~IM_COL32_A_MASK) | (a << IM_COL32_A_SHIFT);
}

// Glow copies of a text are offset by fractions of a pixel; snapping them would collapse the halo.
class NoPixelSnap {
public:
    explicit NoPixelSnap(ImDrawList* dl) : dl_(dl), flags_(dl->Flags) { dl->Flags |= ImDrawListFlags_TextNoPixelSnap; }
    ~NoPixelSnap() { dl_->Flags = flags_; }
    NoPixelSnap(const NoPixelSnap&) = delete;
    NoPixelSnap& operator=(const NoPixelSnap&) = delete;

private:
    ImDrawList* dl_;
    ImDrawListFlags flags_;
};

} // namespace

float hairline() { return ImMax(1.0f, std::floor(style::ui_scale + 0.25f)); }

float font_px(float size) { return ImMax(1.0f, snap(px(size))); }

ImU32 with_alpha(ImU32 col, float alpha)
{
    const float a = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
    return set_alpha(col, a * alpha);
}

ImU32 lerp_color(ImU32 a, ImU32 b, float t)
{
    t = ImClamp(t, 0.0f, 1.0f);
    ImU32 out = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        const float ca = static_cast<float>((a >> shift) & 0xFF);
        const float cb = static_cast<float>((b >> shift) & 0xFF);
        out |= static_cast<ImU32>(ca + (cb - ca) * t + 0.5f) << shift;
    }
    return out;
}

void soft_shadow(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, float size, ImU32 col)
{
    // Concentric rounded-rect rings sharing corner centers, stitched into one mesh. Stacking AA fills
    // instead leaves visible seams where the corner arcs meet the straight edges.
    constexpr int corner_segments = 10;
    constexpr int ring_count = 14;
    constexpr int ring_points = 4 * (corner_segments + 1);
    const float inner = -px(2.0f); // start under the opaque body so its AA edge has nothing to bleed into

    const float base = static_cast<float>((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
    const ImVec2 centers[4] = {
        { max.x - rounding, max.y - rounding },
        { min.x + rounding, max.y - rounding },
        { min.x + rounding, min.y + rounding },
        { max.x - rounding, min.y + rounding },
    };
    const ImVec2 uv = dl->_Data->TexUvWhitePixel;

    dl->PrimReserve(ring_count * ring_points * 6, (ring_count + 1) * ring_points);
    const unsigned int first = dl->_VtxCurrentIdx;
    for (int r = 0; r <= ring_count; ++r) {
        const float d = inner + (size - inner) * static_cast<float>(r) / ring_count;
        const float k = 1.0f - ImClamp(d / size, 0.0f, 1.0f);
        const ImU32 c = set_alpha(col, base * k * k);
        const float radius = ImMax(rounding + d, 0.0f);
        for (int corner = 0; corner < 4; ++corner) {
            for (int s = 0; s <= corner_segments; ++s) {
                const float a = (static_cast<float>(corner) + static_cast<float>(s) / corner_segments) * IM_PI * 0.5f;
                dl->PrimWriteVtx(ImVec2(centers[corner].x + std::cos(a) * radius, centers[corner].y + std::sin(a) * radius),
                                 uv, c);
            }
        }
    }
    for (int r = 0; r < ring_count; ++r) {
        for (int i = 0; i < ring_points; ++i) {
            const unsigned int a0 = first + static_cast<unsigned int>(r * ring_points + i);
            const unsigned int a1 = first + static_cast<unsigned int>(r * ring_points + (i + 1) % ring_points);
            const unsigned int b0 = a0 + ring_points, b1 = a1 + ring_points;
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(a0));
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(b0));
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(b1));
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(a0));
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(b1));
            dl->PrimWriteIdx(static_cast<ImDrawIdx>(a1));
        }
    }
}

void rect_gradient_v(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 top, ImU32 bottom,
                     float rounding, ImDrawFlags flags)
{
    const int v0 = dl->VtxBuffer.Size;
    dl->AddRectFilled(min, max, IM_COL32_WHITE, rounding, flags);
    ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, v0, dl->VtxBuffer.Size, ImVec2(min.x, min.y),
                                                   ImVec2(min.x, max.y), top, bottom);
    const float a0 = static_cast<float>((top >> IM_COL32_A_SHIFT) & 0xFF), a1 = static_cast<float>((bottom >> IM_COL32_A_SHIFT) & 0xFF), dy = ImMax(max.y - min.y, 1.0f);
    for (int i = v0; i < dl->VtxBuffer.Size; ++i) { ImDrawVert& v = dl->VtxBuffer[i]; const float t = ImClamp((v.pos.y - min.y) / dy, 0.0f, 1.0f); v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a0 + (a1 - a0) * t + 0.5f) << IM_COL32_A_SHIFT); }
}

void border(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 col, float rounding, float thickness)
{
    // The stroke is centered on its path, so the path runs half a stroke inside the edges.
    const float h = thickness * 0.5f;
    dl->PathRect(ImVec2(min.x + h, min.y + h), ImVec2(max.x - h, max.y - h), ImMax(rounding - h, 0.0f));
    dl->PathStroke(col, thickness, ImDrawFlags_Closed);
}

void hline(ImDrawList* dl, float x0, float x1, float y, ImU32 col)
{
    const float y0 = snap(y);
    dl->AddRectFilled(ImVec2(snap(x0), y0), ImVec2(snap(x1), y0 + hairline()), col);
}

void vline(ImDrawList* dl, float x, float y0, float y1, ImU32 col)
{
    const float x0 = snap(x);
    dl->AddRectFilled(ImVec2(x0, snap(y0)), ImVec2(x0 + hairline(), snap(y1)), col);
}

float cap_top_offset(ImFont* font, float size)
{
    const float size_px = font_px(size);
    ImFontBaked* baked = font->GetFontBaked(size_px);
    const ImFontGlyph* glyph = baked->FindGlyph(static_cast<ImWchar>('H'));
    return glyph ? glyph->Y0 * (size_px / baked->Size) : 0.0f;
}

float cap_height(ImFont* font, float size)
{
    const float size_px = font_px(size);
    ImFontBaked* baked = font->GetFontBaked(size_px);
    const ImFontGlyph* glyph = baked->FindGlyph(static_cast<ImWchar>('H'));
    return glyph ? (glyph->Y1 - glyph->Y0) * (size_px / baked->Size) : size_px * 0.5f;
}

float text_width(ImFont* font, float size, const char* text, const char* text_end)
{
    const char* translated = l10n::text(text, text_end);
    if (translated != text) { text = translated; text_end = nullptr; }
    return font->CalcTextSizeA(font_px(size), FLT_MAX, 0.0f, text, text_end).x;
}

void text(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col, const char* text,
          const char* text_end)
{
    const char* translated = l10n::text(text, text_end);
    if (translated != text) { text = translated; text_end = nullptr; }
    // With a whole-pixel pen and font size, glyph rows land exactly on pixel rows.
    dl->AddText(font, font_px(size), ImVec2(snap(pen.x), snap(pen.y) - cap_top_offset(font, size)), col, text,
                text_end);
}

void text_glow(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col, ImU32 glow, float radius,
               const char* text)
{
    text = l10n::text(text);
    const float size_px = font_px(size);
    const ImVec2 origin(snap(pen.x), snap(pen.y) - cap_top_offset(font, size));
    {
        NoPixelSnap no_snap(dl);
        constexpr int samples = 16;
        for (int ring = 1; ring <= 2; ++ring) {
            const float r = radius * static_cast<float>(ring) * 0.5f;
            const ImU32 c = with_alpha(glow, ring == 1 ? 1.0f : 0.45f);
            for (int i = 0; i < samples; ++i) {
                const float a = IM_PI * 2.0f * static_cast<float>(i) / samples;
                dl->AddText(font, size_px, ImVec2(origin.x + std::cos(a) * r, origin.y + std::sin(a) * r), c, text);
            }
        }
    }
    dl->AddText(font, size_px, origin, col, text);
}

void text_fade_right(ImDrawList* dl, ImFont* font, float size, const ImVec2& pen, ImU32 col, const char* text,
                     float fade_from, float fade_to)
{
    const int v0 = dl->VtxBuffer.Size;
    const ImVec2 clip_min = dl->GetClipRectMin();
    const ImVec2 clip_max = dl->GetClipRectMax();
    dl->PushClipRect(clip_min, ImVec2(ImMin(clip_max.x, fade_to), clip_max.y), true);
    draw::text(dl, font, size, pen, col, text);
    dl->PopClipRect();

    const float inv = 1.0f / ImMax(fade_to - fade_from, 1.0f);
    for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        v.col = with_alpha(v.col, ImClamp((fade_to - v.pos.x) * inv, 0.0f, 1.0f));
    }
}

void icon(ImDrawList* dl, ImFont* font, float size, const ImVec2& center, ImU32 col, const char* utf8)
{
    unsigned int c = 0;
    ImTextCharFromUtf8(&c, utf8, nullptr);
    const float size_px = font_px(size);
    ImFontBaked* baked = font->GetFontBaked(size_px);
    const ImFontGlyph* glyph = baked->FindGlyph(static_cast<ImWchar>(c));
    if (!glyph)
        return;
    const float scale = size_px / baked->Size;
    const float cx = (glyph->X0 + glyph->X1) * 0.5f * scale;
    const float cy = (glyph->Y0 + glyph->Y1) * 0.5f * scale;
    dl->AddText(font, size_px, ImVec2(snap(center.x - cx), snap(center.y - cy)), col, utf8);
}

namespace {

void star4(ImDrawList* dl, const ImVec2& c, float r, ImU32 col)
{
    const float w = r * 0.28f;
    const ImVec2 pts[8] = { { c.x, c.y - r }, { c.x + w, c.y - w }, { c.x + r, c.y }, { c.x + w, c.y + w },
                            { c.x, c.y + r }, { c.x - w, c.y + w }, { c.x - r, c.y }, { c.x - w, c.y - w } };
    dl->AddConcavePolyFilled(pts, 8, col);
}

} // namespace

void any_icon(ImDrawList* dl, const ImVec2& center, ImU32 col, const char* glyph, float size)
{
    const float k = size / 11.0f;
    const auto u = [k](float v) { return px(v * k); };
    switch (glyph[0]) {
    case '\x01':
        star4(dl, ImVec2(center.x - u(1.5f), center.y + u(1.0f)), u(5.0f), col);
        star4(dl, ImVec2(center.x + u(3.6f), center.y - u(3.4f)), u(2.6f), col);
        return;
    case '\x02': {
        const float r = u(5.0f);
        constexpr int dashes = 8;
        for (int i = 0; i < dashes; ++i) {
            const float a0 = IM_PI * 2.0f * static_cast<float>(i) / dashes;
            dl->PathArcTo(center, r, a0, a0 + IM_PI * 2.0f / dashes * 0.55f, 6);
            dl->PathStroke(col, u(1.5f));
        }
        return;
    }
    case '\x03':
        dl->AddCircle(ImVec2(center.x - u(2.0f), center.y), u(3.6f), col, 24, u(1.5f));
        dl->AddCircle(ImVec2(center.x + u(2.2f), center.y), u(3.6f), col, 24, u(1.5f));
        return;
    case '\x04': {
        // Rays over the horizon: five dashes around an arc, a line under them.
        const ImVec2 c(center.x, center.y + u(2.6f));
        for (int i = 0; i < 5; ++i) {
            const float a0 = IM_PI + IM_PI * (0.1f + 0.2f * static_cast<float>(i));
            const ImVec2 d(std::cos(a0), std::sin(a0));
            dl->AddLine(ImVec2(c.x + d.x * u(3.0f), c.y + d.y * u(3.0f)), ImVec2(c.x + d.x * u(5.4f), c.y + d.y * u(5.4f)),
                        col, u(1.4f));
        }
        dl->AddLine(ImVec2(center.x - u(5.6f), c.y + u(0.6f)), ImVec2(center.x - u(1.4f), c.y + u(0.6f)), col, u(1.4f));
        dl->AddLine(ImVec2(center.x + u(1.4f), c.y + u(0.6f)), ImVec2(center.x + u(5.6f), c.y + u(0.6f)), col, u(1.4f));
        return;
    }
    case '\x05': {
        // Tree list: a stem with two branches and three bars.
        const float x0 = center.x - u(4.6f), x1 = center.x - u(1.6f);
        dl->AddRectFilled(ImVec2(x0 - u(1.0f), center.y - u(5.0f)), ImVec2(x0 + u(1.0f), center.y - u(3.0f)), col, u(0.5f));
        dl->AddLine(ImVec2(x0, center.y - u(3.0f)), ImVec2(x0, center.y + u(3.6f)), col, u(1.2f));
        for (int i = 0; i < 3; ++i) {
            const float y = center.y + u(-3.8f + 3.7f * static_cast<float>(i));
            if (i > 0)
                dl->AddLine(ImVec2(x0, y), ImVec2(x1 - u(0.4f), y), col, u(1.2f));
            dl->AddRectFilled(ImVec2(x1 + u(0.6f), y - u(0.8f)), ImVec2(center.x + u(5.2f), y + u(0.8f)), col, u(0.8f));
        }
        return;
    }
    case '\x06': {
        icon(dl, fonts().icons, size * 0.82f, ImVec2(center.x, center.y - u(1.6f)), col, nl::icon::keyboard);
        const ImVec2 tri[3] = { { center.x - u(2.2f), center.y + u(3.0f) }, { center.x + u(2.2f), center.y + u(3.0f) },
                                { center.x, center.y + u(5.2f) } };
        dl->AddTriangleFilled(tri[0], tri[1], tri[2], col);
        return;
    }
    case '\x07': {
        const float s = u(3.6f), g = u(1.3f);
        const ImVec2 o(center.x - s - g * 0.5f, center.y - s - g * 0.5f);
        dl->AddRectFilled(o, ImVec2(o.x + s, o.y + s), col, u(1.0f));
        dl->AddRectFilled(ImVec2(o.x + s + g, o.y), ImVec2(o.x + 2 * s + g, o.y + s), col, u(1.0f));
        dl->AddRectFilled(ImVec2(o.x, o.y + s + g), ImVec2(o.x + s, o.y + 2 * s + g), col, u(1.0f));
        const ImVec2 pc(o.x + s * 1.5f + g, o.y + s * 1.5f + g);
        dl->AddLine(ImVec2(pc.x - s * 0.5f, pc.y), ImVec2(pc.x + s * 0.5f, pc.y), col, u(1.3f));
        dl->AddLine(ImVec2(pc.x, pc.y - s * 0.5f), ImVec2(pc.x, pc.y + s * 0.5f), col, u(1.3f));
        return;
    }
    default:
        break;
    }
    icon(dl, fonts().icons, size, center, col, glyph);
}

void chevron_down(ImDrawList* dl, const ImVec2& c, float half_w, float half_h, ImU32 col, float thickness)
{
    const ImVec2 pts[3] = { { c.x - half_w, c.y - half_h }, { c.x, c.y + half_h }, { c.x + half_w, c.y - half_h } };
    dl->AddPolyline(pts, 3, col, thickness);
}

void chevron_up(ImDrawList* dl, const ImVec2& c, float half_w, float half_h, ImU32 col, float thickness)
{
    const ImVec2 pts[3] = { { c.x - half_w, c.y + half_h }, { c.x, c.y - half_h }, { c.x + half_w, c.y + half_h } };
    dl->AddPolyline(pts, 3, col, thickness);
}

void chevron_right(ImDrawList* dl, const ImVec2& c, float half_w, float half_h, ImU32 col, float thickness)
{
    const ImVec2 pts[3] = { { c.x - half_w, c.y - half_h }, { c.x + half_w, c.y }, { c.x - half_w, c.y + half_h } };
    dl->AddPolyline(pts, 3, col, thickness);
}

void dots(ImDrawList* dl, const ImVec2& c, ImU32 col)
{
    const float step = px(3.1f), r = px(1.15f);
    for (int i = -1; i <= 1; ++i)
        dl->AddCircleFilled(ImVec2(c.x + static_cast<float>(i) * step, c.y), r, col, 12);
}

void check_mark(ImDrawList* dl, const ImVec2& c, float size, ImU32 col, float thickness)
{
    const float s = size * 0.5f;
    const ImVec2 pts[3] = { { c.x - s, c.y }, { c.x - s * 0.3f, c.y + s * 0.7f }, { c.x + s, c.y - s * 0.75f } };
    dl->AddPolyline(pts, 3, col, thickness);
}

void icon_swap_square(ImDrawList* dl, const ImVec2& center, ImU32 fg, ImU32 cutout)
{
    const float side = snap(px(9.0f));
    const ImVec2 min(snap(center.x - side * 0.5f), snap(center.y - side * 0.5f));
    dl->AddRectFilled(min, ImVec2(min.x + side, min.y + side), fg, px(2.5f));

    // Arrows in unscaled pixels from the square's center: pointing left on top, right at the bottom.
    const ImVec2 c(min.x + side * 0.5f, min.y + side * 0.5f);
    auto at = [&](float x, float y) { return ImVec2(c.x + px(x), c.y + px(y)); };
    const float thickness = px(1.0f);
    for (int i = 0; i < 2; ++i) {
        const float dir = i == 0 ? -1.0f : 1.0f;
        const float y = 1.5f * dir, tip = 3.0f * dir, arm = 1.4f * dir;
        const ImVec2 shaft[2] = { at(-tip, y), at(tip, y) };
        const ImVec2 head[3] = { at(arm, y - 1.6f), at(tip, y), at(arm, y + 1.6f) };
        dl->AddPolyline(shaft, 2, cutout, thickness);
        dl->AddPolyline(head, 3, cutout, thickness);
    }
}

} // namespace nl::draw
