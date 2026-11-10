// Menu source watermark: @use_sberbank
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>

#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/menu.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"
#if defined(KAMSHAKO_PORTABLE_MENU)

#endif

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;

constexpr float k_height = 22.0f;
constexpr float k_margin = 12.0f;
constexpr float k_pad_x = 8.0f;
constexpr float k_gap = 10.0f;     // between items
constexpr float k_icon_gap = 5.0f; // icon -> text
constexpr float k_icon_w = 10.0f;
constexpr float k_avatar = 14.0f;
constexpr float k_drag_threshold = 4.0f;

const char* const k_item_icons[wm_count] = {
    nullptr,          icon::skull,   icon::gauge_high, icon::chart_simple, icon::display,    icon::microchip,
    icon::wave_square, icon::memory, icon::signal,     icon::chart_line,   icon::wifi,       icon::link_slash,
    icon::server,     icon::cloud,   icon::user,       icon::clock,
};

// Anchor fractions of the six snap positions: x 0 / 0.5 / 1, y 0 / 1.
ImVec2 anchor_fraction(int anchor) { return ImVec2(0.5f * static_cast<float>(anchor % 3), static_cast<float>(anchor / 3)); }

ImVec2 anchor_pos(const ImVec2& frac, const ImVec2& size)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float m = px(k_margin);
    return ImVec2(m + (display.x - size.x - 2.0f * m) * frac.x, m + (display.y - size.y - 2.0f * m) * frac.y);
}

float ease(float t) { return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }

} // namespace

void Menu::update_stats()
{
    const ImGuiIO& io = ImGui::GetIO();
    // The host has no verified player/network/hardware telemetry adapter.
    if (host_content)
        return;
    const float dt = ImMax(io.DeltaTime, 1e-4f);
    // Mouse speed in pixels per second stands in for the player's velocity.
    const float d = std::sqrt(io.MouseDelta.x * io.MouseDelta.x + io.MouseDelta.y * io.MouseDelta.y);
    speed_ = ImLerp(speed_, ImGui::IsMousePosValid() ? d / dt : 0.0f, 1.0f - std::exp(-6.0f * dt));

    stats_timer_ -= io.DeltaTime;
    if (stats_timer_ > 0.0f)
        return;
    stats_timer_ = 0.5f;
    const float t = static_cast<float>(ImGui::GetTime());
    latency_ = 9.0f + std::round(std::sin(t * 0.7f) * 1.4f + std::sin(t * 2.3f) * 0.6f);
    var_ = 0.4f + 0.3f * std::fabs(std::sin(t * 1.3f));
    gpu_load_ = 38.0f + 9.0f * std::sin(t * 0.45f) + 4.0f * std::sin(t * 1.9f);
#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        const auto u64 = [](const FILETIME& f) {
            return (static_cast<unsigned long long>(f.dwHighDateTime) << 32) | f.dwLowDateTime;
        };
        const unsigned long long i = u64(idle), total = u64(kernel) + u64(user);
        if (cpu_total_ != 0 && total > cpu_total_)
            cpu_load_ = 100.0f * (1.0f - static_cast<float>(i - cpu_idle_) / static_cast<float>(total - cpu_total_));
        cpu_idle_ = i;
        cpu_total_ = total;
    }
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem))
        memory_load_ = static_cast<float>(mem.dwMemoryLoad);
    if (cpu_mhz_ == 0.0f) {
        HKEY key;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) ==
            ERROR_SUCCESS) {
            DWORD mhz = 0, size = sizeof(mhz);
            if (RegQueryValueExA(key, "~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&mhz), &size) == ERROR_SUCCESS)
                cpu_mhz_ = static_cast<float>(mhz);
            RegCloseKey(key);
        }
        if (cpu_mhz_ == 0.0f)
            cpu_mhz_ = 3600.0f;
    }
#else
    cpu_load_ = 20.0f + 6.0f * std::sin(t);
    memory_load_ = 48.0f;
    cpu_mhz_ = 3600.0f;
#endif
}

