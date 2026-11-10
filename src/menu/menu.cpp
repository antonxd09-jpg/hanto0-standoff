// Menu source watermark: @use_sberbank
#include "menu/menu.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "assets.h"
#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/blur.h"
#include "menu/fonts.h"
#include "menu/images.h"
#include "menu/localization.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;

constexpr const char* k_tab_names[] = { "Misc", "Aim", "Visuals", "Skins", "Settings" };
constexpr const char* k_tab_icons[] = { icon::crosshairs, icon::mouse, icon::image, icon::layers, nullptr };
constexpr int k_tab_count = 5;
constexpr const char* k_subtab_names[] = { "Players", "World", "Misc" };
constexpr const char* k_subtab_icons[] = { icon::user, icon::earth, icon::gear };

constexpr const char* k_languages[] = { "English", "Russian", "German", "Chinese" };
constexpr const char* k_scales[] = { "Auto", "75%", "100%", "125%", "150%", "175%", "200%", "250%" };
constexpr float k_scale_values[] = { 0.0f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.5f };
constexpr const char* k_units[] = { "Auto", "Meters", "Feet", "Units" };
constexpr const char* k_safe_modes[] = { "Automatic", "Enabled", "Disabled" };
constexpr const char* k_username = user_name;

constexpr float k_scale_step = 0.1f;
// The avatar image spans a 30 px circle in the reference; the menu shows the middle avatar_r of it.
constexpr float k_avatar_image_r = 15.0f;
// The avatar is resampled to its on-screen size and drawn 1:1. A border repeating the edge pixels
// keeps the anti-aliased rim of the circle from sampling outside the image.
constexpr int k_avatar_border = 1;
constexpr int k_avatar_tex_side =
    static_cast<int>(2.0f * layout::avatar_r * Menu::max_scale + 0.5f) + 2 * k_avatar_border;

// Menu coordinates (unscaled, relative to the top-left corner `o`) to whole screen pixels.
ImVec2 at(const ImVec2& o, float x, float y) { return ImVec2(o.x + snap(px(x)), o.y + snap(px(y))); }

ImVec2 menu_size(float scale = ui_scale) { return ImVec2(snap(layout::width * scale), snap(layout::height * scale)); }

ImVec4 to_vec4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

float text_cap_y_centered(ImFont* font, float size, float center_y)
{
    return center_y - draw::cap_height(font, size) * 0.5f;
}

// Ctrl + key, with or without Shift (Ctrl + "+" is Ctrl + Shift + = on most layouts), but not with
// Alt, since AltGr arrives as Ctrl + Alt. A Ctrl release queued in the same frame as the key press
// still counts.
bool chord_pressed(ImGuiKey key, ImGuiKey keypad_key)
{
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyAlt || !(io.KeyCtrl || ImGui::IsKeyReleased(ImGuiMod_Ctrl)))
        return false;
    return ImGui::IsKeyPressed(key) || ImGui::IsKeyPressed(keypad_key);
}

int avatar_side() { return static_cast<int>(snap(px(2.0f * layout::avatar_r))); }

float catmull_rom(float x)
{
    x = std::fabs(x);
    if (x < 1.0f)
        return (1.5f * x - 2.5f) * x * x + 1.0f;
    if (x < 2.0f)
        return ((-0.5f * x + 2.5f) * x - 4.0f) * x + 2.0f;
    return 0.0f;
}

// Filter weights for resampling `len` source pixels, starting at `x0`, onto `n` pixels. When
// shrinking, the kernel is widened by the shrink factor so that every source pixel contributes.
struct Kernel {
    int taps = 0;
    std::vector<int> first;     // first source pixel of each output pixel
    std::vector<float> weights; // n * taps
};

Kernel make_kernel(float x0, float len, int n)
{
    const float step = len / static_cast<float>(n);
    const float widen = ImMax(1.0f, step);
    Kernel k;
    k.taps = static_cast<int>(std::ceil(4.0f * widen)) + 2;
    k.first.resize(static_cast<size_t>(n));
    k.weights.resize(static_cast<size_t>(n * k.taps));
    for (int i = 0; i < n; ++i) {
        const float center = x0 + (static_cast<float>(i) + 0.5f) * step;
        const int first = static_cast<int>(std::floor(center - 2.0f * widen));
        float* w = &k.weights[static_cast<size_t>(i * k.taps)];
        float sum = 0.0f;
        for (int t = 0; t < k.taps; ++t) {
            w[t] = catmull_rom((static_cast<float>(first + t) + 0.5f - center) / widen);
            sum += w[t];
        }
        for (int t = 0; t < k.taps; ++t)
            w[t] /= sum;
        k.first[static_cast<size_t>(i)] = first;
    }
    return k;
}

} // namespace

void fade_vertices(ImDrawList* dl, int from, float alpha, float dy)
{
    for (int i = from; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        v.col = draw::with_alpha(v.col, alpha);
        v.pos.y += dy;
    }
}

void apply_imgui_style()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s = ImGuiStyle();
    s.WindowPadding = ImVec2(0.0f, 0.0f);
    s.WindowBorderSize = 0.0f;
    s.WindowRounding = layout::rounding;
    s.PopupRounding = 6.0f;
    s.PopupBorderSize = 1.0f;
    s.FrameRounding = 4.0f;
    s.FrameBorderSize = 0.0f;
    s.GrabRounding = 4.0f;
    s.ItemSpacing = ImVec2(0.0f, 0.0f);
    s.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    s.ScaleAllSizes(ui_scale);
    s.FontScaleDpi = ui_scale;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = to_vec4(col::label);
    c[ImGuiCol_TextDisabled] = to_vec4(col::section_header);
    c[ImGuiCol_WindowBg] = to_vec4(col::main_bg);
    c[ImGuiCol_PopupBg] = to_vec4(col::popup_bg);
    c[ImGuiCol_Border] = to_vec4(col::popup_border);
    c[ImGuiCol_FrameBg] = to_vec4(col::combo_bg);
    c[ImGuiCol_FrameBgHovered] = to_vec4(col::combo_bg_hovered);
    c[ImGuiCol_FrameBgActive] = to_vec4(col::combo_bg_hovered);
    c[ImGuiCol_Button] = to_vec4(col::combo_bg);
    c[ImGuiCol_ButtonHovered] = to_vec4(col::combo_bg_hovered);
    c[ImGuiCol_ButtonActive] = to_vec4(col::popup_item_hover);
    c[ImGuiCol_CheckMark] = to_vec4(col::accent);
    c[ImGuiCol_SliderGrab] = to_vec4(col::accent);
    c[ImGuiCol_SliderGrabActive] = to_vec4(col::toggle_on_knob);
    c[ImGuiCol_Header] = to_vec4(col::popup_item_hover);
    c[ImGuiCol_HeaderHovered] = to_vec4(col::popup_item_hover);
    c[ImGuiCol_HeaderActive] = to_vec4(col::tab_bg_active);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.70f, 0.84f, 1.0f, 0.30f);
    c[ImGuiCol_NavCursor] = to_vec4(col::accent);
}

