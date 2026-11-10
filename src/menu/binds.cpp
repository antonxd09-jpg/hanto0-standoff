// Menu source watermark: @use_sberbank
#include "menu/binds.h"

#include <cstdio>
#include <cstring>

#include "imgui_internal.h"
#include "menu/config.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/style.h"
#include "menu/widgets.h"

#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
#include <windows.h>
#endif

namespace nl::binds {
namespace {

#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
int virtual_key(ImGuiKey key)
{
    if (key >= ImGuiKey_A && key <= ImGuiKey_Z) return 'A' + (key - ImGuiKey_A);
    if (key >= ImGuiKey_0 && key <= ImGuiKey_9) return '0' + (key - ImGuiKey_0);
    if (key >= ImGuiKey_F1 && key <= ImGuiKey_F12) return VK_F1 + (key - ImGuiKey_F1);
    if (key >= ImGuiKey_Keypad0 && key <= ImGuiKey_Keypad9) return VK_NUMPAD0 + (key - ImGuiKey_Keypad0);
    switch (key) {
    case ImGuiKey_MouseLeft: return VK_LBUTTON;
    case ImGuiKey_MouseRight: return VK_RBUTTON;
    case ImGuiKey_MouseMiddle: return VK_MBUTTON;
    case ImGuiKey_MouseX1: return VK_XBUTTON1;
    case ImGuiKey_MouseX2: return VK_XBUTTON2;
    case ImGuiKey_LeftShift: return VK_LSHIFT;
    case ImGuiKey_RightShift: return VK_RSHIFT;
    case ImGuiKey_LeftCtrl: return VK_LCONTROL;
    case ImGuiKey_RightCtrl: return VK_RCONTROL;
    case ImGuiKey_LeftAlt: return VK_LMENU;
    case ImGuiKey_RightAlt: return VK_RMENU;
    case ImGuiKey_Space: return VK_SPACE;
    case ImGuiKey_Tab: return VK_TAB;
    case ImGuiKey_CapsLock: return VK_CAPITAL;
    case ImGuiKey_Enter: return VK_RETURN;
    case ImGuiKey_Backspace: return VK_BACK;
    case ImGuiKey_Insert: return VK_INSERT;
    case ImGuiKey_Delete: return VK_DELETE;
    case ImGuiKey_Home: return VK_HOME;
    case ImGuiKey_End: return VK_END;
    case ImGuiKey_PageUp: return VK_PRIOR;
    case ImGuiKey_PageDown: return VK_NEXT;
    case ImGuiKey_LeftArrow: return VK_LEFT;
    case ImGuiKey_RightArrow: return VK_RIGHT;
    case ImGuiKey_UpArrow: return VK_UP;
    case ImGuiKey_DownArrow: return VK_DOWN;
    case ImGuiKey_GraveAccent: return VK_OEM_3;
    case ImGuiKey_Minus: return VK_OEM_MINUS;
    case ImGuiKey_Equal: return VK_OEM_PLUS;
    case ImGuiKey_LeftBracket: return VK_OEM_4;
    case ImGuiKey_RightBracket: return VK_OEM_6;
    case ImGuiKey_Backslash: return VK_OEM_5;
    case ImGuiKey_Semicolon: return VK_OEM_1;
    case ImGuiKey_Apostrophe: return VK_OEM_7;
    case ImGuiKey_Comma: return VK_OEM_COMMA;
    case ImGuiKey_Period: return VK_OEM_PERIOD;
    case ImGuiKey_Slash: return VK_OEM_2;
    default: return 0;
    }
}

// The game swallows input before ImGui sees it while the menu is hidden, so the
// keyboard is read from the system whenever our process owns the foreground.
bool foreground_ours()
{
    const HWND window = ::GetForegroundWindow();
    DWORD owner = 0;
    if (window)
        ::GetWindowThreadProcessId(window, &owner);
    return owner == ::GetCurrentProcessId();
}
#endif

bool key_down(ImGuiKey key)
{
    if (key == ImGuiKey_None)
        return false;
#if defined(_WIN32) && !defined(KAMSHAKO_PORTABLE_MENU)
    if (const int vk = virtual_key(key))
        return foreground_ours() && (::GetAsyncKeyState(vk) & 0x8000) != 0;
#endif
    return ImGui::IsKeyDown(key);
}

struct edge_state {
    ImGuiKey key = ImGuiKey_None;
    bool down = false;
    bool pressed = false;
    int frame = -1;
};
edge_state g_edges[64]{};

bool key_pressed(ImGuiKey key)
{
    edge_state* slot = nullptr;
    for (edge_state& e : g_edges) {
        if (e.key == key) { slot = &e; break; }
        if (!slot && e.key == ImGuiKey_None) slot = &e;
    }
    if (!slot)
        return false;
    slot->key = key;
    const int frame = ImGui::GetFrameCount();
    if (slot->frame != frame) {
        const bool down = key_down(key);
        slot->pressed = down && !slot->down;
        slot->down = down;
        slot->frame = frame;
    }
    return slot->pressed;
}

using namespace nl::style;
using draw::snap;

Config* g_config = nullptr;
const Config* g_defaults = nullptr;
// Menu-owned settings that live outside the preset (profile window, watermark
// options). Their offsets are marked with the high bit so a bind identity never
// collides with a preset field at the same byte offset.
void* g_external = nullptr;
std::size_t g_external_size = 0;
List* g_list = nullptr;

int g_edit = -1;        // bind shown in the editor window
bool g_capture = false; // the editor waits for a key
float g_hotkeys_alpha = 0.0f;
bool g_hover_lock = false; // set when New Hotkey reflows the rows under a still pointer
ImVec2 g_hover_lock_pos;

constexpr int k_max_items = 24; // multi-select masks are stored in a float

// Marks an offset as relative to the menu-owned block instead of the preset.
constexpr std::size_t k_external = std::size_t{ 1 } << (sizeof( std::size_t ) * 8 - 1 );

// Context menu, editor and hotkey list share the row metrics.
constexpr float k_menu_w = 106.0f;
constexpr float k_editor_w = 150.0f;
constexpr float k_all_w = 168.0f;
constexpr float k_pad = 5.0f;
constexpr float k_rounding = 8.0f;
constexpr float k_row_h = 20.8f;
constexpr float k_row_rounding = 5.0f;
constexpr float k_icon_cx = 13.5f;
constexpr float k_text_x = 28.0f;
constexpr float k_control_pad = 6.7f;
constexpr float k_chip_h = 14.5f;
constexpr float k_chip_pad = 4.5f;
constexpr float k_chip_icon_w = 14.0f;
constexpr float k_group_gap = 4.7f;
constexpr float k_separator_h = 5.0f;
constexpr float k_editor_dy = 20.0f; // editor top above the entry that opened it
constexpr float k_value_w = 57.0f;
constexpr float k_combo_w = 74.0f;
constexpr float k_combo_h = 17.0f;
constexpr float k_mini_toggle = 0.78f;

// Hotkeys window
constexpr float k_list_x = 24.0f;
constexpr float k_list_y = 0.24f; // of the display height
constexpr float k_head_h = 20.8f, k_head_icon_cx = 12.3f, k_head_text_x = 26.4f, k_head_pad_right = 7.5f;
constexpr float k_item_h = 17.5f, k_item_icon_w = 19.5f, k_item_gap = 4.5f, k_item_pad = 5.2f, k_item_gap_y = 5.8f;
constexpr float k_pill_rounding = 6.0f;

ImGuiID slot(ImGuiID id, int n) { return ImHashData(&n, sizeof(n), id); }

std::size_t field_size(Kind kind, std::size_t items)
{
    switch (kind) {
    case Kind::toggle: return sizeof(bool);
    case Kind::slider: return sizeof(float);
    case Kind::combo: return sizeof(int);
    case Kind::multi: return items * sizeof(bool);
    }
    return 0;
}

unsigned char* field(const Bind& b)
{
    auto* base = (b.offset & k_external) != 0
        ? reinterpret_cast<unsigned char*>(g_external)
        : reinterpret_cast<unsigned char*>(g_config);
    return base ? base + (b.offset & ~k_external) : nullptr;
}

unsigned char* field(Config* c, std::size_t offset) { return reinterpret_cast<unsigned char*>(c) + offset; }

bool resolve(const void* ptr, std::size_t& offset)
{
    if (!ptr)
        return false;
    const auto* p = static_cast<const unsigned char*>(ptr);
    if (g_config) {
        const auto* base = reinterpret_cast<const unsigned char*>(g_config);
        if (p >= base && p < base + sizeof(Config)) {
            offset = static_cast<std::size_t>(p - base);
            return true;
        }
    }
    // Settings the menu owns rather than the preset -- the profile window and the
    // watermark options. Without this they are silently unbindable: right-clicking
    // such a row refuses to open the bind menu at all.
    if (g_external && g_external_size) {
        const auto* base = reinterpret_cast<const unsigned char*>(g_external);
        if (p >= base && p < base + g_external_size) {
            offset = k_external | static_cast<std::size_t>(p - base);
            return true;
        }
    }
    return false;
}

float read(const Bind& b)
{
    unsigned char* f = field(b);
    if (!f)
        return 0.0f;
    switch (b.kind) {
    case Kind::toggle: return *reinterpret_cast<bool*>(f) ? 1.0f : 0.0f;
    case Kind::slider: return *reinterpret_cast<float*>(f);
    case Kind::combo: return static_cast<float>(*reinterpret_cast<int*>(f));
    case Kind::multi: {
        const bool* selected = reinterpret_cast<bool*>(f);
        unsigned mask = 0;
        for (std::size_t i = 0; i < b.items.size() && i < k_max_items; ++i)
            if (selected[i])
                mask |= 1u << i;
        return static_cast<float>(mask);
    }
    }
    return 0.0f;
}

void write(const Bind& b, float v)
{
    unsigned char* f = field(b);
    if (!f)
        return;
    switch (b.kind) {
    case Kind::toggle: *reinterpret_cast<bool*>(f) = v > 0.5f; break;
    case Kind::slider: *reinterpret_cast<float*>(f) = v; break;
    case Kind::combo: *reinterpret_cast<int*>(f) = static_cast<int>(v); break;
    case Kind::multi: {
        bool* selected = reinterpret_cast<bool*>(f);
        const unsigned mask = static_cast<unsigned>(v);
        for (std::size_t i = 0; i < b.items.size() && i < k_max_items; ++i)
            selected[i] = (mask >> i) & 1u;
        break;
    }
    }
}

void restore(Bind& b)
{
    if (!b.applied)
        return;
    write(b, b.saved);
    b.applied = false;
}

Bind make_bind(const Target& t, std::size_t offset)
{
    Bind b;
    b.name.assign(t.label, ImGui::FindRenderedTextEnd(t.label));
    b.offset = offset;
    b.kind = t.kind;
    b.lo = t.lo;
    b.hi = t.hi;
    b.step = t.step;
    b.format = t.format;
    b.items = t.items;
    b.value = read(b);
    // Binding a toggle is meant to flip it.
    if (b.kind == Kind::toggle)
        b.value = b.value > 0.5f ? 0.0f : 1.0f;
    return b;
}

void erase(int index)
{
    restore((*g_list)[static_cast<std::size_t>(index)]);
    g_list->erase(g_list->begin() + index);
}

void reset(const Target& t, std::size_t offset)
{
    // Menu-owned settings have no defaults image to restore from; the bind's own
    // saved value is all there is.
    if (g_defaults && (offset & k_external) == 0) {
        const std::size_t size = field_size(t.kind, t.items.size());
        std::memcpy(field(g_config, offset), reinterpret_cast<const unsigned char*>(g_defaults) + offset, size);
    }
    for (Bind& b : *g_list)
        if (b.offset == offset && b.applied)
            b.saved = read(b);
}

const char* mode_icon(Mode mode)
{
    switch (mode) {
    case Mode::hold: return icon::hand;
    case Mode::always: return icon::infinity;
    default: return icon::toggle_on;
    }
}

bool active(const Bind& b, bool typing)
{
    switch (b.mode) {
    case Mode::always: return true;
    case Mode::hold: return b.key != ImGuiKey_None && !typing && key_down(b.key);
    default: return b.toggled;
    }
}

// Whether hovering a row of the context menu may switch its submenu: not while the pointer heads
// for the open submenu, and not after New Hotkey until the pointer moves.
bool hover_switch_allowed(int child_level)
{
    ImGuiContext& g = *GImGui;
    if (g_hover_lock) {
        const float slack = px(3.0f);
        if (ImLengthSqr(ImVec2(g.IO.MousePos.x - g_hover_lock_pos.x, g.IO.MousePos.y - g_hover_lock_pos.y)) < slack * slack)
            return false;
        g_hover_lock = false;
    }
    if (g.OpenPopupStack.Size <= child_level || (g.IO.MouseDelta.x == 0.0f && g.IO.MouseDelta.y == 0.0f))
        return true;
    const ImGuiWindow* child = g.OpenPopupStack[child_level].Window;
    if (!child || !child->WasActive)
        return true;
    const ImVec2 prev(g.IO.MousePos.x - g.IO.MouseDelta.x, g.IO.MousePos.y - g.IO.MouseDelta.y);
    const float x = child->Pos.x >= prev.x ? child->Pos.x : child->Pos.x + child->Size.x;
    const float pad = px(6.0f);
    return !ImTriangleContainsPoint(prev, ImVec2(x, child->Pos.y - pad), ImVec2(x, child->Pos.y + child->Size.y + pad),
                                    g.IO.MousePos);
}

// Rows of the bind windows. Hover is computed from the window under the mouse, so the context
// menu keeps reacting while its editor (a child popup) has the focus.
struct Line {
    ImVec2 min, max;
    float cy;
    bool hovered;
    bool pressed;
};

Line line(const char* str_id, bool highlight = false, float height = k_row_h)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = snap(px(height));
    ImGui::ItemSize(ImVec2(w, h));

