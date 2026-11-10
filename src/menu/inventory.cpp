// Menu source watermark: @use_sberbank
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/agent_models.h"
#include "menu/esp_model.h"
#include "menu/images.h"
#include "menu/inventory_catalog.h"
#include "menu/inventory_host.h"
#include "menu/menu.h"
#include "menu/model_catalog.h"
#include "menu/model_preview.h"
#include "menu/style.h"
#include "menu/widgets.h"

namespace nl {
namespace {

using namespace nl::style;
using draw::snap;
using inv::Kind;

// Loadout (menu units)
constexpr float k_switch_cx = 221.0f, k_switch_cy = 61.6f, k_switch_h = 18.5f;
constexpr float k_body_x0 = 150.0f, k_body_x1 = 286.0f, k_body_y0 = 80.0f, k_body_y1 = 364.0f;
constexpr float k_slot_x[3] = { 143.7f, 199.4f, 255.1f };
constexpr float k_slot_w = 47.8f;
constexpr float k_slot_y[2] = { 379.0f, 431.0f };
constexpr float k_slot_h = 46.0f;
constexpr float k_column_cx[3] = { 366.6f, 471.4f, 576.3f };
constexpr float k_column_cap_y = 55.0f;
constexpr float k_card_x[3] = { 318.5f, 423.3f, 528.2f };
constexpr float k_card_w = 96.3f, k_card_h = 76.5f;
constexpr float k_card_y0 = 72.9f, k_card_pitch = 82.2f;
constexpr float k_bar_h = 2.4f;
constexpr float k_card_rounding = 5.0f;

// Picker (menu units)
constexpr float k_center_x = (layout::main_x + layout::width) * 0.5f;
constexpr float k_close_x = 612.5f, k_close_y = 20.0f;
constexpr float k_title_bar_w = 238.0f;
constexpr float k_preview_x0 = 175.0f, k_preview_x1 = 592.0f, k_preview_y1 = 220.0f;
constexpr float k_tool_y = 231.0f;
constexpr float k_filter_cy = 269.0f, k_filter_h = 21.0f;
constexpr float k_grid_x0 = 143.0f, k_grid_x1 = 624.0f, k_grid_y0 = 285.0f, k_grid_y1 = 481.0f;
constexpr int k_grid_columns = 5;
constexpr float k_grid_gap = 4.7f;
constexpr float k_grid_card_h = 77.0f, k_grid_pitch = 101.0f;

constexpr const char* k_rarity_filters[] = { "Rarity",     "Consumer",   "Industrial", "Mil-Spec",
                                             "Restricted", "Classified", "Covert",     "Contraband" };
constexpr ImU32 k_rarity_filter_colors[] = { 0,
                                             IM_COL32(0xb0, 0xc3, 0xd9, 255),
                                             IM_COL32(0x5e, 0x98, 0xd9, 255),
                                             IM_COL32(0x4b, 0x69, 0xff, 255),
                                             IM_COL32(0x88, 0x47, 0xff, 255),
                                             IM_COL32(0xd3, 0x2c, 0xe6, 255),
                                             IM_COL32(0xeb, 0x4b, 0x4b, 255),
                                             IM_COL32(0xe4, 0xae, 0x39, 255) };

ImVec2 at(const ImVec2& o, float x, float y) { return ImVec2(o.x + snap(px(x)), o.y + snap(px(y))); }

const inv::Weapon& weapon(int i) { return inv::host ? inv::host->weapons[i] : inv::weapons[i]; }
const inv::Item& item(int i) { return inv::host ? inv::host->items[i] : inv::items[i]; }
int weapon_count() { return inv::host ? int(inv::host->weapons.size()) : inv::weapon_count; }
int item_count() { return inv::host ? int(inv::host->items.size()) : inv::item_count; }
void inventory_image(ImDrawList* dl,int image,const ImVec2& min,const ImVec2& max,ImU32 tint=IM_COL32_WHITE,bool crop=false) {
    if(inv::host)inv::host->image(dl,image,min,max,tint);
    else images::draw(dl,image,min,max,tint,crop);
}

Kind slot_kind(int slot)
{
    if (slot < inv::slot_mid_tier)
        return Kind::pistol;
    if (slot < inv::slot_rifle)
        return Kind::mid_tier;
    if (slot < inv::slot_agent)
        return Kind::rifle;
    switch (slot) {
    case inv::slot_agent: return Kind::agent;
    case inv::slot_medal: return Kind::medal;
    case inv::slot_music_kit: return Kind::music_kit;
    case inv::slot_knife: return Kind::knife;
    case inv::slot_gloves: return Kind::gloves;
    default: return Kind::zeus;
    }
}

bool is_gun(Kind k) { return k == Kind::pistol || k == Kind::mid_tier || k == Kind::rifle || k == Kind::zeus; }

std::uint8_t team_bit(int side) { return side == 0 ? inv::team_ct : inv::team_t; }

std::vector<int> slot_weapons(int slot, int side)
{
    std::vector<int> out;
    const Kind kind = slot_kind(slot);
    for (int i = 0; i < weapon_count(); ++i)
        if (weapon(i).kind == kind && (weapon(i).teams & team_bit(side)))
            out.push_back(i);
    return out;
}

bool is_default(const inv::Equip& e) { return weapon(e.weapon).has_default && e.item == weapon(e.weapon).first_item; }

ImU32 rarity_color(int item_index) { return inv::host ? inv::host->rarities[item(item_index).rarity].color : inv::rarities[item(item_index).rarity].color; }

// "AWP | Fade", "Default T Gloves", or the item name for agents, medals and music kits.
std::string title_of(const inv::Equip& e)
{
    const inv::Weapon& w = weapon(e.weapon);
    const inv::Item& it = item(e.item);
    if(inv::host)return it.name;
    if (w.kind == Kind::agent || w.kind == Kind::medal || w.kind == Kind::music_kit)
        return it.name;
    if (is_default(e))
        return std::strncmp(it.name, "Default ", 8) == 0 ? it.name : w.name;
    return std::string(w.name) + " | " + it.name;
}

bool contains_nocase(const char* text, const char* needle)
{
    if (!needle[0])
        return true;
    const size_t n = std::strlen(needle);
    for (const char* p = text; *p; ++p) {
        size_t i = 0;
        while (i < n && p[i] && std::tolower(static_cast<unsigned char>(p[i])) ==
                                    std::tolower(static_cast<unsigned char>(needle[i])))
            ++i;
        if (i == n)
            return true;
    }
    return false;
}

// Rounded card with a picture and the rarity line along its bottom edge. Returns true when clicked.
bool card(ImDrawList* dl, const char* str_id, const ImVec2& min, const ImVec2& max, int image, ImU32 bar,
          const ImVec2& pad, bool selected_ring = false)
{
    const ImGuiID id = ImGui::GetID(str_id);
    const ui::ItemState s = ui::item_behavior(id, min, max);
    const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
    const float rounding = px(k_card_rounding);
    dl->AddRectFilled(min, max, draw::lerp_color(col::card_bg, col::card_bg_hovered, h), rounding);
    if (h > 0.0f)
        draw::border(dl, min, max, draw::with_alpha(col::card_border, h), rounding, draw::hairline());
    const float bar_h = snap(px(k_bar_h));
    // Hovered pictures grow a little, like the reference.
    const float grow = px(2.0f) * h;
    inventory_image(dl, image, ImVec2(min.x + pad.x - grow, min.y + pad.y - grow),
                 ImVec2(max.x - pad.x + grow, max.y - bar_h - pad.y + grow), IM_COL32_WHITE, true);
    dl->AddRectFilled(ImVec2(min.x, max.y - bar_h), max, bar, px(1.0f));
    if (selected_ring) {
        const ImVec2 c(max.x - px(10.0f), min.y + px(10.0f));
        dl->AddCircle(c, px(4.6f), col::picker_accent, 24, px(1.2f));
        dl->AddCircleFilled(c, px(2.3f), col::picker_accent, 16);
    }
    return s.pressed;
}

// Counter-Terrorist and Terrorist emblems of the loadout switch.
void side_emblem(ImDrawList* dl, const ImVec2& c, int side, float alpha)
{
    const float r = px(6.6f);
    if (side == 0) {
        dl->AddCircleFilled(c, r, draw::with_alpha(IM_COL32(26, 34, 50, 255), alpha), 32);
        dl->AddCircle(c, r - px(0.6f), draw::with_alpha(IM_COL32(178, 196, 222, 255), alpha), 32, px(1.2f));
        const ImVec2 pts[5] = { { c.x - px(3.4f), c.y + px(2.2f) }, { c.x - px(1.7f), c.y - px(2.4f) },
                                { c.x, c.y + px(0.6f) },             { c.x + px(1.7f), c.y - px(2.4f) },
                                { c.x + px(3.4f), c.y + px(2.2f) } };
        dl->AddPolyline(pts, 5, draw::with_alpha(IM_COL32(214, 226, 244, 255), alpha), px(1.3f));
    } else {
        dl->AddCircleFilled(c, r, draw::with_alpha(IM_COL32(206, 166, 84, 255), alpha), 32);
        dl->AddCircle(c, r - px(1.6f), draw::with_alpha(IM_COL32(150, 112, 46, 255), alpha), 32, px(1.0f));
        dl->AddCircleFilled(c, px(2.1f), draw::with_alpha(IM_COL32(120, 86, 34, 255), alpha), 16);
    }
}

bool tool_icon(ImDrawList* dl, const char* str_id, const ImVec2& c, const char* glyph, bool active = false)
{
    const float hit = px(9.0f);
    const ImGuiID id = ImGui::GetID(str_id);
    const ui::ItemState s = ui::item_behavior(id, ImVec2(c.x - hit, c.y - hit), ImVec2(c.x + hit, c.y + hit));
    const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
    const ImU32 base = active ? col::accent : col::window_value;
    draw::any_icon(dl, c, draw::lerp_color(base, col::tab_text_active, h), glyph, 11.5f);
    return s.pressed;
}

// Exterior conditions and the wear each one starts at when picked from the list.
constexpr const char* k_conditions[] = { "Factory New", "Minimal Wear", "Field-Tested", "Well-Worn", "Battle-Scarred" };
constexpr float k_condition_end[] = { 0.07f, 0.15f, 0.38f, 0.45f, 1.0f };
constexpr float k_condition_pick[] = { 0.01f, 0.10f, 0.25f, 0.41f, 0.60f };
constexpr const char* k_stat_trak[] = { "Disabled", "Static" };

int condition_of(float wear)
{
    for (int i = 0; i < 4; ++i)
        if (wear < k_condition_end[i])
            return i;
    return 4;
}

// The settings window of a loadout slot (the poll icon of the toolbar).
void draw_finish_options(SlotFinish& rolled, Kind kind)
{
    const bool gloves = kind == Kind::gloves;
    ui::popup::group([&] {
        if (!gloves)
            ui::popup::input("Name", rolled.name, sizeof(rolled.name));
        float seed = static_cast<float>(rolled.seed);
        if (ui::popup::slider("Seed", &seed, 0.0f, 1000.0f, "%.0f"))
            rolled.seed = static_cast<int>(seed);
        ui::popup::toggle("Colors", &rolled.colors);
    });
    ui::popup::group([&] {
        int condition = condition_of(rolled.wear);
        if (ui::popup::value("Wear", &condition, k_conditions))
            rolled.wear = k_condition_pick[condition];
        ui::popup::wear_bar("##wear_bar", &rolled.wear);
    });
    if (!gloves) {
        ui::popup::group([&] {
            int mode = rolled.stat_trak ? 1 : 0;
            if (ui::popup::value("StatTrak\xE2\x84\xA2", &mode, k_stat_trak))
                rolled.stat_trak = mode == 1;
            ui::popup::number("Value", &rolled.kills, 0, 999999, rolled.stat_trak);
        });
    }
    ui::popup::group([&] {
        ui::popup::material("Model", &rolled.model_color, &rolled.model_material, inv::k_model_materials,
                            &rolled.model_overlay, &rolled.model_exponent);
    });
}

} // namespace

void Menu::open_picker(int slot, int item)
{
    tab_ = 3;
    InventorySettings& inv = config().inventory;
    picker_slot_ = ImClamp(slot, 0, inv::slot_count - 1);
    if(inv::host&&!inv::host_slot(picker_slot_)){picker_slot_=-1;return;}
    picker_weapon_ = inv.loadout[static_cast<size_t>(inv.side)][static_cast<size_t>(picker_slot_)].weapon;
    rarity_filter_ = 0;
    search_[0] = '\0';
    grid_scroll_ = 0.0f;

    // A specific item is only meaningful when it belongs to the weapon the slot holds;
    // otherwise the slot keeps its own pick and the picker just shows that weapon.
    if (item >= 0 && item < item_count()) {
        // A saved loadout can leave a weapon of the wrong kind in the slot. The
        // requested item then names which glove (or gun) it belongs to.
        const Kind kind = slot_kind(picker_slot_);
        const inv::Weapon& current = weapon(picker_weapon_);
        if (item < current.first_item || item >= current.first_item + current.item_count) {
            for (int i = 0; i < weapon_count(); ++i) {
                const inv::Weapon& cand = weapon(i);
                if (cand.kind != kind)
                    continue;
                if (item >= cand.first_item && item < cand.first_item + cand.item_count) {
                    picker_weapon_ = i;
                    inv.loadout[static_cast<size_t>(inv.side)][static_cast<size_t>(picker_slot_)].weapon =
                        static_cast<std::int16_t>(i);
                    break;
                }
            }
        }
        const inv::Weapon& chosen = weapon(picker_weapon_);
        if (item >= chosen.first_item && item < chosen.first_item + chosen.item_count)
            inv.loadout[static_cast<size_t>(inv.side)][static_cast<size_t>(picker_slot_)].item =
                static_cast<std::int16_t>(item);
    }
    picker_anim_ = 0.0f;
}

// A saved preset stores catalogue indices. Rebuilding the catalogue moves those
// rows, and a pistol index left in the agent slot draws a gun where the agent
// stands and covers the knife / gloves / medal row under it.
void repair_loadout(InventorySettings& inv)
{
    for (int side = 0; side < 2; ++side) {
        for (int slot = 0; slot < inv::slot_count; ++slot) {
            if(inv::host&&!inv::host_slot(slot))continue;
            auto& equipped = inv.loadout[static_cast<size_t>(side)][static_cast<size_t>(slot)];
            const Kind kind = slot_kind(slot);
            const bool weapon_ok = equipped.weapon >= 0 && equipped.weapon < weapon_count() &&
                                   weapon(equipped.weapon).kind == kind;
            bool item_ok = false;
            if (weapon_ok) {
                const inv::Weapon& w = weapon(equipped.weapon);
                item_ok = equipped.item >= w.first_item && equipped.item < w.first_item + w.item_count;
            }
            if (!weapon_ok || !item_ok)
                equipped = inv::host ? inv::host->defaults[slot] : inv::default_loadouts[side][slot];
        }
    }
}

void Menu::tab_inventory(const ImVec2& o, Config& c)
{
    InventorySettings& inv = c.inventory;
    repair_loadout(inv);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Fonts& f = fonts();
    const bool picker = picker_slot_ >= 0;
    if (picker)
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    auto& loadout = inv.loadout[static_cast<size_t>(inv.side)];

    // CT / T switch: the active side is a labelled pill, the other one just its emblem.
    if(inv::host){
        const ImVec2 pos=at(o,150,54);
        ui::toggle("##host_cosmetics",&inv.apply,pos);
        draw::text(dl,f.medium,font_size::text,at(o,179,56),col::card_name,"Skin changer");
    }else{
        const char* labels[2] = { "CT Loadout", "T Loadout" };
        const float icon_w = px(20.0f), gap = px(6.0f);
        const float label_w = draw::text_width(f.medium, font_size::text, labels[inv.side]);
        const float pill_w = snap(px(25.0f) + label_w + px(9.0f));
        const float total = pill_w + gap + icon_w;
        float x = snap(o.x + px(k_switch_cx) - total * 0.5f);
        const float h = snap(px(k_switch_h));
        const float y0 = snap(o.y + px(k_switch_cy) - h * 0.5f);
        for (int side = 0; side < 2; ++side) {
            const bool active = side == inv.side;
            const float w = active ? pill_w : icon_w;
            const ImVec2 min(x, y0), max(x + w, y0 + h);
            ImGui::PushID(side);
            const ImGuiID id = ImGui::GetID("##side");
            const ui::ItemState s = ui::item_behavior(id, min, max);
            if (s.pressed && !active)
                inv.side = side;
            const float hv = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
            ImGui::PopID();
            const ImU32 accent = side == 0 ? col::ct_color : col::t_color;
            if (active)
                dl->AddRectFilled(min, max, draw::lerp_color(col::card_bg, accent, 0.10f + hv * 0.04f), px(5.0f));
            else if (hv > 0.0f)
                dl->AddRectFilled(min, max, draw::with_alpha(col::card_bg, hv), px(5.0f));
            const float cy = y0 + h * 0.5f;
            side_emblem(dl, ImVec2(x + px(active ? 11.5f : 10.0f), cy), side, active ? 1.0f : 0.75f + 0.25f * hv);
            if (active)
                draw::text(dl, f.medium, font_size::text,
                           ImVec2(x + px(23.0f), cy - draw::cap_height(f.medium, font_size::text) * 0.5f), accent,
                           labels[side]);
            x += w + gap;
        }
    }

    // Agent
    if(inv::host){
        inv::host->character(dl,at(o,k_body_x0,k_body_y0),at(o,k_body_x1,k_body_y1));
    }else{
        const inv::Equip& agent = loadout[inv::slot_agent];
        const inv::Item& it = item(agent.item);
        ImVec2 a_min = at(o, k_body_x0, k_body_y0);
        ImVec2 a_max = at(o, k_body_x1, k_body_y1);
        // A little larger, and three pixels lower than the reference box.
        const float grow = px(6.0f);
        a_min.x -= grow;
        a_max.x += grow;
        a_min.y -= grow * 0.25f;
        a_max.y += grow;
        a_min.y += 3.0f;
        a_max.y += 3.0f;

        const char* agent_model = agents::model_for_name(it.name);
        if (!agent_model && it.name && std::strcmp(it.name, "Default CT Agent") == 0)
            agent_model = "agents/models/ctm_sas/ctm_sas.vmdl_c";
        if (!agent_model && it.name && std::strcmp(it.name, "Default T Agent") == 0)
            agent_model = "agents/models/tm_phoenix/tm_phoenix.vmdl_c";
        bool drew_model = false;
        if (agent_model) {
            esp_model::set_model(agent_model);
            const auto frame = esp_model::render(a_max.x - a_min.x, a_max.y - a_min.y, agent_orbit_, false);
            if (frame.ready) {
                const ui::ItemState orbit = ui::item_behavior(ImGui::GetID("##agent_orbit"), a_min, a_max);
                if (orbit.held)
                    agent_orbit_ += ImGui::GetIO().MouseDelta.x * 0.6f;
                dl->AddImage(frame.texture, a_min, a_max);
                drew_model = true;
            }
        }
        if (!drew_model) {
            const int body = it.body >= 0 ? it.body : it.image;
            images::draw(dl, body, a_min, a_max, IM_COL32_WHITE, true);
        }
    }

    // Agent, medal, music kit / knife, gloves, zeus
    constexpr int bottom_slots[6] = { inv::slot_agent, inv::slot_medal, inv::slot_music_kit,
                                      inv::slot_knife, inv::slot_gloves, inv::slot_zeus };
    for (int i = 0; i < 6; ++i) {
        const int slot = bottom_slots[i];
        if(inv::host&&!inv::host_slot(slot))continue;
        const inv::Equip& e = loadout[static_cast<size_t>(slot)];
        const int column=inv::host?i-3:i%3;const int row=inv::host?0:i/3;
        const ImVec2 min = at(o, k_slot_x[column], k_slot_y[row]);
        const ImVec2 max = at(o, k_slot_x[column] + k_slot_w, k_slot_y[row] + k_slot_h);
        ImGui::PushID(slot);
        if (card(dl, "##slot", min, max, item(e.item).image, rarity_color(e.item), ImVec2(px(4.0f), px(4.0f))))
            open_picker(slot);
        ImGui::PopID();
        if(inv::host)draw::text(dl,f.medium,font_size::card,at(o,k_slot_x[column],k_slot_y[row]+k_slot_h+6),col::card_name,weapon(e.weapon).name);
    }

    // Weapons: pistols, mid-tier and rifles in three columns of five.
    const char* columns[3] = { "Pistols", "Mid-Tier", "Rifles" };
    for (int col_i = 0; col_i < 3; ++col_i) {
        const float w = draw::text_width(f.medium, font_size::text, columns[col_i]);
        draw::text(dl, f.medium, font_size::text, ImVec2(o.x + px(k_column_cx[col_i]) - w * 0.5f, o.y + px(k_column_cap_y)),
                   col::card_name, columns[col_i]);
        for (int row = 0; row < 5; ++row) {
            const int slot = col_i * 5 + row;
            const inv::Equip& e = loadout[static_cast<size_t>(slot)];
            const float y = k_card_y0 + k_card_pitch * static_cast<float>(row);
            const ImVec2 min = at(o, k_card_x[col_i], y);
            const ImVec2 max = at(o, k_card_x[col_i] + k_card_w, y + k_card_h);
            ImGui::PushID(slot);
            if (card(dl, "##card", min, max, item(e.item).image, rarity_color(e.item), ImVec2(px(10.0f), px(9.0f))))
                open_picker(slot);
            ImGui::PopID();
            if(inv::host)draw::text(dl,f.medium,font_size::card,at(o,k_card_x[col_i]+5,y+5),col::card_name,weapon(e.weapon).name);
        }
    }
    if (picker)
        ImGui::PopItemFlag();

    if (picker_slot_ >= 0)
        draw_picker(o, inv);
}

void Menu::draw_picker(const ImVec2& o, InventorySettings& inv)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const Fonts& f = fonts();
    const ImGuiIO& io = ImGui::GetIO();
    const int v0 = dl->VtxBuffer.Size;
    picker_anim_ = ImMin(1.0f, picker_anim_ + io.DeltaTime * 7.0f);
    ImGui::PushID("##picker");

