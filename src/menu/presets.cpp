// Menu source watermark: @use_sberbank
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
#include <windows.h>
#endif

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/menu.h"
#include "menu/search.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;

constexpr const char* k_export_prefix = "nl1:";
constexpr float k_row_h = 30.0f;
constexpr int k_visible_rows = 7;
constexpr const char* k_sort_names[] = { "Default order", "Name A-Z", "Name Z-A" };

constexpr char k_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::vector<std::uint8_t>& in)
{
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    for (size_t i = 0; i < in.size(); i += 3) {
        const std::uint32_t n = static_cast<std::uint32_t>(in[i]) << 16 |
                                (i + 1 < in.size() ? static_cast<std::uint32_t>(in[i + 1]) << 8 : 0u) |
                                (i + 2 < in.size() ? static_cast<std::uint32_t>(in[i + 2]) : 0u);
        out += k_b64[(n >> 18) & 63];
        out += k_b64[(n >> 12) & 63];
        out += i + 1 < in.size() ? k_b64[(n >> 6) & 63] : '=';
        out += i + 2 < in.size() ? k_b64[n & 63] : '=';
    }
    return out;
}

bool base64_decode(const char* in, std::vector<std::uint8_t>& out)
{
    std::uint32_t acc = 0;
    int bits = 0;
    for (const char* c = in; *c; ++c) {
        if (*c == '=' || *c == '\r' || *c == '\n' || *c == ' ')
            continue;
        const char* p = std::strchr(k_b64, *c);
        if (!p || !*p)
            return false;
        acc = acc << 6 | static_cast<std::uint32_t>(p - k_b64);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<std::uint8_t>((acc >> bits) & 0xFF));
        }
    }
    return true;
}

bool icon_button(ImDrawList* dl, const char* str_id, const ImVec2& c, const char* glyph, float size = 11.0f,
                 bool accent = false)
{
    const ImGuiID id = ImGui::GetID(str_id);
    const float hit = px(10.0f);
    const ui::ItemState s = ui::item_behavior(id, ImVec2(c.x - hit, c.y - hit), ImVec2(c.x + hit, c.y + hit));
    const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 18.0f);
    const float press = ui::animate(id + 2, s.held ? 1.0f : 0.0f, 26.0f);
    if (h > 0.0f)
        dl->AddRectFilled(ImVec2(c.x - hit, c.y - hit), ImVec2(c.x + hit, c.y + hit),
                          draw::with_alpha(col::window_chip_bg, h), px(5.0f));
    const ImU32 base = accent ? col::accent : col::window_value;
    draw::any_icon(dl, c, draw::lerp_color(base, col::tab_text_active, h), glyph, size * (1.0f - 0.1f * press));
    return s.pressed;
}

} // namespace

namespace {

// %APPDATA%\neverlose\presets.bin -- user scoped and stable across restarts, unlike the
// runtime area the cheat uses for its own derived cache.
std::filesystem::path presets_file()
{
#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
    wchar_t buffer[MAX_PATH]{};
    const auto length = ::GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
    if (!length || length >= MAX_PATH)
        return {};
    return std::filesystem::path(buffer) / L"камшако" / L"presets.bin";
#else
    // Android's host sets this to Context.getFilesDir(), which needs no storage permission.
    const char* directory = std::getenv("NL_DATA_DIR");
    if (!directory || !*directory)
        directory = std::getenv("HOME");
    return directory && *directory
        ? std::filesystem::path(directory) / "камшако" / "presets.bin"
        : std::filesystem::path{};
#endif
}

constexpr char k_presets_magic[4] = { 'N', 'L', 'P', '1' };
constexpr std::uint32_t k_presets_version = 2;

void append_u16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
}

bool read_u16(const std::vector<std::uint8_t>& in, size_t& at, std::uint16_t& value)
{
    if (at + 2 > in.size())
        return false;
    value = static_cast<std::uint16_t>(in[at] | in[at + 1] << 8);
    at += 2;
    return true;
}

bool read_u32(const std::vector<std::uint8_t>& in, size_t& at, std::uint32_t& value)
{
    if (at + 4 > in.size())
        return false;
    value = static_cast<std::uint32_t>(in[at]) | static_cast<std::uint32_t>(in[at + 1]) << 8 |
            static_cast<std::uint32_t>(in[at + 2]) << 16 | static_cast<std::uint32_t>(in[at + 3]) << 24;
    at += 4;
    return true;
}

} // namespace

