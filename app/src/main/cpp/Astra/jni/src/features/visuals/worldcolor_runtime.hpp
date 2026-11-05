#pragma once
// Native world color for the 1.0.0 ARM64 profile.
//
// Material-replacement world recolor (the proven approach from the original
// game's cheat): every UnityEngine.MeshRenderer in the process - walls, props,
// map geometry - gets its materials re-shaded to one flat colored shader:
//
//   Type.GetType("UnityEngine.MeshRenderer, UnityEngine.CoreModule")
//   -> Object.FindObjectsOfTypeAll(type)
//   -> renderer.get_sharedMaterials()
//   -> per material: set_shader(WorldShader), set_mainTexture(null) [solid],
//      set_color(WorldColor)
//
// Materials are mutated in place and deduplicated in a restore table, so the
// recolor survives forever (no per-frame fighting with the lighting system)
// and can be undone material-by-material when the feature turns off. The scan
// reruns every few seconds to catch objects the map spawns later.
//
// Fog is driven through UnityEngine.RenderSettings static setters
// (set_fog / set_fogMode / set_fogColor / set_fogStartDistance /
// set_fogEndDistance) with originals captured once per world.
//
// ALPHA (v6.5): the Alpha slider owns the FOG color only - applied fog color
// = lerp(original fog, fog color, fog alpha). The world recolor color is
// always opaque; the old alpha->transparent-world shader swap is removed.
//
// REMOVED (v6.6, user request): Sun color, Fullbright and Night mode. The
// sun-color setter pair, the unlit shader family (g_worldShaderUnlit), the
// night mix and the Brightness/Darkness sliders are gone - the material pass
// is the plain opaque recolor again and RenderSettings handling is fog-only.
//
// Threading: tick() is driven from chams::onPlayerUpdate (Unity main thread,
// per player update). Scans are throttled; the render thread only writes the
// word-sized Settings fields.

#include "features/visuals/esp_runtime.hpp"
#include "game/il2cpp_meta.hpp"
#include "game/offsets.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>