    Line l;
    l.min = p;
    l.max = ImVec2(p.x + w, p.y + h);
    l.cy = p.y + h * 0.5f;
    const ImRect bb(l.min, l.max);
    l.hovered = g.HoveredWindow == window && g.ActiveId == 0 && bb.Contains(g.IO.MousePos);
    l.pressed = l.hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && bb.Contains(g.IO.MouseClickedPos[0]);
    const float t = ui::animate(window->GetID(str_id), (l.hovered || highlight) ? 1.0f : 0.0f, 18.0f);
    if (t > 0.0f) {
        const float inset = px(0.7f);
        window->DrawList->AddRectFilled(ImVec2(l.min.x, l.min.y + inset), ImVec2(l.max.x, l.max.y - inset),
                                        draw::with_alpha(col::window_row_hover, t), px(k_row_rounding));
    }
    return l;
}

void label(const Line& l, const char* icon, const char* text, ImU32 text_col = col::window_label,
           ImU32 icon_col = col::window_icon)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (icon)
        draw::any_icon(dl, ImVec2(l.min.x + px(k_icon_cx), l.cy), icon_col, icon);
    ImFont* font = fonts().medium;
    const float size = font_size::popup_label;
    draw::text(dl, font, size, ImVec2(snap(l.min.x + px(k_text_x)), l.cy - draw::cap_height(font, size) * 0.5f),
               text_col, text);
}