void Menu::init(const ImVec2& pos, float scale)
{
    initial_pos_ = pos;
    default_scale_ = ImClamp(scale, min_scale, max_scale);
    set_scale(default_scale_);

    // The profile window and the watermark options are menu state, not preset state.
    // Registering this object makes their rows bindable like every other row.
    binds::set_external_state(this, sizeof(Menu));

    const auto configs = make_presets();
    presets_.clear();
    for (int i = 0; i < preset_count; ++i) {
        auto p = std::make_unique<Preset>();
        p->name = i==0?"Default":("Preset "+std::to_string(i+1));
        p->icon = i == 0 ? icon::sun : nullptr;
        p->config = configs[static_cast<size_t>(i)];
        p->defaults = p->config;
        presets_.push_back(std::move(p));
    }

    binds::set_context(nullptr, nullptr, nullptr);

    // Presets saved by earlier sessions replace the built-in list; without a file the
    // shipped presets stay.
    if (load_presets())
        toast("Presets loaded");
    presets_saved_ = config();

    avatar_ = IM_NEW(ImTextureData)();
    avatar_->Create(ImTextureFormat_RGBA32, k_avatar_tex_side, k_avatar_tex_side);
    avatar_->UseColors = true;
    ImGui::RegisterUserTexture(avatar_);
}

void Menu::update_avatar(int side)
{
    constexpr int src_n = assets::avatar_size;
    IM_ASSERT(assets::avatar_rgba.size == static_cast<std::size_t>(src_n * src_n * 4));
    IM_ASSERT(side > 0 && side + 2 * k_avatar_border <= avatar_->Width);
    const float inset = static_cast<float>(src_n) * (k_avatar_image_r - layout::avatar_r) / (2.0f * k_avatar_image_r);
    const Kernel k = make_kernel(inset, static_cast<float>(src_n) - 2.0f * inset, side);
    const auto src_index = [](int i) { return static_cast<size_t>(ImClamp(i, 0, src_n - 1)); };

    // Filtered with premultiplied alpha so transparent pixels don't bleed their color.
    std::vector<float> src(static_cast<size_t>(src_n * src_n * 4));
    for (size_t i = 0; i < static_cast<size_t>(src_n * src_n); ++i) {
        const unsigned char* p = assets::avatar_rgba.data + i * 4;
        const float a = p[3] / 255.0f;
        for (size_t c = 0; c < 3; ++c)
            src[i * 4 + c] = p[c] / 255.0f * a;
        src[i * 4 + 3] = a;
    }

    const size_t n = static_cast<size_t>(side);
    std::vector<float> rows(n * src_n * 4, 0.0f);
    for (size_t y = 0; y < static_cast<size_t>(src_n); ++y)
        for (size_t x = 0; x < n; ++x) {
            float* out = &rows[(y * n + x) * 4];
            for (int t = 0; t < k.taps; ++t) {
                const float w = k.weights[x * k.taps + t];
                const float* in = &src[(y * src_n + src_index(k.first[x] + t)) * 4];
                for (size_t c = 0; c < 4; ++c)
                    out[c] += in[c] * w;
            }
        }

    for (size_t y = 0; y < n; ++y)
        for (size_t x = 0; x < n; ++x) {
            float v[4] = {};
            for (int t = 0; t < k.taps; ++t) {
                const float w = k.weights[y * k.taps + t];
                const float* in = &rows[(src_index(k.first[y] + t) * n + x) * 4];
                for (size_t c = 0; c < 4; ++c)
                    v[c] += in[c] * w;
            }
            const float a = ImSaturate(v[3]);
            auto* out = static_cast<unsigned char*>(
                avatar_->GetPixelsAt(static_cast<int>(x) + k_avatar_border, static_cast<int>(y) + k_avatar_border));
            for (size_t c = 0; c < 3; ++c)
                out[c] = static_cast<unsigned char>(ImSaturate(a > 0.0f ? v[c] / a : 0.0f) * 255.0f + 0.5f);
            out[3] = static_cast<unsigned char>(a * 255.0f + 0.5f);
        }

    const int used = side + 2 * k_avatar_border;
    for (int y = 0; y < used; ++y)
        for (int x = 0; x < used; ++x) {
            const int sx = ImClamp(x, k_avatar_border, side + k_avatar_border - 1);
            const int sy = ImClamp(y, k_avatar_border, side + k_avatar_border - 1);
            if (sx != x || sy != y)
                std::memcpy(avatar_->GetPixelsAt(x, y), avatar_->GetPixelsAt(sx, sy), 4);
        }
    ImTextureDataQueueUpload(avatar_, 0, 0, used, used);
    avatar_side_ = side;
}

void Menu::draw_avatar(ImDrawList* dl, const ImVec2& min, float d, float alpha) const
{
    if (!avatar_ || avatar_side_ <= 0)
        return;
    const float texel = 1.0f / static_cast<float>(avatar_->Width);
    const float uv0 = static_cast<float>(k_avatar_border) * texel, uv1 = uv0 + static_cast<float>(avatar_side_) * texel;
    dl->AddImageRounded(avatar_->GetTexRef(), min, ImVec2(min.x + d, min.y + d), ImVec2(uv0, uv0), ImVec2(uv1, uv1),
                        draw::with_alpha(IM_COL32_WHITE, alpha), d * 0.5f);
}

void Menu::shutdown()
{
    images::shutdown();
    if (!avatar_)
        return;
    ImGui::UnregisterUserTexture(avatar_);
    IM_DELETE(avatar_);
    avatar_ = nullptr;
}

void Menu::toggle_visible() { visible_ = !visible_; if (!visible_) save_presets(); }