    const int slot = picker_slot_;
    const Kind kind = slot_kind(slot);
    inv::Equip& equipped = inv.loadout[static_cast<size_t>(inv.side)][static_cast<size_t>(slot)];
    const std::vector<int> choices = slot_weapons(slot, inv.side);
    if (std::find(choices.begin(), choices.end(), picker_weapon_) == choices.end())
        picker_weapon_ = equipped.weapon;

    // Backdrop: the loadout stays faintly visible behind the picker.
    const ImVec2 main_min = at(o, layout::main_x, 0.0f);
    const ImVec2 main_max = at(o, layout::width, layout::height);
    dl->AddRectFilled(main_min, main_max, col::overlay_bg, px(layout::main_rounding));
    draw::border(dl, main_min, main_max, col::main_border, px(layout::main_rounding), draw::hairline());

    bool close = false;
    if (inspect_fullscreen_ && ImGui::IsKeyPressed(ImGuiKey_Escape))
        inspect_fullscreen_ = false;
    else
        close = ImGui::IsKeyPressed(ImGuiKey_Escape);
    {
        const ImVec2 c(o.x + px(k_close_x), o.y + px(k_close_y));
        const float hit = px(9.0f);
        const ImGuiID id = ImGui::GetID("##close");
        const ui::ItemState s = ui::item_behavior(id, ImVec2(c.x - hit, c.y - hit), ImVec2(c.x + hit, c.y + hit));
        const float h = ui::animate(id + 1, s.hovered ? 1.0f : 0.0f, 16.0f);
        draw::icon(dl, f.icons, 12.5f, c, draw::lerp_color(col::window_value, col::tab_text_active, h), icon::xmark);
        close |= s.pressed;
    }

