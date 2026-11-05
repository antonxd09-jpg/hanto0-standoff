#pragma once
// HUD color for the 1.0.0 ARM64 profile (the "смена цвета худа" request).
//
// The game HUD (health bar, ammo, weapon panel, buttons) is Unity uGUI: its
// widgets are UnityEngine.UI.Image components whose displayed color is
// Image.color multiplied into the sprite. Recoloring is therefore one managed
// call per widget:
//
//   Type.GetType("UnityEngine.UI.Image, UnityEngine.UI")      (reflection RVA)
//     -> Object.FindObjectsOfTypeAll(type)                    (reflection RVA)
//       -> Graphic.get_color / Graphic.set_color (il2cpp metadata, the
//          UnityEngine.UI.dll image is opened by the meta resolver)
//
// Every widget's ORIGINAL color is saved the first time it is seen, so turning
// the feature off restores the HUD exactly. The applied color is
// original * tint (a multiply), which keeps shading/alpha relationships -
// white HUD parts take the chosen color, black stays black, red damage
// flashes get tinted instead of overwritten.
//
// v4: the save table grew from 256 to 2048 slots (a loaded buy menu pushed
// the whole scene past 256 uGUI Images, and anything past the cap silently
// kept its original color - the user saw the buy HUD stay default), the scan
// runs every 0.8 s instead of 2 s so freshly-opened menus are tinted before
// they can be used, and Text + RawImage widgets join Image so the whole HUD
// (numbers, icons, buy menu labels) recolors, not just the flat panels.
//
// Threading: scan + calls happen inside onPlayerUpdate (Unity main thread,
// ake update hook); the GUI only writes the word-sized settings.

#include "features/visuals/esp_runtime.hpp"
#include "features/visuals/worldcolor_runtime.hpp"
#include "game/il2cpp_meta.hpp"
#include "game/offsets.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