bool Menu::load_presets()
{
    const auto path = presets_file();
    if (path.empty())
        return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(stream)),
                                         std::istreambuf_iterator<char>());
    if (data.size() < 12 || std::memcmp(data.data(), k_presets_magic, 4) != 0)
        return false;
    size_t at = 4;
    std::uint32_t version = 0, count = 0;
    if (!read_u32(data, at, version) || version != k_presets_version)
        return false;
    if (!read_u32(data, at, count) || count == 0 || count > 256)
        return false;

    std::uint32_t selected=0;if(!read_u32(data,at,selected)||selected>=count)return false;
    std::vector<std::unique_ptr<Preset>> loaded;
    for (std::uint32_t index = 0; index < count; ++index) {
        std::uint16_t name_length = 0;
        std::uint32_t size = 0;
        if (!read_u16(data, at, name_length) || at + name_length > data.size())
            return false;
        const std::string name(reinterpret_cast<const char*>(data.data() + at), name_length);
        at += name_length;
        if (!read_u32(data, at, size) || at + size > data.size())
            return false;
        auto preset = std::make_unique<Preset>();
        preset->name = name.empty() ? "Preset" : name;
        if(!decodeHostConfig(preset->config,data.data()+at,size))return false;
        init_ambience_maps(preset->config.visuals.world);
        preset->defaults = preset->config;
        at += size;
        loaded.push_back(std::move(preset));
    }
    if (loaded.empty())
        return false;

    binds::release_all();
    presets_ = std::move(loaded);
    preset_ = static_cast<int>(selected);
    Preset& first = *presets_[preset_];
    binds::set_context(&first.config, &first.defaults, &first.binds);
    presets_saved_ = first.config;
    return true;
}