void Menu::render_host_watermark()
{
    if (!host_content)
        return;
    if (!host_watermark_initialized_) {
        wm_.enabled.fill(false);
        int n=0;for(int item:{wm_logo,wm_username,wm_framerate,wm_time})wm_.order[static_cast<size_t>(n++)]=item;
        for(int item=0;item<wm_count;++item)if(item!=wm_logo&&item!=wm_username&&item!=wm_framerate&&item!=wm_time)wm_.order[static_cast<size_t>(n++)]=item;
        wm_.anchor=0;
        host_watermark_initialized_ = true;
    }
#if defined(KAMSHAKO_PORTABLE_MENU)
    wm_.enabled[wm_logo]=true;
    wm_.enabled[wm_username]=true;
    wm_.enabled[wm_framerate]=true;
    wm_.enabled[wm_time]=true;
    wm_.avatar=true;
#endif
    draw_watermark();
}

void Menu::draw_watermark()
{
    const Fonts& f = fonts();
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    ImFont* font = f.medium;
    const float size = font_size::watermark;
    const ImU32 tint = wm_.custom_color
                           ? ImGui::ColorConvertFloat4ToU32(ImVec4(wm_.color.x, wm_.color.y, wm_.color.z, 1.0f))
                           : col::accent;

    // Item texts.
    char texts[wm_count][48] = {};
    {
        std::snprintf(texts[wm_kd], 48, "1.42 KD");
        std::snprintf(texts[wm_speed], 48, "%d SPEED", static_cast<int>(speed_ + 0.5f));
        std::snprintf(texts[wm_framerate], 48, "%d FPS", static_cast<int>(fps_ + 0.5f));
        std::snprintf(texts[wm_gpu], 48, "%d%% GPU", static_cast<int>(gpu_load_ + 0.5f));
        std::snprintf(texts[wm_cpu], 48, "%d%% CPU", static_cast<int>(cpu_load_ + 0.5f));
        std::snprintf(texts[wm_cpu_freq], 48, "%.2f GHZ", cpu_mhz_ / 1000.0f);
        std::snprintf(texts[wm_memory], 48, "%d%% RAM", static_cast<int>(memory_load_ + 0.5f));
        std::snprintf(texts[wm_latency], 48, "%d MS", static_cast<int>(latency_));
        std::snprintf(texts[wm_var], 48, "%.1f VAR", var_);
        std::snprintf(texts[wm_loss], 48, "0%% LOSS");
        std::snprintf(texts[wm_connection], 48, "STABLE");
        std::snprintf(texts[wm_server], 48, "Valve CS2 EU West");
        std::snprintf(texts[wm_preset], 48, "%s", presets_[static_cast<size_t>(preset_)]->name.c_str());
        std::snprintf(texts[wm_username], 48, "%s", user_name);
        const std::time_t now = std::time(nullptr);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &now);
#else
        localtime_r(&now, &tm);
#endif
        std::snprintf(texts[wm_time], 48, "%02d:%02d", tm.tm_hour, tm.tm_min);
        if (host_content) {
            // Retain the donor's optional item layout without displaying demo data.
            std::snprintf(texts[wm_kd], 48, "-- KD");
            std::snprintf(texts[wm_speed], 48, "-- SPEED");
            std::snprintf(texts[wm_gpu], 48, "-- GPU");
            std::snprintf(texts[wm_cpu], 48, "-- CPU");
            std::snprintf(texts[wm_cpu_freq], 48, "-- GHZ");
            std::snprintf(texts[wm_memory], 48, "-- RAM");
            std::snprintf(texts[wm_latency], 48, "-- MS");
            std::snprintf(texts[wm_var], 48, "-- VAR");
            std::snprintf(texts[wm_loss], 48, "-- LOSS");
            std::snprintf(texts[wm_connection], 48, "-- CONNECTION");
            std::snprintf(texts[wm_server], 48, "-- SERVER");
            std::snprintf(texts[wm_username], 48, "@use_sberbank");
        }
    }
    const float logo_w = draw::text_width(f.black, font_size::watermark_logo, "NL");
    const auto content_w = [&](int item) {
        if (item == wm_logo)
            return logo_w;
        return px(k_icon_w + k_icon_gap) + draw::text_width(font, size, texts[item]);
    };

    // Animated widths and positions of the items inside the bar.
    const float k = 1.0f - std::exp(-16.0f * dt);
    const bool first = wm_pos_.x < 0.0f;
    float x = px(k_pad_x), last_gap = 0.0f;
    std::array<float, wm_count> target_x{};
    for (int item : wm_.order) {
        const size_t i = static_cast<size_t>(item);
        const bool on = wm_.enabled[i];
        const float tw = on ? content_w(item) : 0.0f;
        wm_shown_[i] = first ? (on ? 1.0f : 0.0f) : ImClamp(wm_shown_[i] + (on ? dt : -dt) * 7.0f, 0.0f, 1.0f);
        wm_width_[i] = first ? tw : ImLerp(wm_width_[i], tw, k);
        target_x[i] = x;
        if (wm_width_[i] > 0.5f) {
            const float gap = px(k_gap) * ease(wm_shown_[i]);
            x += wm_width_[i] + gap;
            last_gap = gap;
        }
        if (first || wm_x_[i] == 0.0f)
            wm_x_[i] = target_x[i];
    }
    wm_avatar_ = ImClamp(wm_avatar_ + (wm_.avatar ? dt : -dt) * 7.0f, 0.0f, 1.0f);
    const float avatar_w = (px(k_gap) * 0.6f + px(k_avatar) - px(3.0f)) * ease(wm_avatar_);
    const float bar_w = snap(ImMax(px(30.0f), x - last_gap + avatar_w + px(k_pad_x)));
    const float bar_h = snap(px(k_height));
    wm_bar_w_ = bar_w;

    // Position: animated between the snap points; it never follows the cursor freely.
    const ImVec2 frac = anchor_fraction(wm_moving_ && wm_hover_anchor_ >= 0 ? wm_hover_anchor_ : wm_.anchor);
    if (first)
        wm_pos_ = frac;
    wm_pos_.x = ImLerp(wm_pos_.x, frac.x, 1.0f - std::exp(-12.0f * dt));
    wm_pos_.y = ImLerp(wm_pos_.y, frac.y, 1.0f - std::exp(-12.0f * dt));
    const ImVec2 pos = anchor_pos(wm_pos_, ImVec2(bar_w, bar_h));
    const ImVec2 min(std::floor(pos.x), std::floor(pos.y)), max(min.x + bar_w, min.y + bar_h);

    const bool interactive = visible_ && visibility_ > 0.5f;
    ImGui::SetNextWindowPos(min);
    ImGui::SetNextWindowSize(ImVec2(bar_w, bar_h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ImGui::Begin("##watermark", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoMove | (interactive ? 0 : ImGuiWindowFlags_NoInputs));
    ImGui::PopStyleVar(3);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float rounding = px(6.0f);
    const float cy = min.y + bar_h * 0.5f;

    // Interaction: drag an item to reorder, drag the logo or the background to pick a snap position.
    const ImGuiID bar_id = ImGui::GetID("##bar");
    int hovered_item = -1;
    for (int item : wm_.order) {
        const size_t i = static_cast<size_t>(item);
        if (!wm_.enabled[i] || item == wm_logo)
            continue;
        const float ix = min.x + wm_x_[i];
        if (ImGui::IsMouseHoveringRect(ImVec2(ix - px(3.0f), min.y), ImVec2(ix + wm_width_[i] + px(3.0f), max.y)))
            hovered_item = item;
    }
    const ui::ItemState bs = ui::item_behavior(bar_id, min, max, interactive);
    static int s_press_item = -1;
    static ImVec2 s_press_pos{};
    if (bs.held && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        s_press_item = hovered_item;
        s_press_pos = io.MousePos;
    }
    if (bs.held && wm_drag_item_ < 0 && !wm_moving_) {
        const float dx = io.MousePos.x - s_press_pos.x, dy = io.MousePos.y - s_press_pos.y;
        if (dx * dx + dy * dy > px(k_drag_threshold) * px(k_drag_threshold)) {
            if (s_press_item >= 0) {
                wm_drag_item_ = s_press_item;
                wm_drag_dx_ = s_press_pos.x - (min.x + wm_x_[static_cast<size_t>(s_press_item)]);
            } else {
                wm_moving_ = true;
            }
        }
    }
    const float display_w = io.DisplaySize.x, display_h = io.DisplaySize.y;
    if (wm_moving_) {
        // Nearest snap position to the cursor.
        const ImVec2 m = io.MousePos;
        float best = FLT_MAX;
        for (int a = 0; a < 6; ++a) {
            const ImVec2 p = anchor_pos(anchor_fraction(a), ImVec2(bar_w, bar_h));
            const float ddx = m.x - (p.x + bar_w * 0.5f), ddy = m.y - (p.y + bar_h * 0.5f);
            const float d2 = ddx * ddx + ddy * ddy;
            if (d2 < best) {
                best = d2;
                wm_hover_anchor_ = a;
            }
        }
        if (!bs.held) {
            wm_.anchor = wm_hover_anchor_;
            wm_moving_ = false;
        }
    }
    if (wm_drag_item_ >= 0) {
        const size_t di = static_cast<size_t>(wm_drag_item_);
        const float dragged_x = ImClamp(io.MousePos.x - wm_drag_dx_ - min.x, px(k_pad_x), bar_w - px(k_pad_x) - wm_width_[di]);
        wm_x_[di] = dragged_x;
        // Swap with a neighbour once the dragged item passes its middle.
        int slot = -1;
        for (int s = 0; s < wm_count; ++s)
            if (wm_.order[static_cast<size_t>(s)] == wm_drag_item_)
                slot = s;
        const float center = dragged_x + wm_width_[di] * 0.5f;
        for (int dir = -1; dir <= 1; dir += 2) {
            int s = slot + dir;
            while (s >= 0 && s < wm_count && !wm_.enabled[static_cast<size_t>(wm_.order[static_cast<size_t>(s)])])
                s += dir;
            if (s < 0 || s >= wm_count)
                continue;
            const size_t ni = static_cast<size_t>(wm_.order[static_cast<size_t>(s)]);
            if (ni == wm_logo)
                continue;
            const float ncenter = target_x[ni] + wm_width_[ni] * 0.5f;
            if ((dir < 0 && center < ncenter) || (dir > 0 && center > ncenter)) {
                std::swap(wm_.order[static_cast<size_t>(slot)], wm_.order[static_cast<size_t>(s)]);
                break;
            }
        }
        if (!bs.held)
            wm_drag_item_ = -1;
    }
    for (int item : wm_.order) {
        const size_t i = static_cast<size_t>(item);
        if (item != wm_drag_item_)
            wm_x_[i] = first ? target_x[i] : ImLerp(wm_x_[i], target_x[i], k);
    }

    // Snap guides while moving.
    wm_guides_ = ImClamp(wm_guides_ + (wm_moving_ ? dt : -dt) * 8.0f, 0.0f, 1.0f);
    if (wm_guides_ > 0.0f) {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        const float ga = ease(wm_guides_);
        fg->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(display_w, display_h), draw::with_alpha(IM_COL32(0, 0, 0, 70), ga));
        // Slots are drawn narrower than the bar so neighbours don't overlap on small screens.
        const float slot_w = ImMin(bar_w, display_w * 0.28f);
        for (int a = 0; a < 6; ++a) {
            const ImVec2 p = anchor_pos(anchor_fraction(a), ImVec2(slot_w, bar_h));
            const bool hot = a == wm_hover_anchor_;
            const float h = ui::animate(ImGui::GetID("##slot") + static_cast<ImGuiID>(a), hot ? 1.0f : 0.0f, 16.0f);
            const float grow = px(3.0f) * h;
            const ImVec2 smin(snap(p.x - grow), snap(p.y - grow)), smax(snap(p.x + slot_w + grow), snap(p.y + bar_h + grow));
            fg->AddRectFilled(smin, smax, draw::with_alpha(tint, ga * (0.05f + 0.12f * h)), rounding + grow);
            draw::border(fg, smin, smax, draw::with_alpha(draw::lerp_color(IM_COL32(255, 255, 255, 60), tint, h), ga),
                         rounding + grow, draw::hairline());
            const char* name = watermark_anchors[a];
            const float tw = draw::text_width(font, size, name);
            const bool below = a < 3;
            const float ty = below ? smax.y + px(6.0f) : smin.y - px(6.0f) - draw::cap_height(font, size);
            draw::text(fg, font, size, ImVec2(snap(ImClamp((smin.x + smax.x - tw) * 0.5f, px(4.0f), display_w - tw - px(4.0f))), snap(ty)),
                       draw::with_alpha(draw::lerp_color(IM_COL32(200, 202, 212, 255), tint, h), ga * (0.6f + 0.4f * h)), name);
        }
    }

    // Background
    if (wm_.background) {
        ui::glass(dl, min, max, rounding, col::overlay_pill, wm_.shadow);
        draw::border(dl, min, max, wm_moving_ ? draw::with_alpha(tint, 0.6f) : col::window_border, rounding, draw::hairline());
    } else if (wm_.shadow) {
        dl->PushClipRectFullScreen();
        draw::soft_shadow(dl, min, max, rounding, px(10.0f), IM_COL32(0, 0, 0, 70));
        dl->PopClipRect();
    }

    // Items
    dl->PushClipRect(min, max, true);
    const float text_y = cy - draw::cap_height(font, size) * 0.5f;
    for (int item : wm_.order) {
        const size_t i = static_cast<size_t>(item);
        if (wm_shown_[i] <= 0.0f || wm_width_[i] < 0.5f)
            continue;
        const float a = ease(wm_shown_[i]);
        const int v0 = dl->VtxBuffer.Size;
        const float ix = min.x + wm_x_[i];
        const bool dragged = item == wm_drag_item_;
        if (dragged)
            dl->AddRectFilled(ImVec2(ix - px(4.0f), min.y + px(3.0f)), ImVec2(ix + wm_width_[i] + px(4.0f), max.y - px(3.0f)),
                              draw::with_alpha(tint, 0.16f), px(4.0f));
        const float hover = ui::animate(ImGui::GetID("##item_h") + static_cast<ImGuiID>(item),
                                        (hovered_item == item && interactive && wm_drag_item_ < 0 && !wm_moving_) ? 1.0f : 0.0f, 16.0f);
        if (hover > 0.0f && !dragged)
            dl->AddRectFilled(ImVec2(ix - px(4.0f), min.y + px(3.0f)), ImVec2(ix + wm_width_[i] + px(4.0f), max.y - px(3.0f)),
                              draw::with_alpha(IM_COL32(255, 255, 255, 14), hover), px(4.0f));
        if (item == wm_logo) {
            draw::text_glow(dl, f.black, font_size::watermark_logo,
                            ImVec2(snap(ix), cy - draw::cap_height(f.black, font_size::watermark_logo) * 0.5f),
                            col::watermark_logo, draw::with_alpha(tint, 0.3f), px(1.0f), "NL");
        } else {
            draw::any_icon(dl, ImVec2(ix + px(k_icon_w) * 0.5f, cy), tint, k_item_icons[i], 10.0f);
            draw::text(dl, font, size, ImVec2(snap(ix + px(k_icon_w + k_icon_gap)), text_y), col::watermark_text, texts[i]);
        }
        // Items leave by fading and shrinking towards their middle.
        fade_vertices(dl, v0, a, 0.0f);
        if (a < 1.0f)
            for (int v = v0; v < dl->VtxBuffer.Size; ++v) {
                ImDrawVert& vert = dl->VtxBuffer[v];
                vert.pos.y = cy + (vert.pos.y - cy) * (0.6f + 0.4f * a);
            }
    }
    if (wm_avatar_ > 0.0f) {
        const float d = snap(px(k_avatar));
        const float ae = ease(wm_avatar_);
        const ImVec2 amin(snap(max.x - px(k_pad_x) - d + px(3.0f) + (1.0f - ae) * px(6.0f)), snap(cy - d * 0.5f));
        draw_avatar(dl, amin, d, ae);
    }
    dl->PopClipRect();

    // Hint and context menu.
    const bool hovered = interactive && ImGui::IsWindowHovered() && !wm_moving_ && wm_drag_item_ < 0;
    {
        static float s_hover_time = 0.0f;
        s_hover_time = hovered ? s_hover_time + dt : 0.0f;
        const float ha = ui::animate(ImGui::GetID("##hint"), (hovered && s_hover_time > 0.45f) || wm_moving_ ? 1.0f : 0.0f, 12.0f);
        if (ha > 0.0f) {
            ImDrawList* fg = ImGui::GetForegroundDrawList();
            const char* hint = wm_moving_ ? "Release to snap here" : "Drag to move  \xc2\xb7  Drag items to reorder  \xc2\xb7  RMB for options";
            const float tw = draw::text_width(font, font_size::small, hint);
            const float hw = snap(tw + px(16.0f)), hh = snap(px(20.0f));
            const bool below = wm_pos_.y < 0.5f;
            float hx = snap(ImClamp((min.x + max.x - hw) * 0.5f, px(4.0f), display_w - hw - px(4.0f)));
            const float slide = (1.0f - ha) * px(4.0f);
            const float hy = snap(below ? max.y + px(6.0f) - slide : min.y - px(6.0f) - hh + slide);
            fg->AddRectFilled(ImVec2(hx, hy), ImVec2(hx + hw, hy + hh), draw::with_alpha(col::tooltip_bg, ha), px(5.0f));
            draw::border(fg, ImVec2(hx, hy), ImVec2(hx + hw, hy + hh), draw::with_alpha(col::window_border, ha), px(5.0f),
                         draw::hairline());
            draw::text(fg, font, font_size::small, ImVec2(hx + px(8.0f), hy + (hh - draw::cap_height(font, font_size::small)) * 0.5f),
                       draw::with_alpha(col::tooltip_text, ha), hint);
        }
    }

    const ImGuiID ctx_id = ImGui::GetID("##wm_context");
    static ImVec2 s_ctx_pos{};
    if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup(ctx_id);
        s_ctx_pos = ImVec2(io.MousePos.x + px(4.0f), io.MousePos.y + px(4.0f));
    }
    bool open_settings = false;
    if (ui::begin_popup_at(ctx_id, s_ctx_pos, px(170.0f), ImVec2(px(6.0f), px(6.0f)))) {
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();
        draw::text(pdl, font, font_size::header, ImVec2(p.x + px(4.0f), p.y + px(4.0f)), col::section_header, "POSITION");
        ImGui::Dummy(ImVec2(1.0f, px(16.0f)));
        const int a = ui::dropdown_list(watermark_anchors, wm_.anchor);
        if (a >= 0) {
            wm_.anchor = a;
            ImGui::CloseCurrentPopup();
        }
        ImGui::Dummy(ImVec2(1.0f, px(4.0f)));
        const ImVec2 sp = ImGui::GetCursorScreenPos();
        draw::hline(pdl, sp.x, sp.x + ImGui::GetContentRegionAvail().x, sp.y, col::window_separator);
        ImGui::Dummy(ImVec2(1.0f, px(4.0f)));
        if (ui::dropdown_item("Settings...", false, false)) {
            open_settings = true;
            ImGui::CloseCurrentPopup();
        }
        ui::end_popup();
    }
    if (open_settings)
        ui::popup::open("##wm_settings");
    if (ui::popup::begin("##wm_settings", "Watermark")) {
        watermark_settings();
        ui::popup::end();
    }
    ImGui::End();
}

