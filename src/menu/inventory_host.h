// Menu source watermark: @use_sberbank
#pragma once
#include <span>
#include "menu/inventory_catalog.h"

namespace nl::inv {
// Optional game catalog for the original loadout and picker renderer.
struct Host {
    std::span<const Weapon> weapons;
    std::span<const Item> items;
    std::span<const Rarity> rarities;
    std::span<const char* const> rarity_filters;
    const Equip* defaults = nullptr;
    void (*image)(ImDrawList*, int, const ImVec2&, const ImVec2&, ImU32) = nullptr;
    void (*character)(ImDrawList*, const ImVec2&, const ImVec2&) = nullptr;
    void (*selected)(int, int) = nullptr;
};
inline const Host* host = nullptr;
inline bool host_slot(int slot) { return slot < slot_agent || slot == slot_knife || slot == slot_gloves; }
}