bool Menu::save_presets() const
{
    const auto path = presets_file();
    if (path.empty())
        return false;

    std::vector<std::uint8_t> data;
    data.insert(data.end(), k_presets_magic, k_presets_magic + 4);
    append_u32(data, k_presets_version);
    append_u32(data, static_cast<std::uint32_t>(presets_.size()));
    append_u32(data,static_cast<std::uint32_t>(preset_));
    for (const auto& preset : presets_) {
        const auto name_length = static_cast<std::uint16_t>(
            ImMin<size_t>(preset->name.size(), 0xFFFF));
        append_u16(data, name_length);
        data.insert(data.end(), preset->name.begin(), preset->name.begin() + name_length);
        append_u32(data, static_cast<std::uint32_t>(sizeof(Config)));
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(&preset->config);
        data.insert(data.end(), bytes, bytes + sizeof(Config));
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return false;
    auto pending = path;
    pending += ".tmp";
    {
        std::ofstream stream(pending, std::ios::binary | std::ios::trunc);
        if (!stream)
            return false;
        stream.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
        if (!stream)
            return false;
    }
    // Atomic hand-off so a crash mid-write cannot leave a truncated list behind.
#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
    if (!::MoveFileExW(pending.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
#else
    std::filesystem::rename(pending, path, error);
    if (error) {
#endif
        std::filesystem::remove(pending, error);
        return false;
    }
    return true;
}

void Menu::flush_presets(float dt)
{
    presets_flush_ += dt;
    if (presets_dirty_) {
        presets_dirty_ = false;
        presets_flush_ = 0.0f;
        save_presets();
        presets_saved_ = config();
        return;
    }
    // In-place edits (a slider, a toggle) are picked up by comparing the active preset
    // every few seconds instead of serializing the whole list per frame.
    if (presets_flush_ < 0.5f)
        return;
    presets_flush_ = 0.0f;
    const Config& active = config();
    if (std::memcmp(&active, &presets_saved_, sizeof(Config)) == 0)
        return;
    save_presets();
    presets_saved_ = active;
}

void Menu::add_preset(const std::string& name, const Config& config, bool rename)
{
    auto p = std::make_unique<Preset>();
    p->name = name;
    p->config = config;
    p->defaults = config;
    presets_.push_back(std::move(p));
    if (rename) {
        rename_ = static_cast<int>(presets_.size()) - 1;
        ImStrncpy(rename_buf_, name.c_str(), sizeof(rename_buf_));
        rename_focus_ = true;
    }
    preset_scroll_target_ = 1e6f; // scrolled to the new row, clamped when drawn
    touch_presets();
}

void Menu::delete_preset(int index)
{
    if (presets_.size() <= 1 || index < 0 || index >= static_cast<int>(presets_.size()))
        return;
    const bool current = index == preset_;
    if (current)
        binds::release_all();
    presets_.erase(presets_.begin() + index);
    if (index < preset_ || preset_ >= static_cast<int>(presets_.size()))
        preset_ = ImMax(0, preset_ - 1);
    Preset& p = *presets_[static_cast<size_t>(preset_)];
    binds::set_context(&p.config, &p.defaults, &p.binds);
    rename_ = -1;
    touch_presets();
}

bool Menu::export_preset(int index) const
{
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return false;
    const Preset& p = *presets_[static_cast<size_t>(index)];
    std::vector<std::uint8_t> data;
    const std::uint16_t name_len = static_cast<std::uint16_t>(ImMin<size_t>(p.name.size(), 255));
    data.push_back(static_cast<std::uint8_t>(name_len & 0xFF));
    data.push_back(static_cast<std::uint8_t>(name_len >> 8));
    data.insert(data.end(), p.name.begin(), p.name.begin() + name_len);
    const std::uint32_t size = sizeof(Config);
    for (int i = 0; i < 4; ++i)
        data.push_back(static_cast<std::uint8_t>(size >> (8 * i)));
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&p.config);
    data.insert(data.end(), bytes, bytes + sizeof(Config));
    ImGui::SetClipboardText((std::string(k_export_prefix) + base64_encode(data)).c_str());
    return true;
}

bool Menu::import_preset()
{
    const char* text = ImGui::GetClipboardText();
    if (!text)
        return false;
    while (*text == ' ' || *text == '\n' || *text == '\r' || *text == '\t')
        ++text;
    if (std::strncmp(text, k_export_prefix, std::strlen(k_export_prefix)) != 0)
        return false;
    std::vector<std::uint8_t> data;
    if (!base64_decode(text + std::strlen(k_export_prefix), data) || data.size() < 6)
        return false;
    const size_t name_len = data[0] | static_cast<size_t>(data[1]) << 8;
    if (data.size() < 2 + name_len + 4)
        return false;
    std::string name(data.begin() + 2, data.begin() + 2 + static_cast<std::ptrdiff_t>(name_len));
    const size_t at = 2 + name_len;
    const std::uint32_t size = data[at] | data[at + 1] << 8 | data[at + 2] << 16 | static_cast<std::uint32_t>(data[at + 3]) << 24;
    if (data.size() < at + 4 + size)
        return false;
    // Start from a valid config so every field is initialized, then copy the bytes over it.
    Config config = presets_[static_cast<size_t>(preset_)]->config;
    if(!decodeHostConfig(config,data.data()+at+4,size))return false;
    add_preset(name.empty() ? "Imported" : name, config, false);
    touch_presets();
    return true;
}

void Menu::draw_presets_popup(ImGuiID id, const ImVec2& anchor_min, const ImVec2& anchor_max)
{
    // A deleted row folds up first and is erased once it is gone.
    if (deleting_) {
        delete_anim_ = ImMax(0.0f, delete_anim_ - ImGui::GetIO().DeltaTime * 7.0f);
        if (delete_anim_ <= 0.0f) {
            for (size_t i = 0; i < presets_.size(); ++i)
                if (presets_[i].get() == deleting_) {
                    delete_preset(static_cast<int>(i));
                    break;
                }
            deleting_ = nullptr;
        }
    }

    const float width = px(262.0f);
    if (!ui::begin_popup_id(id, anchor_min, anchor_max, width))
        return;

    const Fonts& f = fonts();
    const ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const bool appearing = ImGui::IsWindowAppearing();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    if (appearing) {
        preset_filter_[0] = '\0';
        preset_menu_ = -1;
        rename_ = -1;
    }

    // Title chip and actions.
    float y = p.y;
    const float top_h = snap(px(24.0f));
    {
        const float cy = y + top_h * 0.5f;
        const float tw = draw::text_width(f.medium, font_size::text, "Presets");
        const ImVec2 cmin(p.x, y), cmax(snap(p.x + px(28.0f) + tw), y + top_h);
        dl->AddRectFilled(cmin, cmax, col::window_chip_bg, px(6.0f));
        draw::icon(dl, f.icons, 10.5f, ImVec2(cmin.x + px(12.0f), cy), col::accent, icon::cloud);
        draw::text(dl, f.medium, font_size::text, ImVec2(cmin.x + px(22.0f), cy - draw::cap_height(f.medium, font_size::text) * 0.5f),
                   col::window_label, "Presets");

        float x = p.x + w - px(10.0f);
        if (icon_button(dl, "##new", ImVec2(x, cy), icon::plus, 11.0f)) {
            add_preset("Unnamed", presets_[static_cast<size_t>(preset_)]->config, true);
            preset_filter_[0] = '\0';
        }
        x -= px(24.0f);
        if (icon_button(dl, "##import", ImVec2(x, cy), icon::file_import, 11.0f))
            toast(import_preset() ? "Preset imported from clipboard" : "Clipboard has no preset");
        x -= px(24.0f);
        if (icon_button(dl, "##export", ImVec2(x, cy), icon::upload, 11.0f) && export_preset(preset_))
            toast("Preset copied to clipboard");
    }
    y += top_h + snap(px(8.0f));

    // Search and sort.
    {
        const float h = snap(px(26.0f));
        const ImVec2 smin(p.x, y), smax(p.x + w - px(30.0f), y + h);
        const ImGuiID field_id = ImGui::GetID("##filter_bg");
        const bool active = ImGui::GetActiveID() == ImGui::GetID("##filter");
        const float fa = ui::animate(field_id, active ? 1.0f : 0.0f, 16.0f);
        dl->AddRectFilled(smin, smax, col::combo_bg, px(6.0f));
        draw::border(dl, smin, smax, draw::lerp_color(col::slider_box_border, col::accent, fa * 0.5f), px(6.0f),
                     draw::hairline());
        draw::icon(dl, f.icons, 10.0f, ImVec2(smin.x + px(12.0f), y + h * 0.5f),
                   draw::lerp_color(col::window_value, col::accent, fa), icon::search);
        if (ui::text_field("##filter", preset_filter_, sizeof(preset_filter_), ImVec2(smin.x + px(24.0f), y),
                           ImVec2(smax.x - px(6.0f), y + h), "Search"))
            preset_scroll_target_ = 0.0f;

        const ImVec2 sc(p.x + w - px(12.0f), y + h * 0.5f);
        if (icon_button(dl, "##sort", sc, icon::sort, 11.0f, preset_sort_ != 0)) {
            preset_sort_ = (preset_sort_ + 1) % 3;
            toast(k_sort_names[preset_sort_]);
        }
        y += h + snap(px(6.0f));
    }

    // Rows: filtered and sorted indices into presets_.
    std::vector<int> order;
    for (int i = 0; i < static_cast<int>(presets_.size()); ++i)
        if (!preset_filter_[0] || search::matches(presets_[static_cast<size_t>(i)]->name.c_str(), preset_filter_) ||
            i == rename_)
            order.push_back(i);
    if (preset_sort_ != 0)
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            const int c = ImStricmp(presets_[static_cast<size_t>(a)]->name.c_str(), presets_[static_cast<size_t>(b)]->name.c_str());
            return preset_sort_ == 1 ? c < 0 : c > 0;
        });

    const float row_h = snap(px(k_row_h));
    float content_h = 0.0f;
    for (int i : order)
        content_h += row_h * (presets_[static_cast<size_t>(i)].get() == deleting_ ? delete_anim_ : 1.0f);
    const float list_h = row_h * static_cast<float>(ImClamp(static_cast<int>(order.size()), 1, k_visible_rows));
    const float max_scroll = ImMax(0.0f, content_h - list_h);
    const ImVec2 lmin(p.x - px(4.0f), y), lmax(p.x + w + px(4.0f), y + list_h);
    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(lmin, lmax) && io.MouseWheel != 0.0f)
        preset_scroll_target_ -= io.MouseWheel * row_h * 1.5f;
    preset_scroll_target_ = ImClamp(preset_scroll_target_, 0.0f, max_scroll);
    if (appearing)
        preset_scroll_ = preset_scroll_target_;
    preset_scroll_ = ImLerp(preset_scroll_, preset_scroll_target_, 1.0f - std::exp(-18.0f * io.DeltaTime));

    if (order.empty()) {
        const char* text = "No presets found";
        const float tw = draw::text_width(f.medium, font_size::text, text);
        draw::text(dl, f.medium, font_size::text,
                   ImVec2(snap(p.x + (w - tw) * 0.5f), y + list_h * 0.5f - draw::cap_height(f.medium, font_size::text) * 0.5f),
                   col::window_value, text);
    }

    int load = -1, save = -1, open_menu = -1;
    ImVec2 menu_pos{};
    dl->PushClipRect(lmin, lmax, true);
    float ry = y - preset_scroll_;
    for (size_t n = 0; n < order.size(); ++n) {
        const int index = order[n];
        Preset& preset = *presets_[static_cast<size_t>(index)];
        const float fold = &preset == deleting_ ? delete_anim_ : 1.0f;
        const float h = row_h * fold;
        const ImVec2 rmin(p.x - px(4.0f), ry), rmax(p.x + w + px(4.0f), ry + row_h);
        ry += h;
        if (rmax.y < lmin.y || rmin.y > lmax.y || fold <= 0.0f)
            continue;

        ImGui::PushID(&preset);
        // Rows unfold one after another when the window opens or a preset is added.
        float* appear = storage->GetFloatRef(ImGui::GetID("##appear"), 0.0f);
        if (appearing)
            *appear = -0.07f * static_cast<float>(n);
        *appear = ImMin(1.0f, *appear + io.DeltaTime * 6.0f);
        const float ap = ImSaturate(*appear);
        const float e = (1.0f - (1.0f - ap) * (1.0f - ap)) * fold;
        const int v0 = dl->VtxBuffer.Size;

        const bool current = index == preset_;
        const float cy = (rmin.y + rmax.y) * 0.5f;
        const ImVec2 action_c(rmax.x - px(14.0f), cy);
        const ImVec2 dots_c(rmax.x - px(38.0f), cy);
        const bool buttons_live = rename_ != index && ImGui::IsMouseHoveringRect(lmin, lmax, false);

        // Buttons first, so they get the hover before the row under them.
        bool action = false, dots = false;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !buttons_live);
        {
            const ImGuiID aid = ImGui::GetID("##action");
            const float hit = px(10.0f);
            const ui::ItemState as = ui::item_behavior(aid, ImVec2(action_c.x - hit, action_c.y - hit),
                                                       ImVec2(action_c.x + hit, action_c.y + hit));
            action = as.pressed;
            const float ah = ui::animate(aid + 1, as.hovered ? 1.0f : 0.0f, 18.0f);
            const float sp = ui::animate(aid + 2, current ? 1.0f : 0.0f, 14.0f);
            const ImU32 c = draw::lerp_color(current ? col::accent : col::window_value, col::tab_text_active, ah);
            // The icon turns from "load" into "save" when the preset becomes the current one.
            if (sp < 1.0f)
                draw::icon(dl, f.icons, 11.0f * (1.0f - 0.4f * sp), ImVec2(action_c.x, action_c.y - px(4.0f) * sp),
                           draw::with_alpha(c, 1.0f - sp), icon::cloud_down);
            if (sp > 0.0f) {
                const float check = current && save_flash_ > 0.0f
                                        ? ImMin(ImSaturate((1.0f - save_flash_) * 8.0f), ImSaturate(save_flash_ * 6.0f))
                                        : 0.0f;
                const ImVec2 ic(action_c.x, action_c.y + px(4.0f) * (1.0f - sp));
                if (check < 1.0f)
                    draw::icon(dl, f.icons, 11.0f * (0.6f + 0.4f * sp) * (1.0f - 0.3f * check), ic,
                               draw::with_alpha(c, sp * (1.0f - check)), icon::floppy);
                if (check > 0.0f)
                    draw::icon(dl, f.icons, 11.0f * (0.7f + 0.3f * check), ic, draw::with_alpha(col::accent, check),
                               icon::check);
            }
            dots = ui::dots_button("##dots", dots_c);
        }
        ImGui::PopItemFlag();

        const ImGuiID row_id = ImGui::GetID("##row");
        const ui::ItemState rs = ui::item_behavior(row_id, ImVec2(rmin.x, ImMax(rmin.y, lmin.y)),
                                                   ImVec2(dots_c.x - px(10.0f), ImMin(rmax.y, lmax.y)),
                                                   rename_ != index);
        const float hv = ui::animate(row_id + 1, rs.hovered ? 1.0f : 0.0f, 18.0f);
        const float cur = ui::animate(row_id + 2, current ? 1.0f : 0.0f, 14.0f);
        const float rounding = px(6.0f);
        const ImVec2 bmin(rmin.x, rmin.y + px(1.0f)), bmax(rmax.x, rmax.y - px(1.0f));
        if (hv > 0.0f || cur > 0.0f)
            dl->AddRectFilled(bmin, bmax, draw::with_alpha(col::window_chip_bg, ImMax(hv * 0.8f, cur)), rounding);
        if (cur > 0.0f)
            draw::border(dl, bmin, bmax, draw::with_alpha(col::accent, cur * 0.45f), rounding, draw::hairline());

        float tx = rmin.x + px(10.0f) + hv * px(2.0f);
        if (preset.icon) {
            draw::icon(dl, f.icons, font_size::icon, ImVec2(tx + px(5.0f), cy), col::preset_sun, preset.icon);
            tx += px(15.0f);
        }
        if (rename_ == index) {
            const ImVec2 fmin(tx - px(4.0f), rmin.y + px(4.0f)), fmax(dots_c.x - px(12.0f), rmax.y - px(4.0f));
            dl->AddRectFilled(fmin, fmax, col::combo_bg, px(4.0f));
            draw::border(dl, fmin, fmax, draw::with_alpha(col::accent, 0.5f), px(4.0f), draw::hairline());
            ui::text_field("##rename", rename_buf_, sizeof(rename_buf_), ImVec2(tx, fmin.y), ImVec2(fmax.x - px(4.0f), fmax.y),
                           nullptr, rename_focus_);
            rename_focus_ = false;
            if (ImGui::IsItemDeactivated()) {
                if (rename_buf_[0] && !ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    preset.name = rename_buf_;
                    touch_presets();
                }
                rename_ = -1;
            }
        } else {
            ImFont* font = current ? f.semibold : f.medium;
            const ImU32 tc = draw::lerp_color(draw::lerp_color(col::window_value, col::window_label, hv), col::tab_text_active, cur);
            draw::text_fade_right(dl, font, font_size::text, ImVec2(snap(tx), cy - draw::cap_height(font, font_size::text) * 0.5f),
                                  tc, preset.name.c_str(), dots_c.x - px(30.0f), dots_c.x - px(14.0f));
        }

        if (rs.pressed && !current)
            load = index;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && rs.hovered) {
            rename_ = index;
            ImStrncpy(rename_buf_, preset.name.c_str(), sizeof(rename_buf_));
            rename_focus_ = true;
        }
        if (action)
            (current ? save : load) = index;
        if (dots) {
            open_menu = index;
            menu_pos = ImVec2(dots_c.x - px(6.0f), rmax.y);
        }

        fade_vertices(dl, v0, e, (1.0f - e) * px(6.0f));
        ImGui::PopID();
    }
    dl->PopClipRect();

    // Scrollbar
    if (max_scroll > 0.0f) {
        const float bar_x = lmax.x + px(2.0f), bar_w = snap(px(3.0f));
        const float thumb_h = ImMax(px(18.0f), list_h * list_h / content_h);
        const float thumb_y = y + (list_h - thumb_h) * (preset_scroll_ / max_scroll);
        dl->AddRectFilled(ImVec2(bar_x, y), ImVec2(bar_x + bar_w, y + list_h), IM_COL32(255, 255, 255, 8), bar_w * 0.5f);
        dl->AddRectFilled(ImVec2(bar_x, thumb_y), ImVec2(bar_x + bar_w, thumb_y + thumb_h), IM_COL32(255, 255, 255, 70),
                          bar_w * 0.5f);
    }
    ImGui::SetCursorScreenPos(p);
    ImGui::ItemSize(ImVec2(w, y + list_h - p.y));

    if (load >= 0) {
        select_preset(load);
        toast("Preset loaded");
    }
    if (save >= 0) {
        Preset& sp = *presets_[static_cast<size_t>(save)];
        sp.defaults = sp.config;
        save_flash_ = 1.0f;
    }

    // "•••" menu of a row.
    const ImGuiID menu_id = ImGui::GetID("##row_menu");
    if (open_menu >= 0) {
        preset_menu_ = open_menu;
        ImGui::OpenPopup(menu_id);
        storage->SetFloat(ImGui::GetID("##menu_x"), menu_pos.x);
        storage->SetFloat(ImGui::GetID("##menu_y"), menu_pos.y);
    }
    const ImVec2 mpos(storage->GetFloat(ImGui::GetID("##menu_x")), storage->GetFloat(ImGui::GetID("##menu_y")));
    if (ui::begin_popup_id(menu_id, ImVec2(mpos.x, mpos.y - px(4.0f)), mpos, px(140.0f))) {
        const int target = preset_menu_;
        const bool valid = target >= 0 && target < static_cast<int>(presets_.size());
        constexpr const char* items[] = { "Rename", "Duplicate", "Export", "Delete" };
        constexpr const char* icons[] = { icon::pen, icon::copy, icon::upload, icon::trash };
        const Fonts& mf = fonts();
        ImDrawList* mdl = ImGui::GetWindowDrawList();
        const ImVec2 mp = ImGui::GetCursorScreenPos();
        const float mw = ImGui::GetContentRegionAvail().x;
        const float item_h = snap(px(26.0f)), pad = snap(px(4.0f));
        ImGui::ItemSize(ImVec2(mw, item_h * IM_ARRAYSIZE(items)));
        int clicked = -1;
        for (int i = 0; i < IM_ARRAYSIZE(items); ++i) {
            // Delete is red, the rest grey until hovered.
            const bool danger = i == 3;
            const ImVec2 imin(mp.x - pad, mp.y + item_h * static_cast<float>(i)), imax(mp.x + mw + pad, imin.y + item_h);
            const ImGuiID iid = ImGui::GetID(items[i]);
            const ui::ItemState s = ui::item_behavior(iid, imin, imax);
            if (s.pressed)
                clicked = i;
            const float h = ui::animate(iid + 1, s.hovered ? 1.0f : 0.0f, 22.0f);
            if (h > 0.0f)
                mdl->AddRectFilled(imin, imax,
                                   draw::with_alpha(danger ? draw::with_alpha(col::window_danger, 0.16f) : col::popup_item_hover, h),
                                   px(5.0f));
            const ImU32 base = danger ? draw::lerp_color(col::window_danger, IM_COL32(255, 120, 120, 255), h)
                                      : draw::lerp_color(rgb(146, 148, 160), col::window_label, h);
            const float cy = (imin.y + imax.y) * 0.5f, slide = h * px(2.0f);
            draw::icon(mdl, mf.icons, 10.0f, ImVec2(snap(imin.x + px(14.0f) + slide), cy), base, icons[i]);
            draw::text(mdl, mf.medium, font_size::text,
                       ImVec2(snap(imin.x + px(28.0f) + slide), cy - draw::cap_height(mf.medium, font_size::text) * 0.5f),
                       base, items[i]);
        }
        if (clicked >= 0 && valid) {
            Preset& t = *presets_[static_cast<size_t>(target)];
            switch (clicked) {
            case 0:
                rename_ = target;
                ImStrncpy(rename_buf_, t.name.c_str(), sizeof(rename_buf_));
                rename_focus_ = true;
                break;
            case 1: {
                if (target == preset_)
                    binds::release_all();
                auto copy = std::make_unique<Preset>();
                copy->name = t.name + " (copy)";
                copy->icon = t.icon;
                copy->config = t.config;
                copy->defaults = t.defaults;
                copy->binds = t.binds;
                for (binds::Bind& b : copy->binds) {
                    b.toggled = b.applied = false;
                    b.shown = 0.0f;
                }
                presets_.insert(presets_.begin() + target + 1, std::move(copy));
                if (preset_ > target) {
                    ++preset_;
                }
                Preset& cur = *presets_[static_cast<size_t>(preset_)];
                binds::set_context(&cur.config, &cur.defaults, &cur.binds);
                touch_presets();
                toast("Preset duplicated");
                break;
            }
            case 2:
                if (export_preset(target))
                    toast("Preset copied to clipboard");
                break;
            default:
                if (presets_.size() > 1 && !deleting_) {
                    deleting_ = &t;
                    delete_anim_ = 1.0f;
                    toast("Preset deleted");
                } else if (presets_.size() <= 1) {
                    toast("The last preset can't be deleted");
                }
                break;
            }
            ImGui::CloseCurrentPopup();
        }
        ui::end_popup();
    }

    ui::end_popup();
}

} // namespace nl