// Rounded tag right-aligned at `right` ("M4", "Toggle").
ImRect chip(ImDrawList* dl, float right, float cy, const char* icon, const char* text, float highlight)
{
    ImFont* font = fonts().medium;
    const float size = font_size::popup_label;
    const float icon_w = icon ? px(k_chip_icon_w) : 0.0f;
    const float w = snap(draw::text_width(font, size, text) + icon_w + 2.0f * px(k_chip_pad));
    const float h = snap(px(k_chip_h));
    const ImVec2 min(snap(right - w), snap(cy - h * 0.5f)), max(min.x + w, min.y + h);
    dl->AddRectFilled(min, max, draw::lerp_color(col::window_chip_bg, col::window_chip_hovered, highlight), px(4.5f));
    float x = min.x + px(k_chip_pad);
    if (icon) {
        draw::any_icon(dl, ImVec2(x + px(5.0f), cy), col::window_label, icon);
        x += icon_w;
    }
    draw::text(dl, font, size, ImVec2(snap(x), cy - draw::cap_height(font, size) * 0.5f),
               draw::lerp_color(col::window_value, col::window_label, highlight), text);
    return ImRect(min, max);
}

void separator()
{
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = snap(px(k_separator_h));
    ImGui::ItemSize(ImVec2(w, h));
    draw::hline(ImGui::GetWindowDrawList(), p.x + px(4.0f), p.x + w - px(4.0f), snap(p.y + h * 0.5f),
                col::window_separator);
}