namespace astra::hudcolor {

using Color4 = astra::worldcolor::Color4;

struct Settings {
    bool enabled{false};
    bool includeText{true};
    Color4 tint{0.35f, 0.80f, 1.00f, 1.0f};
};

// GUI (render thread) writes, main thread reads; word-sized fields only.
inline Settings g_settings;

inline std::atomic<int> g_status{0};       // 0 off, 1 waiting, 2 unavailable, 3 active
inline std::atomic<int> g_tinted{0};       // images carrying the tint
inline std::atomic<std::uint64_t> g_scans{0};

namespace detail {

namespace ed = astra::esp::detail;
using namespace astra::offsets;
using Clock = std::chrono::steady_clock;

using TypeGetTypeFn = void* (*)(void* name, const void* method);
using FindObjectsOfTypeAllFn = void* (*)(void* type, const void* method);
using GraphicGetColorFn = Color4 (*)(void* graphic, const void* method);
using GraphicSetColorFn = void (*)(void* graphic, Color4 color, const void* method);
using StringNewFn = void* (*)(const char*);

// .text gate for direct-RVA resolution (mirrors chams_runtime::textPointer).
inline bool inText(std::uintptr_t value) {
    const std::uintptr_t base = ed::g_runtime.unityBase;
    return base && value >= base + offsets::elf::kTextBegin && value < base + offsets::elf::kTextEnd;
}

template <typename T>
inline T resolve(std::uintptr_t rva) {
    const std::uintptr_t base = ed::g_runtime.unityBase;
    if (!rva || !base || !inText(base + rva)) return nullptr;
    const auto target = reinterpret_cast<T>(base + rva);
    return ed::readable(reinterpret_cast<const void*>(target), 16) ? target : nullptr;
}

inline struct Functions {
    TypeGetTypeFn typeGetType{nullptr};
    FindObjectsOfTypeAllFn findObjectsOfTypeAll{nullptr};
    GraphicGetColorFn getColor{nullptr};
    GraphicSetColorFn setColor{nullptr};
} g_fn;


inline bool liveUnityObject(void* object){
 if(!object)return false;
 std::uintptr_t cached=0;
 return ed::readValue(reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(object)+0x10),cached)&&
        cached&&ed::readable(reinterpret_cast<const void*>(cached),32);
}

inline bool g_initialized{false};
inline Clock::time_point g_lastInitAttempt{};
inline Clock::time_point g_lastScan{};
inline void* g_imageType{nullptr};
inline void* g_rawImageType{nullptr};
inline void* g_textType{nullptr};

// Saved original colors, keyed by the widget pointer. The scan re-runs every
// 0.8 s while enabled; a destroyed widget's slot can be recycled without ever
// touching freed memory (liveness is checked before every restore).
struct SavedColor {
    void* image{nullptr};
    Color4 color{};
};
inline constexpr std::size_t kMaxSaved = 2048;
inline std::array<SavedColor, kMaxSaved> g_saved{};
inline std::size_t g_savedCount{0};

inline bool resolveFunctions() {
    if (g_initialized) return true;
    const auto now = Clock::now();
    if (now - g_lastInitAttempt < std::chrono::seconds(2)) return false;
    g_lastInitAttempt = now;
    if (!ed::g_runtime.ready || !ed::g_runtime.unityBase) return false;

    astra::il2cpp_meta::init(ed::g_runtime.unityBase);
    Functions fn{};
    namespace vr = visuals_rva;

    // RVAs first, metadata second - same pattern as every other native.
    fn.typeGetType = nullptr;
    if (!fn.typeGetType) {
        if (void* typeClass = astra::il2cpp_meta::unityClass("System", "Type")) {
            fn.typeGetType = reinterpret_cast<TypeGetTypeFn>(
                astra::il2cpp_meta::method(typeClass, "GetType", 1, nullptr));
        }
    }
    fn.findObjectsOfTypeAll = nullptr;
    if (!fn.findObjectsOfTypeAll) {
        if (void* objectClass = astra::il2cpp_meta::unityClass("UnityEngine", "Object")) {
            fn.findObjectsOfTypeAll = reinterpret_cast<FindObjectsOfTypeAllFn>(
                astra::il2cpp_meta::method(objectClass, "FindObjectsOfTypeAll", 1, nullptr));
        }
    }
    // Graphic owns color; Image inherits it. UnityEngine.UI.dll is opened by
    // the metadata resolver (see il2cpp_meta.hpp).
    if (void* graphicClass = astra::il2cpp_meta::unityClass("UnityEngine.UI", "Graphic")) {
        fn.getColor = reinterpret_cast<GraphicGetColorFn>(
            astra::il2cpp_meta::method(graphicClass, "get_color", 0, nullptr));
        fn.setColor = reinterpret_cast<GraphicSetColorFn>(
            astra::il2cpp_meta::method(graphicClass, "set_color", 1, nullptr));
    }
    g_fn = fn;
    if (!fn.typeGetType || !fn.findObjectsOfTypeAll || !fn.getColor || !fn.setColor) {
        g_status.store(2, std::memory_order_relaxed);
        __android_log_print(ANDROID_LOG_WARN, astra::esp::kLogTag,
                            "hudcolor init: missing type=%p findAll=%p getColor=%p setColor=%p",
                            reinterpret_cast<void*>(fn.typeGetType),
                            reinterpret_cast<void*>(fn.findObjectsOfTypeAll),
                            reinterpret_cast<void*>(fn.getColor),
                            reinterpret_cast<void*>(fn.setColor));
        return false;
    }
    g_initialized = true;
    __android_log_print(ANDROID_LOG_INFO, astra::esp::kLogTag,
                        "hudcolor ready: Graphic.get/set_color resolved");
    return true;
}

inline SavedColor* findSaved(void* image) {
    for (std::size_t i = 0; i < g_savedCount; ++i) {
        if (g_saved[i].image == image) return &g_saved[i];
    }
    return nullptr;
}

inline void restoreAll() {
    for (std::size_t i = 0; i < g_savedCount; ++i) {
        SavedColor& entry = g_saved[i];
        if (liveUnityObject(entry.image)) {
            g_fn.setColor(entry.image, entry.color, nullptr);
        }
    }
    g_savedCount = 0;
    g_tinted.store(0, std::memory_order_relaxed);
}

inline void* resolveUiType(const char* name) {
    StringNewFn stringNew = nullptr;
    if (void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL)) {
        stringNew = reinterpret_cast<StringNewFn>(dlsym(unity, "il2cpp_string_new"));
    }
    if (!stringNew) stringNew = resolve<StringNewFn>(offsets::il2cpp_api::kStringNew);
    if (!stringNew) return nullptr;
    void* handle = stringNew(name);
    return handle ? g_fn.typeGetType(handle, nullptr) : nullptr;
}