void Menu::set_visible(bool visible)
{
    visible_ = visible;
    if (!visible)
        visibility_ = 0.0f;
    else if (visibility_ <= 0.0f)
        open_speed_ = 3.2f; // the first opening after the load is a slower, longer fade
}

void Menu::set_scale(float scale)
{
    scale = ImClamp(scale, min_scale, max_scale);
    if (placed_ && scale != ui_scale)
        recenter(scale, 1.0f);
    ui_scale = scale;
    apply_imgui_style();
}

float Menu::scale() const { return ui_scale; }

void Menu::rescale(float factor)
{
    default_scale_ = ImClamp(default_scale_ * factor, min_scale, max_scale);
    const float scale = ImClamp(ui_scale * factor, min_scale, max_scale);
    if (placed_)
        recenter(scale, factor);
    ui_scale = scale;
    apply_imgui_style();
}

void Menu::recenter(float scale, float factor)
{
    const ImVec2 old_size = menu_size();
    const ImVec2 size = menu_size(scale);
    pos_.x = std::floor((pos_.x + old_size.x * 0.5f) * factor - size.x * 0.5f);
    pos_.y = std::floor((pos_.y + old_size.y * 0.5f) * factor - size.y * 0.5f);
    reposition_ = true;
}

void Menu::handle_zoom()
{
    const ImGuiIO& io = ImGui::GetIO();
    // Touchpads report fractions of a wheel notch; collect them into whole steps.
    zoom_wheel_ = io.KeyCtrl ? zoom_wheel_ + io.MouseWheel : 0.0f;
    int steps = static_cast<int>(zoom_wheel_);
    zoom_wheel_ -= static_cast<float>(steps);
    if (chord_pressed(ImGuiKey_Equal, ImGuiKey_KeypadAdd))
        ++steps;
    if (chord_pressed(ImGuiKey_Minus, ImGuiKey_KeypadSubtract))
        --steps;

    if (chord_pressed(ImGuiKey_0, ImGuiKey_Keypad0))
        set_scale(default_scale_);
    else if (steps != 0)
        set_scale(std::round(ui_scale / k_scale_step + static_cast<float>(steps)) * k_scale_step);
}

void Menu::select_preset(int preset)
{
    preset = ImClamp(preset, 0, static_cast<int>(presets_.size()) - 1);
    if (preset == preset_)
        return;
    binds::release_all();
    preset_ = preset;
    Preset& p = *presets_[static_cast<size_t>(preset_)];
    binds::set_context(&p.config, &p.defaults, &p.binds);
}

void Menu::toast(const char* text)
{
    toast_text_ = text;
    toast_ = 1.0f;
}