void entry_label(const Bind& b, char* out, std::size_t size)
{
    std::snprintf(out, size, "%s \"%s\"", mode_name(b.mode), key_name(b.key));
}

// Waits for the next key; Escape cancels, Backspace and Delete clear the key.
void capture_key(Bind& b)
{
    for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k) {
        const ImGuiKey key = static_cast<ImGuiKey>(k);
        if (key == ImGuiKey_MouseLeft || key == ImGuiKey_MouseWheelX || key == ImGuiKey_MouseWheelY ||
            (key >= ImGuiKey_ReservedForModCtrl && key <= ImGuiKey_ReservedForModSuper))
            continue;
        if (!ImGui::IsKeyPressed(key, false))
            continue;
        if (key == ImGuiKey_Backspace || key == ImGuiKey_Delete)
            b.key = ImGuiKey_None;
        else if (key != ImGuiKey_Escape)
            b.key = key;
        b.toggled = false;
        g_capture = false;
        return;
    }
    // A click anywhere else gives up.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        g_capture = false;
}

void value_control(Bind& b, const Line& l, float right)
{
    switch (b.kind) {
    case Kind::toggle: {
        bool on = b.value > 0.5f;
        const ImVec2 ts = ui::toggle_size(k_mini_toggle);
        if (ui::toggle("##value_toggle", &on, ImVec2(right - ts.x, l.cy - ts.y * 0.5f), true, k_mini_toggle))
            b.value = on ? 1.0f : 0.0f;
        break;
    }
    case Kind::slider:
        ui::bar_slider("##value_slider", ImVec2(snap(right - px(k_value_w)), l.min.y), ImVec2(right, l.max.y), &b.value,
                       b.lo, b.hi, b.format, b.step);
        break;
    case Kind::combo: {
        int current = static_cast<int>(b.value);
        const float h = snap(px(k_combo_h));
        const ImVec2 min(snap(right - px(k_combo_w)), snap(l.cy - h * 0.5f));
        if (ui::combo_box("##value_combo", min, ImVec2(right, min.y + h), b.items, &current))
            b.value = static_cast<float>(current);
        break;
    }
    case Kind::multi: {
        bool selected[k_max_items] = {};
        const unsigned mask = static_cast<unsigned>(b.value);
        const std::size_t n = ImMin<std::size_t>(b.items.size(), k_max_items);
        for (std::size_t i = 0; i < n; ++i)
            selected[i] = (mask >> i) & 1u;
        const float h = snap(px(k_combo_h));
        const ImVec2 min(snap(right - px(k_combo_w)), snap(l.cy - h * 0.5f));
        if (ui::multi_combo_box("##value_multi", min, ImVec2(right, min.y + h), b.items.first(n), selected)) {
            unsigned m = 0;
            for (std::size_t i = 0; i < n; ++i)
                if (selected[i])
                    m |= 1u << i;
            b.value = static_cast<float>(m);
        }
        break;
    }
    }
}