void Menu::watermark_settings()
{
    int shown = 0;
    for (bool on : wm_.enabled)
        shown += on ? 1 : 0;
    ui::popup::group([&] {
        if (!search::collecting()) {
            const bool pressed = ui::popup::item("Items", icon::list_ul);
            const ImVec2 rmax = ImGui::GetItemRectMax(), rmin = ImGui::GetItemRectMin();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const float cy = (rmin.y + rmax.y) * 0.5f;
            char count[16];
            std::snprintf(count, sizeof(count), "%d", shown);
            ImFont* font = fonts().medium;
            const float tw = draw::text_width(font, font_size::popup_label, count);
            const float chevron_x = rmax.x - px(13.0f);
            draw::text(dl, font, font_size::popup_label,
                       ImVec2(chevron_x - px(8.0f) - tw, cy - draw::cap_height(font, font_size::popup_label) * 0.5f),
                       col::window_value, count);
            draw::chevron_right(dl, ImVec2(chevron_x, cy), px(1.8f), px(3.6f), col::window_value, px(1.5f));
            if (pressed)
                ui::popup::open("##wm_items");
            if (ui::popup::begin("##wm_items", "Items", 220.0f)) {
                watermark_items_list();
                ui::popup::end();
            }
        }
        ui::popup::color_toggle("Additional Color", &wm_.color, &wm_.custom_color, icon::fill_drip);
        ui::popup::toggle("Display Avatar", &wm_.avatar, icon::user);
    });
    ui::popup::group([&] {
        ui::popup::toggle("Background", &wm_.background, icon::image);
        ui::popup::toggle("Shadow", &wm_.shadow, icon::layers);
        ui::popup::value("Position", &wm_.anchor, watermark_anchors, true, icon::arrows);
    });
}