    // Title: collection icon, name and collection, then the rarity line.
    const inv::Item& eq_item = item(equipped.item);
    const std::string title = title_of(equipped);
    const bool has_sub = eq_item.collection != nullptr;
    {
        const float title_w = draw::text_width(f.semibold, font_size::picker_title, title.c_str());
        const float sub_w = has_sub ? draw::text_width(f.medium, font_size::text, eq_item.collection) : 0.0f;
        const bool has_icon = eq_item.collection_icon >= 0;
        const float icon_w = has_icon ? px(40.0f) : 0.0f;
        const float block = icon_w + ImMax(title_w, sub_w);
        const float x = snap(o.x + px(k_center_x) - block * 0.5f);
        if (has_icon)
            images::draw(dl, eq_item.collection_icon, ImVec2(x, o.y + px(9.0f)), ImVec2(x + px(32.0f), o.y + px(43.0f)));
        const float tx = x + icon_w;
        draw::text(dl, f.semibold, font_size::picker_title, ImVec2(tx, o.y + px(has_sub ? 16.5f : 14.0f)),
                   col::tab_text_active, title.c_str());
        if (has_sub)
            draw::text(dl, f.medium, font_size::text, ImVec2(tx, o.y + px(35.5f)), col::window_label, eq_item.collection);
        const float bar_y = has_sub ? 52.0f : 39.5f;
        const ImVec2 bmin = at(o, k_center_x - k_title_bar_w * 0.5f, bar_y);
        dl->AddRectFilled(bmin, ImVec2(at(o, k_center_x + k_title_bar_w * 0.5f, 0.0f).x, bmin.y + snap(px(k_bar_h))),
                          rarity_color(equipped.item), px(1.0f));
    }