ImVec2 beside(const ImGuiWindow* parent, float width, float top)
{
    ImVec2 pos(parent->Pos.x + parent->Size.x + px(1.0f), top);
    if (pos.x + width > ImGui::GetIO().DisplaySize.x - px(6.0f))
        pos.x = parent->Pos.x - width - px(1.0f);
    return pos;
}

void draw_editor(ImGuiID id, const ImGuiWindow* menu, float anchor)
{
    const float w = snap(px(k_editor_w));
    if (!ui::begin_popup_at(id, beside(menu, w, anchor - px(k_editor_dy)), w, ImVec2(px(k_pad), px(k_pad)),
                            px(k_rounding)))
        return;
    Bind& b = (*g_list)[static_cast<std::size_t>(g_edit)];
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    ImGui::PushID(g_edit);

    const Line key = line("##key", g_capture);
    label(key, icon::key_bind, "Key");
    const float right = snap(key.max.x - px(k_control_pad));
    chip(dl, right, key.cy, nullptr, g_capture ? "..." : key_name(b.key), (key.hovered || g_capture) ? 1.0f : 0.0f);
    if (key.pressed)
        g_capture = !g_capture;
    else if (g_capture)
        capture_key(b);

    const ImGuiID mode_id = window->GetID("##mode_list");
    const bool mode_open = ImGui::IsPopupOpen(mode_id, ImGuiPopupFlags_None);
    const Line mode = line("##mode", mode_open);
    label(mode, icon::keyboard, "Mode");
    const float kb_cx = right - px(5.0f);
    draw::any_icon(dl, ImVec2(kb_cx, mode.cy), col::window_label, icon::keyboard_down);
    const ImRect mc = chip(dl, snap(kb_cx - px(10.5f)), mode.cy, mode_icon(b.mode), mode_name(b.mode),
                           (mode.hovered || mode_open) ? 1.0f : 0.0f);
    if (mode.pressed && !mode_open)
        ImGui::OpenPopup(mode_id);
    if (ui::begin_popup_id(mode_id, mc.Min, mc.Max, px(110.0f))) {
        for (int m = 0; m < 3; ++m) {
            ImGui::PushID(m);
            if (ui::dropdown_item(mode_name(static_cast<Mode>(m)), b.mode == static_cast<Mode>(m), false)) {
                b.mode = static_cast<Mode>(m);
                b.toggled = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ui::end_popup();
    }

    ImGui::ItemSize(ImVec2(0.0f, snap(px(k_group_gap))));
    const Line value = line("##value");
    label(value, icon::link, "Value");
    value_control(b, value, right);

    const Line visible = line("##visible");
    label(visible, icon::eye, "Visible");
    const ImVec2 ts = ui::toggle_size(k_mini_toggle);
    ui::toggle("##visible_toggle", &b.visible, ImVec2(right - ts.x, visible.cy - ts.y * 0.5f), true, k_mini_toggle);

    separator();
    const Line del = line("##delete");
    label(del, icon::trash, "Delete", col::window_danger, col::window_danger);
    ImGui::PopID();
    if (del.pressed) {
        erase(g_edit);
        g_edit = -1;
        g_capture = false;
        ImGui::CloseCurrentPopup();
    }
    ui::end_popup();
}

void draw_all(ImGuiID id, const ImGuiWindow* menu, float anchor)
{
    const float w = snap(px(k_all_w));
    if (!ui::begin_popup_at(id, beside(menu, w, anchor - px(k_pad)), w, ImVec2(px(k_pad), px(k_pad)), px(k_rounding)))
        return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (g_list->empty()) {
        const Line l = line("##empty");
        label(l, nullptr, "No hotkeys yet", col::window_value);
    }
    for (std::size_t i = 0; i < g_list->size(); ++i) {
        const Bind& b = (*g_list)[i];
        ImGui::PushID(static_cast<int>(i));
        const Line l = line("##bind");
        label(l, mode_icon(b.mode), b.name.c_str());
        chip(dl, snap(l.max.x - px(k_control_pad)), l.cy, nullptr, key_name(b.key), l.hovered ? 1.0f : 0.0f);
        ImGui::PopID();
    }
    ui::end_popup();
}

void draw_menu(ImGuiID id, const Target& t, std::size_t offset)
{
    ImGuiStorage* storage = ImGui::GetStateStorage();
    const ImVec2 pos(storage->GetFloat(slot(id, 0)), storage->GetFloat(slot(id, 1)));
    ImFont* font = fonts().medium;
    char text[96];
    float width = px(k_menu_w);
    for (const Bind& b : *g_list)
        if (b.offset == offset) {
            entry_label(b, text, sizeof(text));
            width = ImMax(width, draw::text_width(font, font_size::popup_label, text) + px(k_text_x + 22.0f + k_pad));
        }
    if (!ui::begin_popup_at(id, pos, width, ImVec2(px(k_pad), px(k_pad)), px(k_rounding)))
        return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = window->DrawList;
    const int level = GImGui->BeginPopupStack.Size;
    const ImGuiID editor_id = window->GetID("##editor");
    const ImGuiID all_id = window->GetID("##all");
    bool editor_open = ImGui::IsPopupOpen(editor_id, ImGuiPopupFlags_None);
    const int count = static_cast<int>(g_list->size());
    if (editor_open && (g_edit < 0 || g_edit >= count || (*g_list)[static_cast<std::size_t>(g_edit)].offset != offset)) {
        ImGui::ClosePopupToLevel(level, true);
        editor_open = false;
    }
    float* anchor = storage->GetFloatRef(slot(editor_id, 0), window->Pos.y);
    const bool may_switch = hover_switch_allowed(level);

    for (int i = 0; i < count; ++i) {
        const Bind& b = (*g_list)[static_cast<std::size_t>(i)];
        if (b.offset != offset)
            continue;
        ImGui::PushID(i);
        const bool current = editor_open && g_edit == i;
        const Line l = line("##entry", current);
        entry_label(b, text, sizeof(text));
        label(l, icon::key_bind, text);
        draw::chevron_right(dl, ImVec2(l.max.x - px(9.0f), l.cy), px(1.8f), px(3.6f), col::window_label, px(1.5f));
        if (((l.hovered && may_switch) || l.pressed) && !current) {
            g_edit = i;
            g_capture = false;
            *anchor = l.min.y;
            if (!editor_open)
                ImGui::OpenPopup(editor_id);
            editor_open = true;
        }
        ImGui::PopID();
    }

    const bool all_open = ImGui::IsPopupOpen(all_id, ImGuiPopupFlags_None);
    const Line all = line("##all_line", all_open);
    label(all, icon::list_ul, "All Hotkeys");
    if (((all.hovered && may_switch) || all.pressed) && !all_open) {
        storage->SetFloat(slot(all_id, 0), all.min.y);
        ImGui::OpenPopup(all_id);
    }

    const Line add = line("##new");
    label(add, icon::circle_plus, "New Hotkey");
    if (add.pressed) {
        g_list->push_back(make_bind(t, offset));
        g_edit = static_cast<int>(g_list->size()) - 1;
        g_capture = true;
        g_hover_lock = true;
        g_hover_lock_pos = ImGui::GetIO().MousePos;
        *anchor = all.min.y;
        ImGui::OpenPopup(editor_id);
    }

    separator();
    const Line rst = line("##reset");
    label(rst, icon::rotate_left, "Reset", col::window_danger, col::window_danger);
    if (rst.pressed) {
        reset(t, offset);
        ImGui::CloseCurrentPopup();
    } else if ((add.hovered || rst.hovered) && (editor_open || all_open) && !add.pressed && may_switch) {
        ImGui::ClosePopupToLevel(level, true);
    } else if (all.hovered && editor_open && may_switch) {
        g_capture = false;
    }

    if (g_edit >= 0 && g_edit < static_cast<int>(g_list->size()) && ImGui::IsPopupOpen(editor_id, ImGuiPopupFlags_None))
        draw_editor(editor_id, window, *anchor);
    if (ImGui::IsPopupOpen(all_id, ImGuiPopupFlags_None))
        draw_all(all_id, window, storage->GetFloat(slot(all_id, 0), window->Pos.y));
    ui::end_popup();
}

// A translucent pill of the overlay windows.
void pill(ImDrawList* dl, const ImVec2& min, const ImVec2& max)
{
    const float rounding = px(k_pill_rounding);
    ui::glass(dl, min, max, rounding, col::overlay_pill, false);
    draw::border(dl, min, max, col::window_border, rounding, draw::hairline());
}

} // namespace

void set_context(Config* config, const Config* defaults, List* list)
{
    g_config = config;
    g_defaults = defaults;
    g_list = list;
    if (!list)
        g_edit = -1;
}

void set_external_state(void* base, std::size_t size)
{
    g_external = base;
    g_external_size = size;
}

Bind* add(const Target& target)
{
    std::size_t offset = 0;
    if (!g_list || !resolve(target.ptr, offset))
        return nullptr;
    g_list->push_back(make_bind(target, offset));
    return &g_list->back();
}

void row(const Target& target, const ImVec2& min, const ImVec2& max)
{
    std::size_t offset = 0;
    if (!g_list || !resolve(target.ptr, offset))
        return;
    // Rows under an overlay (the Ambience view, the skin picker) must not bind.
    if (ImGui::GetCurrentContext()->CurrentItemFlags & ImGuiItemFlags_Disabled)
        return;
    const ImGuiID id = ImGui::GetID("##bind_menu");
    // Opened on release: ImGui closes popups on the right-click itself.
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered() &&
        ImGui::IsMouseHoveringRect(min, max) && ImRect(min, max).Contains(ImGui::GetIO().MouseClickedPos[1])) {
        ImGuiStorage* storage = ImGui::GetStateStorage();
        const ImVec2 m = ImGui::GetIO().MousePos;
        storage->SetFloat(slot(id, 0), m.x + px(2.0f));
        storage->SetFloat(slot(id, 1), m.y - px(k_pad + k_row_h * 0.5f));
        g_edit = -1;
        g_capture = false;
        ImGui::OpenPopup(id);
    }
    if (ImGui::IsPopupOpen(id, ImGuiPopupFlags_None))
        draw_menu(id, target, offset);
}

