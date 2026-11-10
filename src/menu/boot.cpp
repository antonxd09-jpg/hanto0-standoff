// Menu source watermark: @use_sberbank
#include "menu/boot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl::boot {
namespace {

using namespace nl::style;
using draw::snap;

constexpr float k_window_w = 268.0f, k_window_h = 92.0f, k_window_rounding = 9.0f;
constexpr float k_pad = 14.0f;
constexpr float k_logo = 30.0f, k_logo_rounding = 7.0f;
constexpr float k_bar_h = 4.0f;
constexpr float k_hold = 0.35f, k_fade_in = 0.3f, k_fade_out = 0.35f;

constexpr float k_toast_w = 226.0f, k_toast_h = 44.0f, k_toast_margin = 18.0f, k_toast_logo = 26.0f;
constexpr float k_toast_life = 3.6f, k_toast_slide = 0.35f;

struct Toast {
    std::string text;
    float age = 0.0f;
};

std::vector<Toast> g_toasts;

float ease_out(float t)
{
    t = ImSaturate(t);
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

// Multiplies the alpha of the vertices added since `first` and moves them by `offset`.
void fade(ImDrawList* dl, int first, float alpha, const ImVec2& offset = ImVec2(0.0f, 0.0f))
{
    for (int i = first; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        v.pos.x += offset.x;
        v.pos.y += offset.y;
        const float a = static_cast<float>((v.col >> IM_COL32_A_SHIFT) & 0xFF) * alpha;
        v.col = (v.col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a + 0.5f) << IM_COL32_A_SHIFT);
    }
}

// The sidebar logo at any size: dark tile, glowing "K".
void logo(ImDrawList* dl, const ImVec2& min, float size)
{
    const float scale = size / px(layout::logo_w);
    const ImVec2 max(min.x + size, min.y + size);
    dl->AddRectFilled(min, max, col::logo_bg, px(k_logo_rounding) * scale);
    const Fonts& f = fonts();
    const float font = font_size::logo * scale;
    const float w = draw::text_width(f.black, font, "P");
    draw::text_glow(dl, f.black, font,
                    ImVec2(snap((min.x + max.x - w) * 0.5f), snap((min.y + max.y - draw::cap_height(f.black, font)) * 0.5f)),
                    col::logo_text, col::logo_glow, px(layout::logo_glow_radius) * scale, "P");
}

} // namespace

bool loading(float progress, const char* status, bool done)
{
    static float s_age = 0.0f, s_done_age = -1.0f, s_shown = 0.0f;
    const float dt = ImGui::GetIO().DeltaTime;
    s_age += dt;
    if (done && s_done_age < 0.0f)
        s_done_age = 0.0f;
    else if (s_done_age >= 0.0f)
        s_done_age += dt;

    // The bar never jumps: it chases the reported progress.
    const float target = done ? 1.0f : ImSaturate(progress);
    s_shown = ImLerp(s_shown, target, 1.0f - std::exp(-(done ? 14.0f : 6.0f) * dt));
    if (done && s_shown > 0.995f)
        s_shown = 1.0f;

    float alpha = ease_out(s_age / k_fade_in);
    float lift = 0.0f;
    if (s_done_age >= 0.0f && s_shown >= 1.0f) {
        const float out = ImSaturate((s_done_age - k_hold) / k_fade_out);
        alpha *= 1.0f - out;
        lift = out * px(10.0f);
        if (out >= 1.0f)
            return true;
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float w = snap(px(k_window_w)), h = snap(px(k_window_h));
    const ImVec2 min(snap((display.x - w) * 0.5f), snap((display.y - h) * 0.5f));
    const ImVec2 max(min.x + w, min.y + h);
    const int first = dl->VtxBuffer.Size;

    ui::glass(dl, min, max, px(k_window_rounding), col::window_bg, true);
    draw::border(dl, min, max, col::window_border, px(k_window_rounding), draw::hairline());

    const Fonts& f = fonts();
    const float pad = px(k_pad);
    const float logo_size = snap(px(k_logo));
    const ImVec2 logo_min(snap(min.x + pad), snap(min.y + pad));
    logo(dl, logo_min, logo_size);

    const float text_x = snap(logo_min.x + logo_size + px(10.0f));
    draw::text(dl, f.semibold, font_size::title, ImVec2(text_x, snap(logo_min.y + px(3.0f))), col::title, "Pyzirik");
    draw::text(dl, f.medium, font_size::subtitle, ImVec2(text_x, snap(logo_min.y + logo_size - px(9.0f))), col::subtitle,
               done ? "Ready" : (status && *status ? status : "Loading"));

    // Progress bar with the percentage over its right end.
    const float bar_y = snap(max.y - pad - px(k_bar_h));
    const ImVec2 bar_min(snap(min.x + pad), bar_y), bar_max(snap(max.x - pad), bar_y + snap(px(k_bar_h)));
    const float r = (bar_max.y - bar_min.y) * 0.5f;
    dl->AddRectFilled(bar_min, bar_max, col::window_chip_bg, r);
    const float fill_x = ImLerp(bar_min.x, bar_max.x, s_shown);
    if (fill_x > bar_min.x + 1.0f) {
        dl->AddRectFilled(bar_min, ImVec2(fill_x, bar_max.y), col::accent, r);
        // A soft light running along the filled part while it is still loading.
        if (!done) {
            const float t = std::fmod(s_age * 0.9f, 1.0f);
            const float cx = ImLerp(bar_min.x, fill_x, t);
            const float half = px(26.0f);
            dl->AddRectFilledMultiColor(ImVec2(ImMax(bar_min.x, cx - half), bar_min.y), ImVec2(ImMin(fill_x, cx), bar_max.y),
                                        IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 90),
                                        IM_COL32(255, 255, 255, 90), IM_COL32(255, 255, 255, 0));
            dl->AddRectFilledMultiColor(ImVec2(ImMin(fill_x, cx), bar_min.y), ImVec2(ImMin(fill_x, cx + half), bar_max.y),
                                        IM_COL32(255, 255, 255, 90), IM_COL32(255, 255, 255, 0),
                                        IM_COL32(255, 255, 255, 0), IM_COL32(255, 255, 255, 90));
        }
    }
    char percent[8];
    std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(s_shown * 100.0f + 0.5f));
    const float pw = draw::text_width(f.medium, font_size::subtitle, percent);
    draw::text(dl, f.medium, font_size::subtitle, ImVec2(snap(bar_max.x - pw), snap(bar_min.y - px(12.0f))), col::subtitle,
               percent);

    fade(dl, first, alpha, ImVec2(0.0f, -lift + (1.0f - ease_out(s_age / k_fade_in)) * px(8.0f)));
    return false;
}