void Menu::render()
{
    l10n::russian = host_content != nullptr && l10n::host_language == 1;
    if(host_content)visuals_subtab_=0;
    const float dt = ImGui::GetIO().DeltaTime;
    col::accent = ImGui::ColorConvertFloat4ToU32(ImVec4(style_color_.x, style_color_.y, style_color_.z, 1.0f));

    // Binds and overlays work while the menu is hidden. The overlays are created first so they
    // stay behind the menu.
    Preset& preset = *presets_[static_cast<size_t>(preset_)];
    Config& config = preset.config;
    binds::set_context(&config, &preset.defaults, &preset.binds);
    binds::update();
    // Keeps the preset list on disk: structural changes immediately, in-place edits
    // every few seconds.
    flush_presets(dt);
    fps_timer_ -= dt;
    if (fps_timer_ <= 0.0f) {
        fps_ = ImGui::GetIO().Framerate;
        fps_timer_ = 0.5f;
    }
    update_stats();
    if (!host_content&&config.visuals.world.window_watermark)
        draw_watermark();
    binds::draw_hotkeys(config.visuals.world.window_keybinds && (!host_content || !preset.binds.empty()), visible_);
    if (pending_tab_ >= 0) {
        if (tab_ != pending_tab_ || (pending_tab_ == 2 && visuals_subtab_ != pending_subtab_))
            tab_switch_ = 0.0f;
        tab_ = pending_tab_;
        if (pending_tab_ == 2)
            visuals_subtab_ = pending_subtab_;
        pending_tab_ = -1;
    }
    toast_ = ImMax(0.0f, toast_ - dt * 0.6f);

    if (!visible_ && visibility_ == 1.0f)
        ImGui::ClosePopupsExceptModals();
    visibility_ = ImClamp(visibility_ + (visible_ ? dt : -dt) * open_speed_, 0.0f, 1.0f);
    if (visibility_ >= 1.0f)
        open_speed_ = 7.0f;
    if (visibility_ <= 0.0f)
        return;
    if (visible_)
        handle_zoom();
    if (pending_scale_ > 0.0f) {
        set_scale(pending_scale_);
        pending_scale_ = 0.0f;
    }
    images::new_frame();

    const ImVec2 size = menu_size();
    if (reposition_) {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        pos_.x = ImClamp(pos_.x, 0.0f, ImMax(0.0f, std::floor(display.x - size.x)));
        pos_.y = ImClamp(pos_.y, 0.0f, ImMax(0.0f, std::floor(display.y - size.y)));
        ImGui::SetNextWindowPos(pos_);
        reposition_ = false;
    } else {
        ImGui::SetNextWindowPos(initial_pos_, ImGuiCond_Once);
    }
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, px(layout::rounding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoNav;
    ImGui::Begin("##neverlose", nullptr, flags);
    ImGui::PopStyleVar(4);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const ImVec2 o(std::floor(window->Pos.x), std::floor(window->Pos.y));
    pos_ = o;
    placed_ = true;
    const ImVec2 max(o.x + size.x, o.y + size.y);
    const int vtx_begin = dl->VtxBuffer.Size;

    dl->PushClipRectFullScreen();

    dl->PopClipRect();


    const ImVec2 main_min = at(o, layout::main_x, 0.0f);
    const ImVec2 side_max(main_min.x + px(layout::main_rounding), max.y);
    const float side_r = px(layout::rounding);
    const float panel_a = blur::available() ? 0.52f : 0.68f;
    blur::capture(dl);
    dl->PushClipRectFullScreen();
    draw::soft_shadow(dl, o, max, side_r, px(layout::shadow_size), col::shadow);
    dl->PopClipRect();
    blur::image(dl, o, side_max, side_r, ImDrawFlags_RoundCornersLeft);
    draw::rect_gradient_v(dl, o, side_max, draw::with_alpha(rgb(18, 24, 38), panel_a), draw::with_alpha(rgb(12, 16, 26), panel_a + 0.06f), side_r, ImDrawFlags_RoundCornersLeft);
    draw::rect_gradient_v(dl, o, side_max, rgb(215, 228, 255, 28), rgb(160, 180, 220, 12), side_r, ImDrawFlags_RoundCornersLeft);
    draw::border(dl, o, side_max, rgb(255, 255, 255, 28), side_r, draw::hairline());

    dl->AddRectFilled(main_min, max, col::main_bg, px(layout::main_rounding));
    draw::border(dl, main_min, max, col::main_border, px(layout::main_rounding), draw::hairline());

    draw_sidebar(dl, o);
    // The skin picker covers the header, so the header must not react underneath it.
    // The Ambience view blurs the header too, so it is just as inert.
    const bool picker = (picker_slot_ >= 0 && tab_ == 3) || (!host_content && tab_ == 2 && visuals_subtab_ == 1 && world_view_ == 1);
    const bool searching = search_open_ || search_anim_ > 0.0f;
    if (picker || searching)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    draw_header(dl, o);
    if (picker && !searching)
        ImGui::PopItemFlag();
    draw_content(o);
    if (searching)
        ImGui::PopItemFlag();
    draw_search(o);
    draw_profile_popup(o);

    if (toast_ > 0.0f && !toast_text_.empty()) {
        // Notification sliding up from the bottom edge of the menu.
        const Fonts& f = fonts();
        const float a = ImSaturate(toast_ * 4.0f);
        const float in = ImSaturate((1.0f - toast_) * 8.0f);
        const float e = 1.0f - (1.0f - in) * (1.0f - in);
        const float tw = draw::text_width(f.medium, font_size::text, toast_text_.c_str());
        const float w = snap(tw + px(34.0f)), h = snap(px(26.0f));
        const float cx = o.x + px((layout::main_x + layout::width) * 0.5f);
        const ImVec2 tmin(snap(cx - w * 0.5f), snap(max.y - px(12.0f) - h + (1.0f - e) * px(14.0f)));
        const ImVec2 tmax(tmin.x + w, tmin.y + h);
        const int t0 = dl->VtxBuffer.Size;
        ui::glass(dl, tmin, tmax, px(6.0f), col::window_bg, true);
        draw::border(dl, tmin, tmax, col::window_border, px(6.0f), draw::hairline());
        draw::icon(dl, f.icons, 10.0f, ImVec2(tmin.x + px(13.0f), (tmin.y + tmax.y) * 0.5f), col::accent, icon::check);
        draw::text(dl, f.medium, font_size::text,
                   ImVec2(tmin.x + px(24.0f), (tmin.y + tmax.y) * 0.5f - draw::cap_height(f.medium, font_size::text) * 0.5f),
                   col::window_label, toast_text_.c_str());
        fade_vertices(dl, t0, a * e);
    }

    if (visibility_ < 1.0f) {
        const float e = visibility_ * (2.0f - visibility_);
        fade_vertices(dl, vtx_begin, e, (1.0f - e) * px(8.0f));
    }
    ImGui::End();
}

void Menu::draw_sidebar(ImDrawList* dl, const ImVec2& o)
{
    const Fonts& f = fonts();

    // Logo and product name
    dl->AddRectFilled(at(o, layout::logo_x, layout::logo_y),
                      at(o, layout::logo_x + layout::logo_w, layout::logo_y + layout::logo_h), col::logo_bg,
                      px(layout::logo_rounding));
    const float nl_w = draw::text_width(f.black, font_size::logo, "P");
    draw::text_glow(dl, f.black, font_size::logo,
                    ImVec2(o.x + px(layout::logo_text_cx) - nl_w * 0.5f, o.y + px(layout::logo_text_cap_y)),
                    col::logo_text, col::logo_glow, px(layout::logo_glow_radius), "P");
    draw::text(dl, f.semibold, font_size::title, at(o, layout::title_x, layout::title_cap_y), col::title, "Pyzirik");
    draw::text(dl, f.medium, font_size::subtitle, at(o, layout::subtitle_x, layout::subtitle_cap_y), col::subtitle,
               "Standoff 2");

    const float sep_x0 = o.x + px(layout::sidebar_sep_x0), sep_x1 = o.x + px(layout::sidebar_sep_x1);
    draw::hline(dl, sep_x0, sep_x1, o.y + px(layout::sidebar_sep_top_y), col::sidebar_separator);
    draw::hline(dl, sep_x0, sep_x1, o.y + px(layout::sidebar_sep_bottom_y), col::sidebar_separator);

    draw::text(dl, f.medium, font_size::sidebar_header, at(o, layout::sidebar_header_x, layout::aimbot_header_cap_y),
               col::sidebar_header, "AIMBOT");
    const float common_shift = host_content ? layout::host_common_shift : 0.0f;
    draw::text(dl, f.medium, font_size::sidebar_header, at(o, layout::sidebar_header_x, layout::common_header_cap_y-common_shift),
               col::sidebar_header, "COMMON");

    // The host groups Misc under Visuals while the donor layout keeps its original tabs.
    const float dt = ImGui::GetIO().DeltaTime;
    const float open_target = tab_ == 2 ? 1.0f : 0.0f;
    if (visuals_open_ < 0.0f)
        visuals_open_ = open_target;
    visuals_open_ = ImLerp(visuals_open_, open_target, 1.0f - std::exp(-18.0f * dt));
    if (std::fabs(visuals_open_ - open_target) < 0.002f)
        visuals_open_ = open_target;
    const float unfold = visuals_open_ * (2.0f - visuals_open_);
    const float tab_pitch = layout::tab_top_y[4] - layout::tab_top_y[3];

    struct Entry {
        int tab, sub;
        float y, indent, alpha;
        const char* name;
        const char* icon;
    };
    Entry entries[8];
    int n = 0;
    const int subtab_count = host_content ? 1 : 2;
    for (int i = host_content ? 1 : 0; i < k_tab_count; ++i) {
        const float push = i > 2 ? unfold * static_cast<float>(subtab_count) * tab_pitch : 0.0f;
        // The host hides Misc here, so Aim takes its first row instead of
        // leaving the donor's hidden row as an empty gap below the heading.
        const float tab_y = host_content && i == 1 ? layout::tab_top_y[0] : layout::tab_top_y[i]-(i>=2?common_shift:0.f);
        entries[n++] = { i, -1, tab_y + push, 0.0f, 1.0f, k_tab_names[i], k_tab_icons[i] };
        if (i == 2 && unfold > 0.0f)
            for (int sub = 0; sub < subtab_count; ++sub) {
                const float alpha = sub == 2 ? ImSaturate(unfold * 2.2f - 1.2f)
                                             : ImSaturate(unfold * 1.6f - 0.6f * static_cast<float>(sub));
                entries[n++] = { 2, sub, layout::tab_top_y[2]-common_shift + tab_pitch * (1.0f + sub) * unfold, layout::subtab_indent,
                                 alpha, k_subtab_names[sub],
                                 k_subtab_icons[sub] };
            }
    }

    // One highlight slides between the tabs instead of each tab fading its own.
    {
        float target_y = -1.0f, target_indent = 0.0f;
        for (int e = 0; e < n; ++e) {
            const Entry& t = entries[e];
            const bool selected = t.sub >= 0 ? (tab_ == 2 && visuals_subtab_ == t.sub) : (tab_ == t.tab && t.tab != 2);
            if (selected) {
                target_y = t.y;
                target_indent = t.indent;
            }
        }
        if (target_y >= 0.0f) {
            if (tab_marker_y_ < 0.0f)
                tab_marker_y_ = target_y;
            tab_marker_y_ = ImLerp(tab_marker_y_, target_y, 1.0f - std::exp(-16.0f * dt));
            if (std::fabs(tab_marker_y_ - target_y) < 0.01f)
                tab_marker_y_ = target_y;
            const float indent = ui::animate(ImGui::GetID("##marker_indent"), target_indent, 16.0f);
            // Stretches a little while moving, like it is being pulled.
            const float stretch = ImMin(std::fabs(target_y - tab_marker_y_) * 0.25f, 6.0f);
            const float y0 = target_y < tab_marker_y_ ? tab_marker_y_ - stretch : tab_marker_y_;
            const float y1 = target_y > tab_marker_y_ ? tab_marker_y_ + layout::tab_h + stretch : tab_marker_y_ + layout::tab_h;
            const ImVec2 mmin = at(o, layout::tab_x0 + indent, y0), mmax = at(o, layout::tab_x1, y1);
            dl->AddRectFilled(mmin, mmax, rgb(210, 225, 255, 36), px(layout::tab_rounding));
            draw::border(dl, mmin, mmax, rgb(255, 255, 255, 30), px(layout::tab_rounding), draw::hairline());
            // Accent bar removed per user request
            // const float bar_h = (mmax.y - mmin.y) * 0.46f, mcy = (mmin.y + mmax.y) * 0.5f;
            // dl->AddRectFilled(ImVec2(mmin.x, mcy - bar_h * 0.5f), ImVec2(mmin.x + px(2.0f), mcy + bar_h * 0.5f),
            //                   col::accent, px(1.0f));
        }
    }

    for (int e = 0; e < n; ++e) {
        const Entry& t = entries[e];
        const bool sub = t.sub >= 0;
        const bool selected = sub ? (tab_ == 2 && visuals_subtab_ == t.sub) : (tab_ == t.tab && t.tab != 2);
        const ImVec2 min = at(o, layout::tab_x0 + t.indent, t.y);
        const ImVec2 max = at(o, layout::tab_x1, t.y + layout::tab_h);
        ImGui::PushID(t.tab * 4 + t.sub + 1);
        const ImGuiID id = ImGui::GetID("##tab");
        const ui::ItemState s = ui::item_behavior(id, min, max, t.alpha > 0.5f);
        if (s.pressed) {
            // Clicking the entry of the section that is already open returns from the
            // Ambience view to the World panels.
            if (tab_ == t.tab && (!sub || visuals_subtab_ == t.sub)) {
                if (t.tab == 2 && t.sub == 1)
                    world_view_ = 0;
            } else {
                tab_ = t.tab;
                if (sub)
                    visuals_subtab_ = t.sub;
                tab_switch_ = 0.0f;
                if (t.tab != 2 || t.sub != 1)
                    world_view_ = 0;
            }
        }
        const float a = ui::animate(id + 1, selected ? 1.0f : 0.0f, 20.0f) * t.alpha;
        const float h = ui::animate(id + 2, s.hovered ? 1.0f : 0.0f, 16.0f) * (1.0f - a) * t.alpha;
        ImGui::PopID();

        if (h > 0.0f)
            dl->AddRectFilled(min, max, draw::with_alpha(col::tab_bg_hovered, h), px(layout::tab_rounding));

        const float cy = (min.y + max.y) * 0.5f;
        const ImU32 icon_col = draw::with_alpha(
            draw::lerp_color(draw::lerp_color(col::tab_icon, col::tab_text_hovered, h), col::tab_icon_active, a), t.alpha);
        const ImU32 text_col = draw::with_alpha(
            draw::lerp_color(draw::lerp_color(col::tab_text, col::tab_text_hovered, h), col::tab_text_active, a), t.alpha);
        if (t.icon)
            draw::icon(dl, f.icons, font_size::icon,
                       ImVec2(o.x + px(layout::tab_icon_cx + t.indent), cy + px(layout::tab_icon_dy)), icon_col, t.icon);
        else
            draw::icon_swap_square(dl, ImVec2(o.x + px(layout::misc_icon_cx), cy), icon_col,
                                   draw::lerp_color(col::sidebar_icon_hole, col::tab_icon_cutout, a));

        draw::text(dl, f.medium, font_size::tab,
                   ImVec2(snap(o.x + px(layout::tab_text_x + t.indent + 2.0f * a + 1.0f * h)),
                          text_cap_y_centered(f.medium, font_size::tab, cy) + px(layout::tab_text_dy)),
                   text_col, t.name);
    }

    // Profile
    const ImVec2 profile_min = at(o, layout::tab_x0, layout::sidebar_sep_bottom_y + 6.0f);
    const ImVec2 profile_max = at(o, layout::tab_x1, layout::height - 4.0f);
    const ui::ItemState profile = ui::item_behavior(ImGui::GetID("##profile"), profile_min, profile_max);
    if (profile.pressed)
        ImGui::OpenPopup("##profile_popup");
    const bool profile_open = ImGui::IsPopupOpen("##profile_popup");
    const float ph =
        ui::animate(ImGui::GetID("##profile_h"), (profile.hovered || profile_open) ? 1.0f : 0.0f, 16.0f);
    if (ph > 0.0f)
        dl->AddRectFilled(profile_min, profile_max, draw::with_alpha(col::tab_bg_hovered, ph), px(layout::tab_rounding));

    if (avatar_) {
        const int side = avatar_side();
        if (side != avatar_side_)
            update_avatar(side);
        const float d = static_cast<float>(side);
        const ImVec2 min(snap(o.x + px(layout::avatar_cx) - d * 0.5f), snap(o.y + px(layout::avatar_cy) - d * 0.5f));
        const float texel = 1.0f / static_cast<float>(avatar_->Width);
        const float uv0 = static_cast<float>(k_avatar_border) * texel, uv1 = uv0 + d * texel;
        dl->AddImageRounded(avatar_->GetTexRef(), min, ImVec2(min.x + d, min.y + d), ImVec2(uv0, uv0), ImVec2(uv1, uv1),
                            IM_COL32_WHITE, d * 0.5f);
    }
    draw::text(dl, f.medium, font_size::tab, at(o, layout::username_x, layout::username_cap_y), col::username, k_username);
    draw::text(dl, f.medium, font_size::small, at(o, layout::days_x, layout::days_cap_y), col::subscription,
               "internal");
    draw::chevron_right(dl, ImVec2(o.x + px(layout::profile_chevron_cx), o.y + px(layout::profile_chevron_cy)),
                        px(1.8f), px(3.6f), draw::lerp_color(col::profile_chevron, col::tab_text_active, ph), px(1.6f));
}

void Menu::draw_profile_popup(const ImVec2& o)
{
    const char* id = "##profile_popup";
    const float profile_cy = o.y + px(layout::avatar_cy);
    const float h = ui::popup::last_height(id);
    ui::popup::place(id, ImVec2(at(o, layout::tab_x1 + 8.0f, 0.0f).x, std::floor(profile_cy - h * 0.5f)));
    if (!ui::popup::begin(id, nullptr, 190.0f))
        return;

    const Fonts& f = fonts();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float header_h = snap(px(54.0f));
    ImGui::ItemSize(ImVec2(ImGui::GetContentRegionAvail().x, header_h));
    if (avatar_ && avatar_side_ > 0) {
        const float d = static_cast<float>(avatar_side_);
        const ImVec2 min(snap(p.x + px(4.0f)), snap(p.y + px(6.0f)));
        const float texel = 1.0f / static_cast<float>(avatar_->Width);
        const float uv0 = static_cast<float>(k_avatar_border) * texel, uv1 = uv0 + d * texel;
        dl->AddImageRounded(avatar_->GetTexRef(), min, ImVec2(min.x + d, min.y + d), ImVec2(uv0, uv0), ImVec2(uv1, uv1),
                            IM_COL32_WHITE, d * 0.5f);
    }
    const float tx = p.x + px(42.0f);
    draw::text(dl, f.semibold, font_size::window_title, ImVec2(tx, p.y + px(6.0f)), col::window_title, k_username);
    draw::text(dl, f.medium, font_size::popup_label, ImVec2(tx, p.y + px(22.0f)), col::window_label,
               "Till: January 26 2027");
    const ImVec2 renew_pen(tx, p.y + px(38.0f));
    const float renew_w = draw::text_width(f.medium, font_size::popup_label, "Renew");
    const ui::ItemState renew = ui::item_behavior(ImGui::GetID("##renew"), ImVec2(renew_pen.x, renew_pen.y - px(3.0f)),
                                                  ImVec2(renew_pen.x + renew_w, renew_pen.y + px(12.0f)));
    draw::text(dl, f.medium, font_size::popup_label, renew_pen,
               renew.hovered ? col::accent : col::window_value, "Renew");

    int scale_choice = 0;
    if (std::fabs(ui_scale - default_scale_) > 0.001f)
        for (int i = 1; i < IM_ARRAYSIZE(k_scale_values); ++i)
            if (std::fabs(ui_scale - k_scale_values[i]) < 0.001f)
                scale_choice = i;
    ui::popup::group([&] {
        if(host_content){const char* languages[]={"English","Русский"};ui::popup::value("Language",&l10n::host_language,languages,true,icon::globe);}
        else ui::popup::value("Language", &language_, k_languages, true, icon::globe);
        if (ui::popup::value("Menu Scale", &scale_choice, k_scales, true, icon::scale))
            pending_scale_ = scale_choice == 0 ? default_scale_ : k_scale_values[scale_choice];
        ui::popup::value("ESP Scale", &esp_scale_, k_scales, true, icon::text_height);
        ui::popup::value("Windows Scale", &windows_scale_, k_scales, true, icon::windows);
        ui::popup::value("Units", &units_, k_units, true, icon::ruler);
        ui::popup::color("Style", &style_color_, true, icon::fill_drip, icon::gear);
    });
    ui::popup::group([&] {
        ui::popup::value("Safe Mode", &safe_mode_, k_safe_modes, true, icon::shield);
        ui::popup::toggle("Synchronization", &sync_, icon::display);
    });
    ui::popup::group([&] {
        ui::popup::item("About", icon::copyright);
        ui::popup::item("Chat", icon::comments);
    });
    ui::popup::end();
}

void Menu::draw_preset_group(ImDrawList* dl, const ImVec2& o, float& right)
{
    const Fonts& f = fonts();
    const float dt = ImGui::GetIO().DeltaTime;
    const Preset& current = *presets_[static_cast<size_t>(preset_)];
    if (current.name != shown_preset_) {
        previous_preset_ = shown_preset_;
        shown_preset_ = current.name;
        preset_change_ = previous_preset_.empty() ? 1.0f : 0.0f;
    }
    preset_change_ = ImMin(1.0f, preset_change_ + dt * 6.0f);
    const char* preset = shown_preset_.c_str();
    const char* preset_icon = current.icon;

    // The group grows with the preset name up to a maximum width; longer names fade out.
    const float icon_t = ui::animate(ImGui::GetID("##preset_icon"), preset_icon ? 1.0f : 0.0f, 16.0f);
    const float text_x = layout::preset_text_x + layout::preset_icon_text_dx * icon_t;
    const float text_w_target = draw::text_width(f.medium, font_size::text, preset);
    const float text_w = ui::animate(ImGui::GetID("##preset_w"), text_w_target, 18.0f);
    float chevron_x = o.x + px(text_x + layout::preset_chevron_gap) + text_w;
    const float max_right = o.x + px(layout::preset_group_max_right);
    const bool clipped = chevron_x + px(layout::preset_right_pad) > max_right;
    if (clipped)
        chevron_x = max_right - px(layout::preset_right_pad);
    const ImVec2 gmin = at(o, layout::group_x, layout::group_y);
    const ImVec2 gmax(snap(chevron_x + px(layout::preset_right_pad)), at(o, 0.0f, layout::group_y + layout::group_h).y);
    right = gmax.x;
    const float gcy = (gmin.y + gmax.y) * 0.5f;
    const float group_rounding = px(layout::group_rounding);
    dl->AddRectFilled(gmin, gmax, col::group_bg, group_rounding);

    // Save button: keeps the current values as the preset's defaults.
    const ImVec2 save_max(at(o, layout::group_sep_x, 0.0f).x, gmax.y);
    const ImGuiID save_id = ImGui::GetID("##save");
    const ui::ItemState save = ui::item_behavior(save_id, gmin, save_max);
    if (save.pressed) {
        save_flash_ = 1.0f;
        Preset& p = *presets_[static_cast<size_t>(preset_)];
        p.defaults = p.config;
    }
    save_flash_ = ImMax(0.0f, save_flash_ - dt);
    const float sh = ui::animate(save_id + 1, save.hovered ? 1.0f : 0.0f, 16.0f);
    if (sh > 0.0f)
        dl->AddRectFilled(gmin, save_max, draw::with_alpha(col::group_hovered, sh), group_rounding,
                          ImDrawFlags_RoundCornersLeft);
    {
        // For a second after saving the floppy turns into a check mark, then back.
        const float in = ImSaturate((1.0f - save_flash_) * 8.0f), out = ImSaturate(save_flash_ * 6.0f);
        const float check = save_flash_ > 0.0f ? ImMin(in, out) : 0.0f;
        const float pop = 1.0f + 0.25f * std::sin(in * IM_PI) * (save_flash_ > 0.0f ? 1.0f : 0.0f);
        const float press = ui::animate(save_id + 2, save.held ? 1.0f : 0.0f, 24.0f);
        const ImVec2 c(o.x + px(layout::save_icon_cx), o.y + px(layout::save_icon_cy) + px(1.0f) * press);
        const ImU32 base = draw::lerp_color(col::save_icon, col::tab_text_active, sh * 0.3f);
        if (check < 1.0f)
            draw::icon(dl, f.icons, font_size::icon_save * (1.0f - 0.08f * press) * (1.0f - 0.3f * check), c,
                       draw::with_alpha(base, 1.0f - check), icon::floppy);
        if (check > 0.0f)
            draw::icon(dl, f.icons, font_size::icon_save * (0.7f + 0.3f * check) * pop, c,
                       draw::with_alpha(col::accent, check), icon::check);
    }

    // Presets window
    const ImVec2 preset_min(save_max.x, gmin.y);
    const ImGuiID preset_id = ImGui::GetID("##preset");
    const ui::ItemState ps = ui::item_behavior(preset_id, preset_min, gmax);
    const bool open = ImGui::IsPopupOpen(preset_id, ImGuiPopupFlags_None);
    if (ps.pressed && !open)
        ImGui::OpenPopup(preset_id);
    const float hh = ui::animate(preset_id + 1, (ps.hovered || open) ? 1.0f : 0.0f, 16.0f);
    const float oa = ui::animate(preset_id + 2, open ? 1.0f : 0.0f, 16.0f);
    if (hh > 0.0f)
        dl->AddRectFilled(preset_min, gmax, draw::with_alpha(col::group_hovered, hh), group_rounding,
                          ImDrawFlags_RoundCornersRight);
    if (icon_t > 0.01f && preset_icon)
        draw::icon(dl, f.icons, font_size::icon * (0.6f + 0.4f * icon_t),
                   ImVec2(o.x + px(layout::preset_text_x + layout::preset_icon_dx), gcy),
                   draw::with_alpha(col::preset_sun, icon_t), preset_icon);

    // The name crossfades: the old one rises out, the new one comes up from below.
    const float e = 1.0f - (1.0f - preset_change_) * (1.0f - preset_change_);
    const ImU32 text_col = draw::lerp_color(col::preset_text, col::tab_text_active, hh * 0.6f);
    const ImVec2 pen = at(o, text_x, layout::preset_cap_y);
    dl->PushClipRect(ImVec2(preset_min.x, gmin.y + 1.0f), ImVec2(gmax.x, gmax.y - 1.0f), true);
    const auto name = [&](const char* text, float dy, float alpha) {
        const ImVec2 p(pen.x, snap(pen.y + dy));
        const ImU32 c = draw::with_alpha(text_col, alpha);
        if (clipped)
            draw::text_fade_right(dl, f.medium, font_size::text, p, c, text,
                                  chevron_x - px(layout::preset_fade_to + layout::preset_fade_len),
                                  chevron_x - px(layout::preset_fade_to));
        else
            draw::text(dl, f.medium, font_size::text, p, c, text);
    };
    if (preset_change_ < 1.0f && !previous_preset_.empty())
        name(previous_preset_.c_str(), -px(9.0f) * e, 1.0f - e);
    name(preset, px(9.0f) * (1.0f - e), e);
    dl->PopClipRect();

    // The chevron turns over while the window is open.
    const ImU32 chevron_col = draw::lerp_color(col::preset_chevron, col::tab_text_active, hh * 0.6f);
    const ImVec2 chevron_c(chevron_x, gcy + px(layout::preset_chevron_dy));
    const float hw = px(3.0f), hy = px(1.5f) * (1.0f - 2.0f * oa);
    const ImVec2 pts[3] = { { chevron_c.x - hw, chevron_c.y - hy }, { chevron_c.x, chevron_c.y + hy },
                            { chevron_c.x + hw, chevron_c.y - hy } };
    dl->AddPolyline(pts, 3, chevron_col, px(1.4f));

    // Borders go on top of the hover fills.
    const float line = draw::hairline();
    draw::vline(dl, save_max.x - line * 0.5f, gmin.y + line, gmax.y - line, col::group_border);
    draw::border(dl, gmin, gmax, draw::lerp_color(col::group_border, col::accent, oa * 0.35f), group_rounding, line);

    draw_presets_popup(preset_id, preset_min, gmax);
}

float Menu::draw_weapon_group(ImDrawList* dl, const ImVec2& o, float left)
{
    const Fonts& f = fonts();
    RageSettings& r = config().rage;
    r.weapon = ImClamp(r.weapon, 0, rage_weapon_count - 1);
    const char* name = rage_weapons[r.weapon];
    const float text_w = draw::text_width(f.medium, font_size::text, name);
    const float w_target = ImMax(snap(px(layout::weapon_group_w)),
                                 snap(px(layout::weapon_text_x + layout::weapon_chevron_gap + layout::weapon_right_pad) + text_w));
    const ImGuiID id = ImGui::GetID("##weapon");
    const float w = snap(ui::animate(id + 3, w_target, 18.0f));
    const ImVec2 min(snap(left), at(o, 0.0f, layout::group_y).y);
    const ImVec2 max(min.x + w, at(o, 0.0f, layout::group_y + layout::group_h).y);
    const float cy = (min.y + max.y) * 0.5f;
    const float rounding = px(layout::group_rounding);

    const ui::ItemState s = ui::item_behavior(id, min, max);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    if (s.pressed && !open)
        ImGui::OpenPopup(id);
    const float h = ui::animate(id + 1, (s.hovered || open) ? 1.0f : 0.0f, 16.0f);
    const float oa = ui::animate(id + 2, open ? 1.0f : 0.0f, 16.0f);
    dl->AddRectFilled(min, max, draw::lerp_color(col::group_bg, col::group_hovered, h), rounding);
    draw::border(dl, min, max, draw::lerp_color(col::group_border, col::accent, oa * 0.35f), rounding, draw::hairline());
    draw::text(dl, f.medium, font_size::text, ImVec2(snap(min.x + px(layout::weapon_text_x)), o.y + snap(px(layout::preset_cap_y))),
               draw::lerp_color(col::preset_text, col::tab_text_active, h * 0.6f), name);
    const ImVec2 chevron_c(max.x - px(layout::weapon_right_pad), cy + px(layout::preset_chevron_dy));
    const ImU32 chevron_col = draw::lerp_color(col::preset_chevron, col::tab_text_active, h * 0.6f);
    const float hw = px(3.0f), hy = px(1.5f) * (1.0f - 2.0f * oa);
    const ImVec2 pts[3] = { { chevron_c.x - hw, chevron_c.y - hy }, { chevron_c.x, chevron_c.y + hy },
                            { chevron_c.x + hw, chevron_c.y - hy } };
    dl->AddPolyline(pts, 3, chevron_col, px(1.4f));

    if (ui::begin_popup_id(id, min, max, ImMax(w, px(150.0f)))) {
        const int i = ui::dropdown_list(std::span<const char* const>(rage_weapons, rage_weapon_count), r.weapon);
        if (i >= 0) {
            r.weapon = i;
            ImGui::CloseCurrentPopup();
        }
        ui::end_popup();
    }
    return max.x;
}

void Menu::draw_header(ImDrawList* dl, const ImVec2& o)
{
    const Fonts& f = fonts();
    float right = 0.0f;
    draw_preset_group(dl, o, right);
    // The weapon selector of the Rage tab slides in next to the presets.
    const float wa = ui::animate(ImGui::GetID("##weapon_show"), tab_ == 0 ? 1.0f : 0.0f, 14.0f);
    if (!host_content&&wa > 0.01f) {
        const int v0 = dl->VtxBuffer.Size;
        if (tab_ != 0)
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        draw_weapon_group(dl, o, right + px(layout::weapon_group_gap) - (1.0f - wa) * px(10.0f));
        if (tab_ != 0)
            ImGui::PopItemFlag();
        fade_vertices(dl, v0, wa);
    }

    // Search
    const ImVec2 sc(o.x + px(layout::search_cx), o.y + px(layout::search_cy));
    const ImGuiID search_id = ImGui::GetID("##search");
    const float hit = px(10.0f);
    const ui::ItemState ss = ui::item_behavior(search_id, ImVec2(sc.x - hit, sc.y - hit), ImVec2(sc.x + hit, sc.y + hit));
    if (ss.pressed)
        open_search();
    if (!search_open_ && !ImGui::GetIO().WantTextInput && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F))
        open_search();
    const float search_h = ui::animate(search_id + 1, ss.hovered ? 1.0f : 0.0f, 16.0f);
    const float search_a = 1.0f - ImSaturate(search_anim_ * 3.0f);
    if (search_h > 0.0f)
        dl->AddCircleFilled(sc, px(10.0f) * (0.7f + 0.3f * search_h), draw::with_alpha(col::group_hovered, search_h * search_a), 24);
    draw::icon(dl, f.icons, font_size::icon * (1.0f + 0.08f * search_h), sc,
               draw::with_alpha(draw::lerp_color(col::search_icon, col::tab_text_active, search_h), search_a), icon::search);

    draw::hline(dl, o.x + px(layout::header_sep_x0), o.x + px(layout::header_sep_x1), o.y + px(layout::header_sep_y),
                col::header_separator);
}

void Menu::draw_content(const ImVec2& o)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    tab_switch_ = ImMin(1.0f, tab_switch_ + ImGui::GetIO().DeltaTime * 6.0f);
    const int v0 = dl->VtxBuffer.Size;

    Config& c = config();
    ImGui::PushID(tab_);
    if(host_content)host_content(*this,o,tab_,visuals_subtab_);
    else switch (tab_) {
    case 0: tab_rage(o, c); break;
    case 1: tab_legit(o, c); break;
    case 2: tab_visuals(o, c); break;
    case 3: tab_inventory(o, c); break;
    default: tab_misc(o, c); break;
    }
    ImGui::PopID();

    if (tab_switch_ < 1.0f) {
        const float e = tab_switch_ * (2.0f - tab_switch_);
        fade_vertices(dl, v0, e, (1.0f - e) * px(6.0f));
    }
}

} // namespace nl