bool bound(const void* ptr)
{
    std::size_t offset = 0;
    if (!g_list || !resolve(ptr, offset))
        return false;
    for (const Bind& b : *g_list)
        if (b.offset == offset)
            return true;
    return false;
}

void bound_icon(ImDrawList* dl, float x, float cy, ImU32 col)
{
    draw::any_icon(dl, ImVec2(snap(x + px(10.0f)), cy), col, icon::keyboard, 9.5f);
}

void update()
{
    if (!g_list || !g_config)
        return;
    const bool typing = ImGui::GetIO().WantTextInput || g_capture;
    for (Bind& b : *g_list) {
        if (b.mode == Mode::toggle && b.key != ImGuiKey_None && !typing && key_pressed(b.key))
            b.toggled = !b.toggled;
        if (active(b, typing)) {
            if (!b.applied) {
                b.saved = read(b);
                b.applied = true;
            }
            write(b, b.value);
        } else {
            restore(b);
        }
    }
}

void release_all()
{
    if (!g_list || !g_config)
        return;
    for (auto it = g_list->rbegin(); it != g_list->rend(); ++it)
        restore(*it);
}

const char* key_name(ImGuiKey key)
{
    switch (key) {
    case ImGuiKey_None: return "None";
    case ImGuiKey_MouseLeft: return "M1";
    case ImGuiKey_MouseRight: return "M2";
    case ImGuiKey_MouseMiddle: return "M3";
    case ImGuiKey_MouseX1: return "Mouse4";
    case ImGuiKey_MouseX2: return "Mouse5";
    case ImGuiKey_LeftShift:
    case ImGuiKey_RightShift: return "Shift";
    case ImGuiKey_LeftCtrl:
    case ImGuiKey_RightCtrl: return "Ctrl";
    case ImGuiKey_LeftAlt:
    case ImGuiKey_RightAlt: return "Alt";
    case ImGuiKey_LeftSuper:
    case ImGuiKey_RightSuper: return "Win";
    default: return ImGui::GetKeyName(key);
    }
}