void Menu::watermark_items_list()
{
    if (search::collecting())
        return;
    const Fonts& f = fonts();
    const ImGuiIO& io = ImGui::GetIO();
    ui::popup::group([&] {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;
        const float row_h = snap(px(24.0f));
        const ImVec2 p0 = ImGui::GetCursorScreenPos();
        const float w = ImGui::GetContentRegionAvail().x;
        ImGui::ItemSize(ImVec2(w, row_h * static_cast<float>(wm_count)));
        const ImU32 tint = wm_.custom_color
                               ? ImGui::ColorConvertFloat4ToU32(ImVec4(wm_.color.x, wm_.color.y, wm_.color.z, 1.0f))
                               : col::accent;

        // Rows can be dragged by their grip; the others make room smoothly.
        if (wm_list_drag_ >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
            wm_list_drag_ = -1;
        if (wm_list_drag_ >= 0) {
            int slot = 0;
            for (int s = 0; s < wm_count; ++s)
                if (wm_.order[static_cast<size_t>(s)] == wm_list_drag_)
                    slot = s;
            const int want = ImClamp(static_cast<int>((io.MousePos.y - wm_list_grab_ - p0.y + row_h * 0.5f) / row_h), 0, wm_count - 1);
            if (want != slot) {
                const int item = wm_.order[static_cast<size_t>(slot)];
                if (want > slot)
                    for (int s = slot; s < want; ++s)
                        wm_.order[static_cast<size_t>(s)] = wm_.order[static_cast<size_t>(s + 1)];
                else
                    for (int s = slot; s > want; --s)
                        wm_.order[static_cast<size_t>(s)] = wm_.order[static_cast<size_t>(s - 1)];
                wm_.order[static_cast<size_t>(want)] = item;
            }
        }

        for (int s = 0; s < wm_count; ++s) {
            const int item = wm_.order[static_cast<size_t>(s)];
            const size_t i = static_cast<size_t>(item);
            ImGui::PushID(item);
            const ImGuiID id = ImGui::GetID("##wm_row");
            const float target_y = p0.y + row_h * static_cast<float>(s);
            const bool dragged = item == wm_list_drag_;
            float* ry = ImGui::GetStateStorage()->GetFloatRef(id + 9, target_y);
            if (ImGui::IsWindowAppearing())
                *ry = target_y;
            *ry = dragged ? ImClamp(io.MousePos.y - wm_list_grab_, p0.y, p0.y + row_h * static_cast<float>(wm_count - 1))
                          : ImLerp(*ry, target_y, 1.0f - std::exp(-18.0f * io.DeltaTime));
            const float y = snap(*ry);
            const ImVec2 rmin(p0.x + px(3.0f), y), rmax(p0.x + w - px(3.0f), y + row_h);
            const ImVec2 grip_c(rmin.x + px(10.0f), y + row_h * 0.5f);

            const ImGuiID grip_id = ImGui::GetID("##grip");
            const ui::ItemState gs = ui::item_behavior(grip_id, ImVec2(rmin.x, y), ImVec2(rmin.x + px(20.0f), y + row_h));
            if (gs.held && wm_list_drag_ < 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.0f)) {
                wm_list_drag_ = item;
                wm_list_grab_ = io.MouseClickedPos[0].y - target_y;
            }
            const ui::ItemState rs = ui::item_behavior(id, ImVec2(rmin.x + px(20.0f), y), rmax);
            if (rs.pressed)
                wm_.enabled[i] = !wm_.enabled[i];
            const float hv = ui::animate(id + 1, (rs.hovered || gs.hovered || dragged) ? 1.0f : 0.0f, 18.0f);
            const float on = ui::animate(id + 2, wm_.enabled[i] ? 1.0f : 0.0f, 16.0f);
            if (hv > 0.0f || dragged)
                dl->AddRectFilled(rmin, rmax, draw::with_alpha(dragged ? draw::lerp_color(col::window_row_hover, tint, 0.15f) : col::window_row_hover,
                                                             dragged ? 1.0f : hv),
                                  px(5.0f));
            draw::icon(dl, f.icons, 9.0f, grip_c, draw::lerp_color(col::window_label_disabled, col::window_value, hv), icon::grip);
            const float cy = y + row_h * 0.5f;
            const ImU32 icon_col = draw::lerp_color(col::window_label_disabled, tint, on);
            if (item == wm_logo)
                draw::text(dl, f.black, 9.5f, ImVec2(rmin.x + px(22.0f), cy - draw::cap_height(f.black, 9.5f) * 0.5f), icon_col, "NL");
            else
                draw::any_icon(dl, ImVec2(rmin.x + px(30.0f), cy), icon_col, k_item_icons[i], 10.0f);
            draw::text(dl, f.medium, font_size::popup_label,
                       ImVec2(snap(rmin.x + px(44.0f) + px(1.5f) * hv), cy - draw::cap_height(f.medium, font_size::popup_label) * 0.5f),
                       draw::lerp_color(col::window_value, col::window_label, on), watermark_item_names[i]);
            if (on > 0.0f)
                draw::check_mark(dl, ImVec2(rmax.x - px(12.0f), cy), px(8.0f) * (0.6f + 0.4f * on), draw::with_alpha(tint, on),
                                 px(1.6f));
            ImGui::PopID();
        }
    });
}

} // namespace nl