namespace astra::worldcolor {

// Blittable stand-in for UnityEngine.Color (shared with chams_runtime.hpp).
struct Color4 {
    float r{1.0f}, g{1.0f}, b{1.0f}, a{1.0f};
};
static_assert(sizeof(Color4) == 16, "UnityEngine.Color ABI size");

struct Settings {
    bool enabled{false};   // World Color master (material replacement)
    bool solid{true};      // remove albedo textures (set_mainTexture(null))
    bool fog{false};       // force RenderSettings fog with the fog color
    Color4 color{0.16f, 0.36f, 0.72f, 1.0f};  // recolor tint (.a forced opaque)
    Color4 fogColor{0.16f, 0.36f, 0.72f, 1.0f};  // .a = fog alpha (color strength)
    float fogStart{0.0f};
    float fogEnd{150.0f};
};

// GUI (render thread) writes, main thread reads; word-sized fields only.
inline Settings g_settings;

// Diagnostics surfaced on the Visuals / World page.
inline std::atomic<int> g_status{0};    // 0 off, 1 waiting, 2 unavailable, 3 active
inline std::atomic<int> g_tinted{0};    // materials currently recolored
inline std::atomic<int> g_metaResolved{0};
// True while the feature had at least one successful pass (drives the
// restore-on-disable path). Defined here so tick() can read it first.
inline bool g_appliedFlag{false};

namespace detail {

namespace ed = astra::esp::detail;
using namespace astra::offsets;
using Clock = std::chrono::steady_clock;

using RSGetColorFn = Color4 (*)(const void* method);
using RSSetColorFn = void (*)(Color4 value, const void* method);
using RSGetFogFn = bool (*)(const void* method);
using RSSetFogFn = void (*)(bool value, const void* method);
using RSGetFloatFn = float (*)(const void* method);
using RSSetFloatFn = void (*)(float value, const void* method);
using RSGetModeFn = int (*)(const void* method);
using RSSetModeFn = void (*)(int value, const void* method);
using TypeGetTypeFn = void* (*)(void* name, const void* method);
using FindObjectsOfTypeAllFn = void* (*)(void* type, const void* method);
using ShaderFindFn = void* (*)(void* name, const void* method);
using MatGetShaderFn = void* (*)(const void* material, const void* method);
using MatSetShaderFn = void (*)(void* material, const void* shader, const void* method);
using MatGetColorPropFn = Color4 (*)(const void* material, const void* method);
using MatSetColorPropFn = void (*)(void* material, Color4 color, const void* method);
using MatGetColorFn = Color4 (*)(const void* material, void* name, const void* method);
using MatSetColorFn = void (*)(void* material, void* name, Color4 color, const void* method);
using MatHasPropertyFn = bool (*)(const void* material, void* name, const void* method);
using MatGetMainTextureFn = void* (*)(const void* material, const void* method);
using MatSetMainTextureFn = void (*)(void* material, void* texture, const void* method);
using MatGetTextureByNameFn = void* (*)(const void* material, void* name, const void* method);
using MatSetTextureByNameFn = void (*)(void* material, void* name, void* texture, const void* method);
using RendererGetSharedMaterialsFn = void* (*)(void* renderer, const void* method);
using StringNewFn = void* (*)(const char* value);

struct Functions {
    StringNewFn stringNew{nullptr};
    ShaderFindFn shaderFind{nullptr};
    ShaderFindFn shaderFindBuiltin{nullptr};
    TypeGetTypeFn typeGetType{nullptr};
    FindObjectsOfTypeAllFn findObjectsOfTypeAll{nullptr};
    RendererGetSharedMaterialsFn getSharedMaterials{nullptr};
    MatSetShaderFn setShader{nullptr};
    MatGetShaderFn getShader{nullptr};
    MatSetColorPropFn setColorProp{nullptr};
    MatGetColorPropFn getColorProp{nullptr};
    MatSetColorFn setColor{nullptr};
    MatGetColorFn getColor{nullptr};
    MatHasPropertyFn hasProperty{nullptr};
    MatSetMainTextureFn setMainTexture{nullptr};
    MatGetMainTextureFn getMainTexture{nullptr};
    MatSetTextureByNameFn setTexture{nullptr};
    MatGetTextureByNameFn getTexture{nullptr};
    // Fog statics
    RSGetFogFn getFog{nullptr};
    RSSetFogFn setFog{nullptr};
    RSGetColorFn getFogColor{nullptr};
    RSSetColorFn setFogColor{nullptr};
    RSGetFloatFn getFogStart{nullptr};
    RSSetFloatFn setFogStart{nullptr};
    RSGetFloatFn getFogEnd{nullptr};
    RSSetFloatFn setFogEnd{nullptr};
    RSGetModeFn getFogMode{nullptr};
    RSSetModeFn setFogMode{nullptr};
};

inline Functions g_fn;

inline bool liveUnityObject(void* object){
 if(!object)return false;
 std::uintptr_t cached=0;
 return ed::readValue(reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(object)+0x10),cached)&&
        cached&&ed::readable(reinterpret_cast<const void*>(cached),32);
}

inline bool g_initialized{false};
inline Clock::time_point g_lastInitAttempt{};
inline Clock::time_point g_lastScan{};
inline Clock::time_point g_lastFogApply{};
inline std::atomic<std::uint64_t> g_fogApplies{0};  // fog asserted per tick
inline void* g_meshRendererType{nullptr};  // cached System.Type of MeshRenderer

inline bool textPointer(std::uintptr_t value) {
    const std::uintptr_t base = ed::g_runtime.unityBase;
    return base && value >= base + offsets::elf::kTextBegin && value < base + offsets::elf::kTextEnd;
}

template <typename T>
inline T resolve(std::uintptr_t rva) {
    const std::uintptr_t base = ed::g_runtime.unityBase;
    if (!rva || !base) return nullptr;
    const std::uintptr_t address = base + rva;
    if (!textPointer(address) || !ed::readable(reinterpret_cast<const void*>(address), 16)) return nullptr;
    return reinterpret_cast<T>(address);
}

// METADATA-FIRST (v6.1): live il2cpp tables are authoritative on every game
// build; the dump RVA is only a fallback (a game update moves libunity code
// and stale RVAs then call into the wrong method). Drift is counted and
// logged in game/il2cpp_meta.hpp.
template <typename T>
inline T resolveTarget(std::uintptr_t rva, const char* namespaze, const char* klass,
                       const char* method, int argc, const char* param0 = nullptr) {
    void* target = astra::il2cpp_meta::resolveMethod<void*>(rva, namespaze, klass, method,
                                                              argc, param0);
    if (!target) return nullptr;
    g_metaResolved.fetch_add(1, std::memory_order_relaxed);
    return reinterpret_cast<T>(target);
}

inline void* makeString(const char* value) {
    return g_fn.stringNew && value ? g_fn.stringNew(value) : nullptr;
}

inline void* cachedString(const char* value) {
    struct Entry {
        const char* name;
        void* handle;
    };
    static Entry entries[] = {{"_Color", nullptr},   {"_BaseColor", nullptr},
                              {"_MainTex", nullptr}, {"_BaseMap", nullptr},
                              {"_Cutoff", nullptr}};
    for (Entry& entry : entries) {
        if (std::strcmp(entry.name, value) == 0) {
            if (!entry.handle) entry.handle = makeString(value);
            return entry.handle;
        }
    }
    return makeString(value);
}

// Shader used for the recolor. The original implementation recolored the world
// to "Legacy Shaders/Diffuse"; URP fallbacks cover builds that no longer ship
// the builtin shaders. The transparent variant drives the Alpha slider: alpha
// below 1 re-shades the world with a translucent Diffuse instead.
inline void* findShader(const char* name) {
    if (!g_fn.shaderFind || !g_fn.stringNew) return nullptr;
    void* shader = g_fn.shaderFindBuiltin ? g_fn.shaderFindBuiltin(makeString(name), nullptr) : nullptr;
    return shader ? shader : g_fn.shaderFind(makeString(name), nullptr);
}

inline void* g_worldShader{nullptr};            // resolved recolor shader (opaque)
inline int g_shaderSource{0};              // 1 builtin, 2 URP fallback

inline bool resolveWorldShader() {
    static const char* kNames[] = {"Legacy Shaders/Diffuse", "Mobile/Diffuse",
                                   "Universal Render Pipeline/Unlit", "Unlit/Color",
                                   "Sprites/Default"};
    for (const char* name : kNames) {
        if (void* shader = findShader(name)) {
            g_worldShader = shader;
            g_shaderSource = (std::strstr(name, "Universal Render Pipeline") != nullptr) ? 2 : 1;
            break;
        }
    }
    return g_worldShader != nullptr;
}

struct Saved {
    void* material{nullptr};
    void* shader{nullptr};
    Color4 color{};
    void* texture{nullptr};
    bool baseColor{false};  // write colors through the _BaseColor string
    bool baseMap{false};    // texture writes go through _BaseMap
    Clock::time_point lastSeen{};
};

inline constexpr std::size_t kMaxSaved = 1024;
inline std::array<Saved, kMaxSaved> g_saved{};
inline std::size_t g_savedCount{0};

inline Saved* findSavedMaterial(void* material) {
    for (std::size_t i = 0; i < g_savedCount; ++i) {
        if (g_saved[i].material == material) return &g_saved[i];
    }
    return nullptr;
}

inline bool initialize() {
    if (g_initialized) return true;
    const auto now = Clock::now();
    if (now - g_lastInitAttempt < std::chrono::seconds(2)) return false;
    g_lastInitAttempt = now;
    if (!ed::g_runtime.ready || !ed::g_runtime.unityBase) {
        g_status.store(1, std::memory_order_relaxed);
        return false;
    }

    Functions fn{};
    namespace vr = visuals_rva;
    void* unityHandle = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    fn.stringNew = ed::resolveSymbol<StringNewFn>(unityHandle, "il2cpp_string_new");
    if (!fn.stringNew) {
        fn.stringNew = resolve<StringNewFn>(offsets::il2cpp_api::kStringNew);
        if (fn.stringNew) g_metaResolved.fetch_add(1, std::memory_order_relaxed);
    }
    astra::il2cpp_meta::init(ed::g_runtime.unityBase);
    if (!fn.stringNew) {
        g_status.store(2, std::memory_order_relaxed);
        __android_log_print(ANDROID_LOG_WARN, astra::esp::kLogTag,
                            "world color init: il2cpp_string_new unavailable");
        return false;
    }

    fn.shaderFind = resolveTarget<ShaderFindFn>(vr::shader::kFind, "UnityEngine", "Shader", "Find", 1);
    fn.shaderFindBuiltin = resolve<ShaderFindFn>(vr::shader::kFindBuiltin);
    fn.typeGetType = resolveTarget<TypeGetTypeFn>(vr::reflection::kTypeGetType, "System", "Type", "GetType", 1);
    fn.findObjectsOfTypeAll = resolveTarget<FindObjectsOfTypeAllFn>(vr::reflection::kFindObjectsOfTypeAll, "UnityEngine", "Object", "FindObjectsOfTypeAll", 1);
    fn.getSharedMaterials = resolveTarget<RendererGetSharedMaterialsFn>(vr::renderer::kGetSharedMaterials, "UnityEngine", "Renderer", "get_sharedMaterials", 0);
    fn.setShader = resolveTarget<MatSetShaderFn>(vr::material::kSetShader, "UnityEngine", "Material", "set_shader", 1);
    fn.getShader = resolveTarget<MatGetShaderFn>(vr::material::kGetShader, "UnityEngine", "Material", "get_shader", 0);
    fn.setColorProp = resolveTarget<MatSetColorPropFn>(vr::material::kSetColorProp, "UnityEngine", "Material", "set_color", 1);
    fn.getColorProp = resolveTarget<MatGetColorPropFn>(vr::material::kGetColorProp, "UnityEngine", "Material", "get_color", 0);
    fn.setColor = resolveTarget<MatSetColorFn>(vr::material::kSetColor, "UnityEngine", "Material", "SetColor", 2, "propertyName");
    fn.getColor = resolveTarget<MatGetColorFn>(vr::material::kGetColor, "UnityEngine", "Material", "GetColor", 1, "propertyName");
    fn.hasProperty = resolveTarget<MatHasPropertyFn>(vr::material::kHasProperty, "UnityEngine", "Material", "HasProperty", 1, "propertyName");
    fn.setMainTexture = resolveTarget<MatSetMainTextureFn>(vr::material::kSetMainTexture, "UnityEngine", "Material", "set_mainTexture", 1);
    fn.getMainTexture = resolveTarget<MatGetMainTextureFn>(vr::material::kGetMainTexture, "UnityEngine", "Material", "get_mainTexture", 0);
    fn.setTexture = resolveTarget<MatSetTextureByNameFn>(vr::material::kSetTexture, "UnityEngine", "Material", "SetTexture", 2, "propertyName");
    fn.getTexture = resolveTarget<MatGetTextureByNameFn>(vr::material::kGetTexture, "UnityEngine", "Material", "GetTexture", 1, "propertyName");

    namespace rs = vr::render_settings;
    fn.getFog = resolveTarget<RSGetFogFn>(rs::kGetFog, "UnityEngine", "RenderSettings", "get_fog", 0);
    fn.setFog = resolveTarget<RSSetFogFn>(rs::kSetFog, "UnityEngine", "RenderSettings", "set_fog", 1);
    fn.getFogColor = resolveTarget<RSGetColorFn>(rs::kGetFogColor, "UnityEngine", "RenderSettings", "get_fogColor", 0);
    fn.setFogColor = resolveTarget<RSSetColorFn>(rs::kSetFogColor, "UnityEngine", "RenderSettings", "set_fogColor", 1);
    fn.getFogStart = resolveTarget<RSGetFloatFn>(rs::kGetFogStart, "UnityEngine", "RenderSettings", "get_fogStartDistance", 0);
    fn.setFogStart = resolveTarget<RSSetFloatFn>(rs::kSetFogStart, "UnityEngine", "RenderSettings", "set_fogStartDistance", 1);
    fn.getFogEnd = resolveTarget<RSGetFloatFn>(rs::kGetFogEnd, "UnityEngine", "RenderSettings", "get_fogEndDistance", 0);
    fn.setFogEnd = resolveTarget<RSSetFloatFn>(rs::kSetFogEnd, "UnityEngine", "RenderSettings", "set_fogEndDistance", 1);
    fn.getFogMode = resolveTarget<RSGetModeFn>(rs::kGetFogMode, "UnityEngine", "RenderSettings", "get_fogMode", 0);
    fn.setFogMode = resolveTarget<RSSetModeFn>(rs::kSetFogMode, "UnityEngine", "RenderSettings", "set_fogMode", 1);

    // The material path is the feature; fog is an add-on and degrades alone.
    if (!fn.shaderFind || !fn.typeGetType || !fn.findObjectsOfTypeAll || !fn.getSharedMaterials ||
        !fn.setShader || !fn.getShader || !fn.setMainTexture || !fn.getMainTexture || !fn.getColorProp || !fn.setColorProp) {
        g_status.store(2, std::memory_order_relaxed);
        __android_log_print(ANDROID_LOG_WARN, astra::esp::kLogTag,
                            "world color init: entry points missing find=%p getType=%p findAll=%p sharedMats=%p setShader=%p setMainTex=%p",
                            reinterpret_cast<void*>(fn.shaderFind),
                            reinterpret_cast<void*>(fn.typeGetType),
                            reinterpret_cast<void*>(fn.findObjectsOfTypeAll),
                            reinterpret_cast<void*>(fn.getSharedMaterials),
                            reinterpret_cast<void*>(fn.setShader),
                            reinterpret_cast<void*>(fn.setMainTexture));
        return false;
    }
    g_fn = fn;

    if (!resolveWorldShader()) {
        g_status.store(2, std::memory_order_relaxed);
        __android_log_print(ANDROID_LOG_WARN, astra::esp::kLogTag,
                            "world color init: no recolor shader found");
        return false;
    }

    g_initialized = true;
    g_meshRendererType = nullptr;  // resolve on the first scan
    g_savedCount = 0;
    __android_log_print(ANDROID_LOG_INFO, astra::esp::kLogTag,
                        "world color ready: shaderSource=%d fog=%d metaResolved=%d",
                        g_shaderSource, fn.setFog ? 1 : 0,
                        g_metaResolved.load(std::memory_order_relaxed));
    return true;
}

// One pass over every MeshRenderer in the process. Materials are deduplicated
// by pointer: many renderers share the same material, and the restore table
// would otherwise explode on map-sized scenes.
inline void scanAndRecolor(const Color4& color, bool solid) {
    void* shader = g_worldShader;
    if (!shader) return;
    if (!g_meshRendererType) {
        if (g_fn.typeGetType) {
            g_meshRendererType = g_fn.typeGetType(makeString("UnityEngine.MeshRenderer, UnityEngine.CoreModule"), nullptr);
        }
        if (!g_meshRendererType) return;
    }
    void* array = g_fn.findObjectsOfTypeAll(g_meshRendererType, nullptr);
    if (!array || !ed::readable(array, 32)) return;

    std::uintptr_t length = 0;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(array) +
                                               offsets::managed::kArrayLength),
                       length)) {
        return;
    }
    if (length == 0 || length > 100000) return;  // sanity cap

    const std::uintptr_t data = reinterpret_cast<std::uintptr_t>(array) + offsets::managed::kArrayData;
    int touched = 0;
    bool tableFull = g_savedCount >= kMaxSaved;
    for (std::uintptr_t i = 0; i < length; ++i) {
        void* renderer = nullptr;
        if (!ed::readValue(reinterpret_cast<void*>(data + i * 8), renderer) || !renderer) continue;
        if (!liveUnityObject(renderer)) continue;
        void* materials = g_fn.getSharedMaterials(renderer, nullptr);
        if (!materials || !ed::readable(materials, 32)) continue;
        std::uintptr_t matCount = 0;
        if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(materials) +
                                                   offsets::managed::kArrayLength),
                           matCount)) {
            continue;
        }
        if (matCount == 0 || matCount > 16) continue;
        const std::uintptr_t matData = reinterpret_cast<std::uintptr_t>(materials) + offsets::managed::kArrayData;
        for (std::uintptr_t j = 0; j < matCount; ++j) {
            void* material = nullptr;
            if (!ed::readValue(reinterpret_cast<void*>(matData + j * 8), material) || !material) continue;
            if (!liveUnityObject(material)) continue;

            Saved* entry = findSavedMaterial(material);
            if (!entry) {
                if (tableFull) continue;  // never recolor what we cannot restore
                entry = &g_saved[g_savedCount++];
                *entry = Saved{};
                entry->material = material;
                entry->shader = g_fn.getShader(material, nullptr);
                entry->texture = g_fn.getMainTexture ? g_fn.getMainTexture(material, nullptr) : nullptr;
                // Material.color is [MainColor]-aware in Unity 6 (URP marks
                // _BaseColor with it), so the property form covers both URP
                // and builtin shaders. String forms stay as the fallback.
                entry->color = g_fn.getColorProp ? g_fn.getColorProp(material, nullptr)
                                                 : Color4{1.0f, 1.0f, 1.0f, 1.0f};
                entry->baseColor = g_fn.hasProperty &&
                                   g_fn.hasProperty(material, cachedString("_BaseColor"), nullptr);
                entry->baseMap = g_fn.hasProperty &&
                                 g_fn.hasProperty(material, cachedString("_BaseMap"), nullptr);
                if (!entry->shader) {
                    --g_savedCount;  // untrackable material, skip it entirely
                    continue;
                }
                if (g_savedCount >= kMaxSaved) tableFull = true;
            }
            entry->lastSeen = Clock::now();

            g_fn.setShader(material, shader, nullptr);
            if (solid && g_fn.setMainTexture) g_fn.setMainTexture(material, nullptr, nullptr);
            if (g_fn.setColorProp) {
                g_fn.setColorProp(material, color, nullptr);
            } else if (g_fn.setColor) {
                g_fn.setColor(material, cachedString(entry->baseColor ? "_BaseColor" : "_Color"),
                              color, nullptr);
            }
            ++touched;
        }
    }
    g_tinted.store(touched, std::memory_order_relaxed);
}