inline void scanAndApply() {
    // il2cpp_string_new rides the chams/worldcolor resolver chain.
    if (!g_imageType) g_imageType = resolveUiType("UnityEngine.UI.Image, UnityEngine.UI");
    if (!g_rawImageType) g_rawImageType = resolveUiType("UnityEngine.UI.RawImage, UnityEngine.UI");
    if (!g_textType) g_textType = resolveUiType("UnityEngine.UI.Text, UnityEngine.UI");

    void* types[3] = {g_imageType, g_rawImageType, g_textType};
    const bool wantText = g_settings.includeText;
    const Color4& tint = g_settings.tint;
    int applied = 0;
    bool scanned = false;

    for (void* type : types) {
        if (!type) continue;
        if (type == g_textType && !wantText) continue;
        void* array = g_fn.findObjectsOfTypeAll(type, nullptr);
        if (!array || !ed::readable(array, 32)) continue;
        scanned = true;
        std::uintptr_t length = 0;
        if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(array) +
                                                   managed::kArrayLength),
                           length)) {
            continue;
        }
        if (length > 4096) length = 4096;  // paranoia cap per type

        const std::uintptr_t data = reinterpret_cast<std::uintptr_t>(array) + managed::kArrayData;
        for (std::uintptr_t i = 0; i < length; ++i) {
            void* image = nullptr;
            if (!ed::readValue(reinterpret_cast<void*>(data + i * 8), image) || !image) continue;
            if (!liveUnityObject(image)) continue;

            SavedColor* entry = findSaved(image);
            if (!entry) {
                if (g_savedCount >= kMaxSaved) continue;
                entry = &g_saved[g_savedCount++];
                entry->image = image;
                entry->color = g_fn.getColor(image, nullptr);
            }
            const Color4& original = entry->color;
            Color4 mixed{original.r * tint.r, original.g * tint.g, original.b * tint.b, original.a};
            if (!std::isfinite(mixed.r) || !std::isfinite(mixed.g) || !std::isfinite(mixed.b)) continue;
            g_fn.setColor(image, mixed, nullptr);
            ++applied;
        }
    }
    if (!scanned) return;
    g_tinted.store(applied, std::memory_order_relaxed);
    g_scans.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace detail

// Unity main thread entry (rides the visuals dispatch slot).
inline void onPlayerUpdate(void* player) {
    (void)player;
    const Settings& s = g_settings;
    if (!s.enabled) {
        if (g_status.load(std::memory_order_relaxed) == 3) {
            detail::restoreAll();
        }
        g_status.store(0, std::memory_order_relaxed);
        return;
    }
    if (!detail::resolveFunctions()) return;
    const auto now = std::chrono::steady_clock::now();
    if (now - detail::g_lastScan < std::chrono::milliseconds(800)) return;
    detail::g_lastScan = now;
    detail::scanAndApply();
    g_status.store(3, std::memory_order_relaxed);
}

}  // namespace astra::hudcolor