    // Preview: the real model the game loads, with the selected finish painted on it.
    {
        const float top = has_sub ? 64.0f : 50.0f;
        ImVec2 pmin = at(o, k_preview_x0, top);
        ImVec2 pmax = at(o, k_preview_x1, k_preview_y1);

        // Anything the catalogue has a world model for -- weapons, knives, gloves and the
        // agents -- is drawn in 3D; medals and music kits have no model and keep the
        // picture they have always used.
        const char* model = inv::host ? nullptr : kind == Kind::agent ? agents::model_for_name(eq_item.name)
                                                : models::weapon(picker_weapon_);
        const models::Finish* finish = inv::host || kind == Kind::agent ? nullptr : models::finish(equipped.item);
        bool drawn = false;
        if (inspect_fullscreen_) {
            pmin = ImVec2(px(24.0f), px(36.0f));
            pmax = ImVec2(io.DisplaySize.x - px(24.0f), io.DisplaySize.y - px(24.0f));
            ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0.0f, 0.0f), io.DisplaySize,
                                                         IM_COL32(6, 7, 10, 245));
        }
        // Knives and guns are models. A missing finish still shows the mesh; the
        // flat picture is only the fallback when the model itself will not load.
        if (model) {
            model_preview::set_model(model);
            if (finish)
                model_preview::set_finish(finish->material, finish->color, finish->normal,
                                          finish->roughness, finish->ao, finish->mask,
                                          finish->palette);
            else
                model_preview::clear_finish();
            const SlotFinish& look = inv.finish[static_cast<size_t>(inv.side)][static_cast<size_t>(slot)];
            model_preview::set_wear(finish ? look.wear : 0.0f);
            const int material = ImClamp(look.model_material, 0, IM_ARRAYSIZE(inv::k_model_material_type) - 1);
            model_preview::set_chams(kind == Kind::agent ? -1 : inv::k_model_material_type[material],
                                     look.model_color, look.model_exponent, look.model_overlay);

            const model_preview::Frame frame =
                model_preview::render(pmax.x - pmin.x, pmax.y - pmin.y);

            // Claim the rectangle whether or not a frame is ready yet: an item has to be
            // registered here or ImGui treats the preview as empty menu space and a drag
            // ends up moving the window instead of the model. item_behavior adds the item
            // without advancing the layout cursor, so it cannot grow the window content and
            // shift every other hitbox the way an InvisibleButton here did.
            const ui::ItemState inspect = ui::item_behavior(ImGui::GetID("##inspect"), pmin, pmax);
            // Gloves play the inspect clip on their own. Dragging would spin them out of it.
            if (inspect.held && kind != Kind::gloves)
                model_preview::orbit(io.MouseDelta.x, io.MouseDelta.y);
            if (kind != Kind::gloves && ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(pmin, pmax) && io.MouseWheel != 0.0f)
                model_preview::zoom(io.MouseWheel);

            if (frame.ready) {
                ImDrawList* target = inspect_fullscreen_ ? ImGui::GetForegroundDrawList() : dl;
                target->AddImage(frame.texture, pmin, pmax);
                drawn = true;
            }
        }
        if (!drawn)
            inventory_image(inspect_fullscreen_?ImGui::GetForegroundDrawList():dl, eq_item.body >= 0 ? eq_item.body : eq_item.image, pmin, pmax,
                         IM_COL32_WHITE, true);
    }

    // Toolbar
    {
        const float cy = o.y + px(k_tool_y);
        float x = layout::main_x + 15.0f;
        const auto next = [&](const char* id, const char* glyph, bool active = false) {
            const bool pressed = tool_icon(dl, id, ImVec2(o.x + px(x), cy), glyph, active);
            x += 23.0f;
            return pressed;
        };
        SlotFinish& rolled = inv.finish[static_cast<size_t>(inv.side)][static_cast<size_t>(slot)];
        if (!inv::host&&(is_gun(kind) || kind == Kind::knife || kind == Kind::gloves)) {
            if (next("##wear", icon::poll, ImGui::IsPopupOpen(ImGui::GetID("##finish_popup"), ImGuiPopupFlags_None)))
                ui::popup::open("##finish_popup");
            if (kind != Kind::gloves)
                next("##zoom", icon::zoom_in);
        }
        if (ui::popup::begin("##finish_popup", "Options")) {
            draw_finish_options(rolled, kind);
            ui::popup::end();
        }
        if (next("##expand", icon::expand, inspect_fullscreen_))
            inspect_fullscreen_ = !inspect_fullscreen_;
        // The check applies the loadout to the game; it stays highlighted while the
        // skinchanger is on so the picker's state is readable at a glance.
        if (next("##apply", icon::check_double, inv.apply))
            inv.apply = !inv.apply;

        if (kind == Kind::gloves) {
            // Glove type selector.
            const char* name = weapon(picker_weapon_).name;
            const float tw = draw::text_width(f.medium, font_size::text, name);
            const ImVec2 min(o.x + px(x - 8.0f), cy - px(10.0f));
            const ImVec2 max(o.x + px(x + 18.0f) + tw, cy + px(10.0f));
            const ImGuiID id = ImGui::GetID("##glove_type");
            const ui::ItemState s = ui::item_behavior(id, min, max);
            if (s.pressed)
                ImGui::OpenPopup(id);
            const float h = ui::animate(id + 1, (s.hovered || ImGui::IsPopupOpen(id, ImGuiPopupFlags_None)) ? 1.0f : 0.0f,
                                        16.0f);
            const ImU32 c = draw::lerp_color(col::window_value, col::tab_text_active, h);
            draw::icon(dl, f.icons, 11.5f, ImVec2(o.x + px(x), cy), c, icon::hand);
            draw::text(dl, f.medium, font_size::text,
                       ImVec2(o.x + px(x + 12.0f), cy - draw::cap_height(f.medium, font_size::text) * 0.5f), c, name);
            if (ui::begin_popup_id(id, min, max, px(170.0f))) {
                for (int w : choices) {
                    ImGui::PushID(w);
                    if (ui::dropdown_item(weapon(w).name, w == picker_weapon_, false)) {
                        picker_weapon_ = w;
                        grid_scroll_ = 0.0f;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                ui::end_popup();
            }
        }

        if (!inv::host&&is_gun(kind) && kind != Kind::zeus) {
            // Sticker and charm slots.
            const ImVec2 pill_min = at(o, 557.0f, 188.5f), pill_max = at(o, 627.0f, 206.5f);
            const ImGuiID pid = ImGui::GetID("##stickers");
            const ui::ItemState ps = ui::item_behavior(pid, pill_min, pill_max);
            const float ph = ui::animate(pid + 1, ps.hovered ? 1.0f : 0.0f, 16.0f);
            dl->AddRectFilled(pill_min, pill_max, draw::lerp_color(col::chip_bg_selected, col::chip_bg_hovered, ph),
                              px(5.0f));
            const float pcy = (pill_min.y + pill_max.y) * 0.5f;
            draw::icon(dl, f.icons, 10.5f, ImVec2(pill_min.x + px(10.0f), pcy), col::window_value, icon::sticker);
            draw::text(dl, f.medium, font_size::text,
                       ImVec2(pill_min.x + px(19.5f), pcy - draw::cap_height(f.medium, font_size::text) * 0.5f),
                       col::picker_accent, "Stickers");
            const ImVec2 knob(o.x + px(546.0f), pcy);
            dl->AddCircleFilled(knob, px(7.2f), IM_COL32(204, 207, 216, 255), 32);
            draw::any_icon(dl, knob, IM_COL32(60, 62, 72, 255), icon::sparkles, 8.5f);

            const ImVec2 row_min = at(o, 459.0f, 212.0f), row_max = at(o, 627.0f, 242.0f);
            dl->AddRectFilled(row_min, row_max, col::chip_bg, px(6.0f));
            const float slot_w = (row_max.x - row_min.x) / 5.0f;
            for (int i = 0; i < 5; ++i) {
                const ImVec2 smin(row_min.x + slot_w * static_cast<float>(i) + px(3.0f), row_min.y + px(3.0f));
                const ImVec2 smax(row_min.x + slot_w * static_cast<float>(i + 1) - px(3.0f), row_max.y - px(3.0f));
                if (i < 3) {
                    images::draw(dl, inv::sticker_images[i], smin, smax, IM_COL32_WHITE, true);
                } else {
                    const ImVec2 c((smin.x + smax.x) * 0.5f, (smin.y + smax.y) * 0.5f);
                    const float r = ImMin(smax.x - smin.x, smax.y - smin.y) * 0.5f;
                    dl->AddCircleFilled(c, r, IM_COL32(40, 42, 52, 255), 32);
                    dl->PathArcTo(c, r - px(1.0f), IM_PI * 0.9f, IM_PI * 1.6f, 12);
                    dl->PathStroke(IM_COL32(92, 95, 108, 255), px(1.2f));
                    dl->AddCircle(c, r, IM_COL32(58, 60, 72, 255), 32, px(1.0f));
                }
            }
            images::draw(dl, inv::charm_image, at(o, 433.0f, 213.0f), at(o, 455.0f, 241.0f), IM_COL32_WHITE, true);
        }
    }

    // Filters: weapon, rarity and search.
    const float fh = snap(px(k_filter_h));
    const float fy = snap(o.y + px(k_filter_cy) - fh * 0.5f);
    float fx = layout::main_x + 9.0f;
    const bool weapon_combo = (is_gun(kind) && kind != Kind::zeus) || kind == Kind::knife;
    if (weapon_combo) {
        std::vector<const char*> names;
        int current = 0;
        for (size_t i = 0; i < choices.size(); ++i) {
            names.push_back(weapon(choices[i]).name);
            if (choices[i] == picker_weapon_)
                current = static_cast<int>(i);
        }
        const ImVec2 min(o.x + snap(px(fx)), fy);
        if (ui::combo_box("##weapon", min, ImVec2(o.x + snap(px(fx + 92.0f)), fy + fh), names, &current)) {
            picker_weapon_ = choices[static_cast<size_t>(current)];
            grid_scroll_ = 0.0f;
        }
        fx += 97.0f;
    }
    if (ui::combo_box("##rarity", ImVec2(o.x + snap(px(fx)), fy), ImVec2(o.x + snap(px(fx + 72.0f)), fy + fh),
                      inv::host?inv::host->rarity_filters:std::span<const char* const>(k_rarity_filters), &rarity_filter_))
        grid_scroll_ = 0.0f;
    fx += 77.0f;
    {
        const ImVec2 min(o.x + snap(px(fx)), fy), max = ImVec2(at(o, k_grid_x1, 0.0f).x, fy + fh);
        dl->AddRectFilled(min, max, col::combo_bg, px(layout::multi_rounding));
        const float cy = fy + fh * 0.5f;
        draw::icon(dl, f.icons, 10.5f, ImVec2(min.x + px(10.0f), cy), col::window_value, icon::search);
        const float font_px = draw::font_px(font_size::combo);
        ImGui::PushFont(f.medium, font_px / ui_scale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, std::floor((fh - font_px) * 0.5f)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, col::combo_text);
        ImGui::SetCursorScreenPos(ImVec2(min.x + px(20.0f), fy));
        ImGui::SetNextItemWidth(max.x - min.x - px(26.0f));
        if (ImGui::InputText("##search", search_, sizeof(search_)))
            grid_scroll_ = 0.0f;
        const bool active = ImGui::IsItemActive();
        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
        ImGui::PopFont();
        if (!search_[0] && !active)
            draw::text(dl, f.medium, font_size::combo,
                       ImVec2(min.x + px(20.0f), cy - draw::cap_height(f.medium, font_size::combo) * 0.5f),
                       col::window_value, "Search");
    }

    // Grid of skins.
    {
        const inv::Weapon& w = weapon(picker_weapon_);
        std::vector<int> list;
        for (int i = w.first_item; i < w.first_item + w.item_count; ++i) {
            if (rarity_filter_ > 0 && (inv::host ? item(i).rarity!=rarity_filter_-1 : rarity_color(i)!=k_rarity_filter_colors[rarity_filter_]))
                continue;
            if (!contains_nocase(item(i).name, search_))
                continue;
            list.push_back(i);
        }

        const ImVec2 gmin = at(o, k_grid_x0, k_grid_y0), gmax = at(o, k_grid_x1, k_grid_y1);
        const float gap = px(k_grid_gap);
        const float cw = std::floor(((gmax.x - gmin.x) - gap * (k_grid_columns - 1)) / k_grid_columns);
        const float pitch = px(k_grid_pitch);
        const int rows = (static_cast<int>(list.size()) + k_grid_columns - 1) / k_grid_columns;
        const float content_h = static_cast<float>(rows) * pitch - (pitch - px(k_grid_card_h + 16.0f));
        const float max_scroll = ImMax(0.0f, content_h - (gmax.y - gmin.y));

        const bool hovered = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(gmin, ImVec2(gmax.x + px(8.0f), gmax.y));
        if (hovered && !io.KeyCtrl && io.MouseWheel != 0.0f)
            grid_scroll_ -= io.MouseWheel * px(60.0f);

        // Scrollbar
        const ImVec2 track_min(gmax.x + px(3.0f), gmin.y), track_max(gmax.x + px(5.5f), gmax.y);
        if (max_scroll > 0.0f) {
            const float track_h = track_max.y - track_min.y;
            const float thumb_h = ImMax(px(24.0f), track_h * (gmax.y - gmin.y) / content_h);
            const ImGuiID sid = ImGui::GetID("##scrollbar");
            const ui::ItemState ss = ui::item_behavior(sid, ImVec2(track_min.x - px(3.0f), track_min.y),
                                                       ImVec2(track_max.x + px(3.0f), track_max.y));
            if (ss.held)
                grid_scroll_ = (io.MousePos.y - track_min.y - thumb_h * 0.5f) / (track_h - thumb_h) * max_scroll;
            grid_scroll_ = ImClamp(grid_scroll_, 0.0f, max_scroll);
            const float t = grid_scroll_ / max_scroll;
            const float ty = track_min.y + (track_h - thumb_h) * t;
            dl->AddRectFilled(track_min, track_max, IM_COL32(24, 25, 33, 255), px(2.0f));
            dl->AddRectFilled(ImVec2(track_min.x, ty), ImVec2(track_max.x, ty + thumb_h),
                              ss.held || ss.hovered ? IM_COL32(120, 123, 136, 255) : IM_COL32(78, 80, 92, 255), px(2.0f));
        } else {
            grid_scroll_ = 0.0f;
        }
        const float scroll = ui::animate(ImGui::GetID("##scroll_anim"), grid_scroll_, 20.0f);

        ImGui::PushClipRect(gmin, ImVec2(gmax.x, gmax.y), true);
        if (list.empty()) {
            const char* empty = "No skins match the filters";
            const float tw = draw::text_width(f.medium, font_size::text, empty);
            draw::text(dl, f.medium, font_size::text, ImVec2(snap((gmin.x + gmax.x - tw) * 0.5f), gmin.y + px(40.0f)),
                       col::window_value, empty);
        }
        for (size_t i = 0; i < list.size(); ++i) {
            const int idx = list[i];
            const int c = static_cast<int>(i) % k_grid_columns, r = static_cast<int>(i) / k_grid_columns;
            const float x = gmin.x + static_cast<float>(c) * (cw + gap);
            const float y = snap(gmin.y + static_cast<float>(r) * pitch - scroll);
            if (y > gmax.y || y + pitch < gmin.y)
                continue;
            const ImVec2 min(snap(x), y), max(snap(x + cw), y + snap(px(k_grid_card_h)));
            const bool selected = equipped.weapon == picker_weapon_ && equipped.item == idx;
            ImGui::PushID(idx);
            if (card(dl, "##skin", min, max, item(idx).image, rarity_color(idx), ImVec2(px(9.0f), px(8.0f)), selected)){
                equipped = { static_cast<std::int16_t>(picker_weapon_), static_cast<std::int16_t>(idx) };
                if(inv::host&&inv::host->selected)inv::host->selected(picker_weapon_,idx);
            }
            ImGui::PopID();
            const char* name = item(idx).name;
            const ImVec2 pen(min.x + px(1.0f), max.y + px(6.0f));
            if (draw::text_width(f.medium, font_size::card + 1.0f, name) > cw)
                draw::text_fade_right(dl, f.medium, font_size::card + 1.0f, pen, col::card_name, name, max.x - px(18.0f),
                                      max.x);
            else
                draw::text(dl, f.medium, font_size::card + 1.0f, pen, col::card_name, name);
        }
        ImGui::PopClipRect();
    }

    ImGui::PopID();
    if (picker_anim_ < 1.0f) {
        const float e = picker_anim_ * (2.0f - picker_anim_);
        for (int i = v0; i < dl->VtxBuffer.Size; ++i)
            dl->VtxBuffer[i].col = draw::with_alpha(dl->VtxBuffer[i].col, e);
    }
    if (close) {
        picker_slot_ = -1;
        inspect_fullscreen_ = false;
    }
}

} // namespace nl