inline void restoreMaterials() {
    int restored = 0;
    for (std::size_t i = 0; i < g_savedCount; ++i) {
        Saved& entry = g_saved[i];
        if (!liveUnityObject(entry.material) || !liveUnityObject(entry.shader)) continue;
        g_fn.setShader(entry.material, entry.shader, nullptr);
        if (g_fn.setMainTexture) g_fn.setMainTexture(entry.material, entry.texture, nullptr);
        if (g_fn.setColorProp) {
            g_fn.setColorProp(entry.material, entry.color, nullptr);
        } else if (g_fn.setColor) {
            g_fn.setColor(entry.material, cachedString(entry.baseColor ? "_BaseColor" : "_Color"),
                          entry.color, nullptr);
        }
        ++restored;
    }
    g_savedCount = 0;
    g_tinted.store(0, std::memory_order_relaxed);
    __android_log_print(ANDROID_LOG_INFO, astra::esp::kLogTag,
                        "world color restored: %d materials", restored);
}

// Fog state (captured once per world).
struct FogSaved {
    void* world{nullptr};
    bool fog{false};
    Color4 color{};
    float start{0.0f};
    float end{0.0f};
    int mode{0};
    bool valid{false};
};
inline FogSaved g_fogSaved;

inline void captureFog(void* world) {
    g_fogSaved.world = world;
    g_fogSaved.fog = g_fn.getFog ? g_fn.getFog(nullptr) : false;
    g_fogSaved.color = g_fn.getFogColor ? g_fn.getFogColor(nullptr) : Color4{};
    g_fogSaved.start = g_fn.getFogStart ? g_fn.getFogStart(nullptr) : 0.0f;
    g_fogSaved.end = g_fn.getFogEnd ? g_fn.getFogEnd(nullptr) : 0.0f;
    g_fogSaved.mode = g_fn.getFogMode ? g_fn.getFogMode(nullptr) : 0;
    g_fogSaved.valid = true;
}