void notify(const char* text)
{
    g_toasts.push_back(Toast{ text ? text : "", 0.0f });
}

void notifications()
{
    if (g_toasts.empty())
        return;
    const float dt = ImGui::GetIO().DeltaTime;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const Fonts& f = fonts();

    float y = snap(px(k_toast_margin));
    for (Toast& t : g_toasts) {
        t.age += dt;
        const float in = ease_out(t.age / k_toast_slide);
        const float out = ease_out((t.age - k_toast_life) / k_toast_slide);
        const float shown = in * (1.0f - out);

        const float w = snap(px(k_toast_w)), h = snap(px(k_toast_h));
        const ImVec2 min(snap(display.x - px(k_toast_margin) - w), y), max(min.x + w, min.y + h);
        const int first = dl->VtxBuffer.Size;
        ui::glass(dl, min, max, px(8.0f), col::window_bg, true);
        draw::border(dl, min, max, col::window_border, px(8.0f), draw::hairline());

        const float logo_size = snap(px(k_toast_logo));
        const ImVec2 logo_min(snap(min.x + (h - logo_size) * 0.5f), snap(min.y + (h - logo_size) * 0.5f));
        logo(dl, logo_min, logo_size);
        const float text_x = snap(logo_min.x + logo_size + px(10.0f));
        const float cy = (min.y + max.y) * 0.5f;
        draw::text(dl, f.semibold, font_size::text, ImVec2(text_x, snap(cy - px(8.0f))), col::title, "Pyzirik");
        draw::text(dl, f.medium, font_size::text, ImVec2(text_x, snap(cy + px(3.0f))), col::window_value, t.text.c_str());

        // Time left, as a thin accent line along the bottom edge.
        const float left = 1.0f - ImSaturate(t.age / k_toast_life);
        if (left > 0.0f) {
            const float inset = px(8.0f);
            dl->AddRectFilled(ImVec2(min.x + inset, max.y - px(2.0f)),
                              ImVec2(min.x + inset + (w - inset * 2.0f) * left, max.y - px(1.0f)),
                              draw::with_alpha(col::accent, 0.85f), px(1.0f));
        }

        fade(dl, first, shown, ImVec2((1.0f - shown) * (w * 0.6f), 0.0f));
        y += (h + px(8.0f)) * shown;
    }
    g_toasts.erase(std::remove_if(g_toasts.begin(), g_toasts.end(),
                                  [](const Toast& t) { return t.age > k_toast_life + k_toast_slide; }),
                   g_toasts.end());
}

} // namespace nl::boot