const char* mode_name(Mode mode)
{
    switch (mode) {
    case Mode::hold: return "Hold";
    case Mode::always: return "Always";
    default: return "Toggle";
    }
}

// How bright a configured-but-inactive bind stays in the list while the menu is closed.
constexpr float k_resting_row = 0.42f;

void draw_hotkeys(bool show, bool menu_open)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float dt = io.DeltaTime;
    const bool typing = io.WantTextInput || g_capture;
    bool any_active = false;
    // Each row eases toward one target: full while its bind is active, dimmed while the menu
    // is open and the bind is set up (so the open menu lists what is configured), gone
    // otherwise. A single target per frame keeps rows from stalling half-faded, which the
    // old fade-out-then-bump-up pair did whenever a row sat below the dim level.
    if (g_list)
        for (Bind& b : *g_list) {
            const bool configured = b.visible && (b.key != ImGuiKey_None || b.mode == Mode::always);
            const bool on = configured && active(b, typing);
            any_active |= on;
            const float target = on ? 1.0f : (menu_open && configured ? k_resting_row : 0.0f);
            const float step = dt * 6.5f;
            b.shown = b.shown < target ? ImMin(target, b.shown + step) : ImMax(target, b.shown - step);
        }
    // Hidden while nothing is active; the menu shows the (possibly empty) window so it can be moved.
    const bool want = show && g_list && (menu_open || any_active);
    g_hotkeys_alpha = ImClamp(g_hotkeys_alpha + (want ? dt : -dt) * 6.5f, 0.0f, 1.0f);
    if (g_hotkeys_alpha <= 0.0f || !g_list)
        return;
    const float window_e = 1.0f - (1.0f - g_hotkeys_alpha) * (1.0f - g_hotkeys_alpha) * (1.0f - g_hotkeys_alpha);

    ImGui::SetNextWindowPos(ImVec2(snap(px(k_list_x)), std::floor(io.DisplaySize.y * k_list_y)), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNav;
    if (!menu_open)
        flags |= ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1.0f, 1.0f));
    ImGui::Begin("##hotkeys", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const int v0 = dl->VtxBuffer.Size;
    ImFont* semibold = fonts().semibold;
    ImFont* medium = fonts().medium;
    const float size = font_size::popup_label;

    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float head_h = snap(px(k_head_h));
    const float head_w = snap(px(k_head_text_x + k_head_pad_right) + draw::text_width(semibold, size, "Hotkeys"));
    pill(dl, p, ImVec2(p.x + head_w, p.y + head_h));
    const float head_cy = p.y + head_h * 0.5f;
    draw::any_icon(dl, ImVec2(p.x + px(k_head_icon_cx), head_cy), col::window_label, icon::list_tree);
    draw::text(dl, semibold, size, ImVec2(snap(p.x + px(k_head_text_x)), head_cy - draw::cap_height(semibold, size) * 0.5f),
               col::window_label, "Hotkeys");
    ImGui::ItemSize(ImVec2(head_w, head_h));

    char value[48];
    for (const Bind& b : *g_list) {
        if (b.shown <= 0.0f)
            continue;
        // Rows unfold from the header: the gap grows and the pills slide in from the left.
        const float e = 1.0f - (1.0f - b.shown) * (1.0f - b.shown) * (1.0f - b.shown);
        const int row_v0 = dl->VtxBuffer.Size;
        ImGui::ItemSize(ImVec2(0.0f, snap(px(k_item_gap_y) * e)));
        const ImVec2 q0 = ImGui::GetCursorScreenPos();
        const float h = snap(px(k_item_h));
        const ImVec2 q(q0.x, q0.y - (1.0f - e) * h * 0.6f);
        const float cy = q.y + h * 0.5f;
        const ImU32 text_col = col::window_label;

        // State: a toggle glyph for switches, the bound value for the rest.
        const char* state_icon = nullptr;
        value[0] = '\0';
        switch (b.kind) {
        case Kind::toggle: state_icon = icon::toggle_on; break;
        case Kind::multi: state_icon = icon::list_ul; break;
        case Kind::slider: std::snprintf(value, sizeof(value), b.format ? b.format : "%.0f", b.value); break;
        case Kind::combo:
            if (!b.items.empty())
                std::snprintf(value, sizeof(value), "%s",
                              b.items[static_cast<std::size_t>(ImClamp(static_cast<int>(b.value), 0,
                                                                       static_cast<int>(b.items.size()) - 1))]);
            break;
        }
        const float icon_w =
            state_icon ? snap(px(k_item_icon_w)) : snap(draw::text_width(medium, size, value) + 2.0f * px(k_item_pad));
        pill(dl, q, ImVec2(q.x + icon_w, q.y + h));
        if (state_icon)
            draw::any_icon(dl, ImVec2(q.x + icon_w * 0.5f, cy), text_col, state_icon);
        else
            draw::text(dl, medium, size, ImVec2(q.x + px(k_item_pad), cy - draw::cap_height(medium, size) * 0.5f), text_col,
                       value);

        const float name_x = snap(q.x + icon_w + px(k_item_gap));
        const float name_w = snap(draw::text_width(medium, size, b.name.c_str()) + 2.0f * px(k_item_pad));
        pill(dl, ImVec2(name_x, q.y), ImVec2(name_x + name_w, q.y + h));
        draw::text(dl, medium, size, ImVec2(name_x + px(k_item_pad), cy - draw::cap_height(medium, size) * 0.5f),
                   text_col, b.name.c_str());
        ImGui::ItemSize(ImVec2(name_x + name_w - q.x, h * e));
        if (e < 1.0f) {
            const float slide = (1.0f - e) * -px(14.0f);
            for (int i = row_v0; i < dl->VtxBuffer.Size; ++i) {
                dl->VtxBuffer[i].col = draw::with_alpha(dl->VtxBuffer[i].col, e);
                dl->VtxBuffer[i].pos.x += slide;
            }
        }
    }

    if (g_hotkeys_alpha < 1.0f)
        for (int i = v0; i < dl->VtxBuffer.Size; ++i) {
            dl->VtxBuffer[i].col = draw::with_alpha(dl->VtxBuffer[i].col, window_e);
            dl->VtxBuffer[i].pos.y += (1.0f - window_e) * -px(6.0f);
        }
    ImGui::End();
}

} // namespace nl::binds