inline void restoreFog() {
    if (!g_fogSaved.valid) return;
    if (g_fn.setFog) g_fn.setFog(g_fogSaved.fog, nullptr);
    if (g_fn.setFogColor) g_fn.setFogColor(g_fogSaved.color, nullptr);
    if (g_fn.setFogStart) g_fn.setFogStart(g_fogSaved.start, nullptr);
    if (g_fn.setFogEnd) g_fn.setFogEnd(g_fogSaved.end, nullptr);
    if (g_fn.setFogMode) g_fn.setFogMode(g_fogSaved.mode, nullptr);
    g_fogSaved.valid = false;
}

inline void applyFog(const Settings& s) {
    if (!g_fn.setFog || !g_fn.setFogColor || !g_fn.setFogStart || !g_fn.setFogEnd) return;
    // Linear fog (FogMode 1) honors the start/end pair, like the original.
    if (g_fn.setFogMode) g_fn.setFogMode(1, nullptr);
    g_fn.setFog(true, nullptr);
    // ALPHA OWNS THE FOG: the applied color is the original fog blended toward
    // the user color by the fog alpha (100% = full custom color).
    const float a = std::clamp(s.fogColor.a, 0.0f, 1.0f);
    const Color4& orig = g_fogSaved.valid ? g_fogSaved.color : s.fogColor;
    g_fn.setFogColor(Color4{orig.r + (s.fogColor.r - orig.r) * a,
                            orig.g + (s.fogColor.g - orig.g) * a,
                            orig.b + (s.fogColor.b - orig.b) * a, 1.0f}, nullptr);
    g_fn.setFogStart(s.fogStart, nullptr);
    g_fn.setFogEnd(s.fogEnd, nullptr);
}

}  // namespace detail

// Aliases for the outer tick(); detail keeps its own copies.
namespace ed = astra::esp::detail;
using Clock = std::chrono::steady_clock;

// Drives (and clears) the world recolor + fog. Called from chams::onPlayerUpdate
// on the Unity main thread, once per player update (every frame).
//
// FOG IS NOT THROTTLED and does not depend on the World Color master toggle:
// the game's lighting controller rewrites RenderSettings on every frame, so
// fog must be re-asserted after each game write, exactly like the original
// cheat did (world.cpp called fog() every tick). The five setters are cheap
// indirect calls; per-tick cost is negligible next to the material scan.
inline void tick(void* player) {
    const Settings& s = g_settings;

    // World tracking: a scene change frees every material we saved, so drop
    // the tables instead of writing into dangling pointers.
    void* world = nullptr;
    if (player) {
        world = ed::readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player) +
                                                       offsets::field::mfc::kWorld));
    }
    if (detail::g_fogSaved.valid && detail::g_fogSaved.world != world) {
        detail::g_fogSaved.valid = false;   // new scene: RenderSettings are fresh
        detail::g_savedCount = 0;           // old materials are gone
        detail::g_lastScan = Clock::time_point{};  // force an immediate rescan
        detail::g_meshRendererType = nullptr;      // types survive, but re-verify cheaply
        g_appliedFlag = false;
    }

    // Fog pass: independent of the World Color master. Capture the originals
    // once per world, then re-assert every tick so the game's per-frame
    // lighting rewrites never win a render.
    if (s.fog) {
        if (!detail::initialize()) return;
        const auto& f=detail::g_fn;
        if(!f.getFog||!f.setFog||!f.getFogColor||!f.setFogColor||
           !f.getFogStart||!f.setFogStart||!f.getFogEnd||!f.setFogEnd||
           !f.getFogMode||!f.setFogMode){g_status.store(2);return;}

        if (!detail::g_fogSaved.valid) detail::captureFog(world);
        detail::applyFog(s);
        detail::g_fogApplies.fetch_add(1, std::memory_order_relaxed);
        g_status.store(3, std::memory_order_relaxed);
    } else if (detail::g_fogSaved.valid) {
        detail::restoreFog();
    }

    // Material pass: the plain opaque World Color recolor.
    if (!s.enabled) {
        if (g_appliedFlag) {
            detail::restoreMaterials();
            g_appliedFlag = false;
            if (!s.fog) g_status.store(0, std::memory_order_relaxed);
        }
        return;
    }
    if (!detail::initialize()) return;

    Color4 effective = s.color;
    effective.a = 1.0f;

    const auto now = Clock::now();

    // Material scan: the heavy pass runs at most every 3 seconds.
    if (now - detail::g_lastScan > std::chrono::milliseconds(3000) || detail::g_savedCount == 0) {
        detail::g_lastScan = now;
        detail::scanAndRecolor(effective, s.solid);
    }
    g_appliedFlag = true;
    g_status.store(3, std::memory_order_relaxed);
}

}  // namespace astra::worldcolor
