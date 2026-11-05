#pragma once
// Runtime il2cpp metadata resolver for the stripped 1.0.0 ARM64 build.
//
// libunity.so does not export the il2cpp API, so every entry point comes from
// the fixed 91-entry API table in game/offsets.hpp (base + RVA). On top of
// that, methods are resolved BY NAME through the live metadata:
//
//   il2cpp_domain_get -> il2cpp_domain_assembly_open("UnityEngine.CoreModule.dll")
//                     -> il2cpp_assembly_get_image
//                     -> il2cpp_class_from_name(image, ns, name)
//                     -> MethodInfo* (class_get_method_from_name / method walk)
//                     -> call target = *(MethodInfo + method_info::kMethodPointer)
//
// The dump.cs RVAs in visuals_rva are exact for this build, but the metadata
// path keeps the natives (chams / world color) alive when a game update moves
// managed code while the il2cpp API table stays put. Call targets are always
// validated to live inside libunity text before being handed out.
//
// Threading: init()/lookups run on the Unity main thread from the ake update
// hook, exactly like the rest of the ESP runtime. First init attaches nothing
// new; the ESP runtime already attached this thread to the il2cpp domain.

#include "features/visuals/esp_runtime.hpp"
#include "game/offsets.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>

namespace astra::il2cpp_meta {

namespace ed = astra::esp::detail;
using namespace astra::offsets;

// Diagnostics for the Visuals pages and the log.
inline std::atomic<int> g_apiResolved{0};   // count of API entries bound
inline std::atomic<int> g_metaClasses{0};   // classes resolved by name
inline std::atomic<int> g_metaMethods{0};   // methods resolved by name
inline std::atomic<int> g_rvaDrift{0};      // dump RVA != live metadata hits

namespace detail {

using DomainGetFn = void* (*)();
using DomainAssemblyOpenFn = void* (*)(void*, const char*);
using AssemblyGetImageFn = void* (*)(void*);
using ClassFromNameFn = void* (*)(void*, const char*, const char*);
using ClassGetMethodFromNameFn = void* (*)(void*, const char*, int);
using ClassGetMethodsFn = void* (*)(void*, void**);
using MethodGetNameFn = const char* (*)(void*);
using MethodGetParamCountFn = std::uint32_t (*)(void*);
using MethodGetParamNameFn = const char* (*)(void*, std::uint32_t);
using StringNewFn = void* (*)(const char*);
using StringCharsFn = const std::uint16_t* (*)(void*);

struct Api {
    DomainGetFn domainGet{nullptr};
    DomainAssemblyOpenFn domainAssemblyOpen{nullptr};
    AssemblyGetImageFn assemblyGetImage{nullptr};
    ClassFromNameFn classFromName{nullptr};
    ClassGetMethodFromNameFn classGetMethodFromName{nullptr};
    ClassGetMethodsFn classGetMethods{nullptr};
    MethodGetNameFn methodGetName{nullptr};
    MethodGetParamCountFn methodGetParamCount{nullptr};
    MethodGetParamNameFn methodGetParamName{nullptr};
    StringNewFn stringNew{nullptr};
    StringCharsFn stringChars{nullptr};
};

inline Api g_api;
inline bool g_ready{false};
inline std::uintptr_t g_base{0};

struct CachedClass {
    const char* namespaze;
    const char* name;
    void* klass;
};
inline constexpr int kMaxCachedClasses = 16;
inline CachedClass g_classes[kMaxCachedClasses];
inline int g_classCount{0};

template <typename T>
inline T resolveEntry(void* unity, const char* name, std::uintptr_t rva) {
    if (unity) {
        if (void* symbol = dlsym(unity, name)) return reinterpret_cast<T>(symbol);
    }
    if (void* symbol = dlsym(RTLD_DEFAULT, name)) return reinterpret_cast<T>(symbol);
#if defined(__aarch64__)
    if (rva && g_base && rva >= offsets::elf::kTextBegin && rva < offsets::elf::kTextEnd) {
        return reinterpret_cast<T>(g_base + rva);
    }
#else
    (void)name;
    (void)rva;
#endif
    return nullptr;
}

inline bool textTarget(std::uintptr_t value) {
    if (!g_base || value < g_base + offsets::elf::kTextBegin || value >= g_base + offsets::elf::kTextEnd) {
        return false;
    }
    return ed::readable(reinterpret_cast<const void*>(value), 16);
}

}  // namespace detail

inline bool ready() { return detail::g_ready; }

// Resolves the il2cpp API of this build. Safe to call repeatedly; the work
// happens once and only succeeds when libunity is mapped.
inline bool init(std::uintptr_t unityBase) {
    if (detail::g_ready) return true;
    if (!unityBase) return false;
    detail::g_base = unityBase;

    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    detail::Api api{};
    api.domainGet = detail::resolveEntry<detail::DomainGetFn>(unity, "il2cpp_domain_get", il2cpp_api::kDomainGet);
    api.domainAssemblyOpen = detail::resolveEntry<detail::DomainAssemblyOpenFn>(unity, "il2cpp_domain_assembly_open", il2cpp_api::kDomainAssemblyOpen);
    api.assemblyGetImage = detail::resolveEntry<detail::AssemblyGetImageFn>(unity, "il2cpp_assembly_get_image", il2cpp_api::kAssemblyGetImage);
    api.classFromName = detail::resolveEntry<detail::ClassFromNameFn>(unity, "il2cpp_class_from_name", 0x6330BEC); // donor 1.0.0 ARM64 API entry
    api.classGetMethodFromName = detail::resolveEntry<detail::ClassGetMethodFromNameFn>(unity, "il2cpp_class_get_method_from_name", il2cpp_api::kClassGetMethodFromName);
    api.classGetMethods = detail::resolveEntry<detail::ClassGetMethodsFn>(unity, "il2cpp_class_get_methods", il2cpp_api::kClassGetMethods);
    api.methodGetName = detail::resolveEntry<detail::MethodGetNameFn>(unity, "il2cpp_method_get_name", il2cpp_api::kMethodGetName);
    api.methodGetParamCount = detail::resolveEntry<detail::MethodGetParamCountFn>(unity, "il2cpp_method_get_param_count", il2cpp_api::kMethodGetParamCount);
    api.methodGetParamName = detail::resolveEntry<detail::MethodGetParamNameFn>(unity, "il2cpp_method_get_param_name", il2cpp_api::kMethodGetParamName);
    api.stringNew = detail::resolveEntry<detail::StringNewFn>(unity, "il2cpp_string_new", il2cpp_api::kStringNew);
    api.stringChars = detail::resolveEntry<detail::StringCharsFn>(unity, "il2cpp_string_chars", il2cpp_api::kStringChars);

    // classFromName + at least one method resolver are the hard requirement;
    // param inspection is optional because class_get_method_from_name covers
    // unambiguous lookups.
    if (!api.domainGet || !api.domainAssemblyOpen || !api.assemblyGetImage || !api.classFromName ||
        (!api.classGetMethodFromName && !api.classGetMethods)) {
        return false;
    }

    // The domain must be alive: natives only call this from the Unity main
    // thread while a match is running.
    void* domain = api.domainGet();
    if (!domain) return false;

    detail::g_api = api;
    detail::g_ready = true;
    int resolved = 0;
    if (api.domainGet) ++resolved;
    if (api.domainAssemblyOpen) ++resolved;
    if (api.assemblyGetImage) ++resolved;
    if (api.classFromName) ++resolved;
    if (api.classGetMethodFromName) ++resolved;
    if (api.classGetMethods) ++resolved;
    if (api.methodGetName) ++resolved;
    if (api.methodGetParamCount) ++resolved;
    if (api.methodGetParamName) ++resolved;
    if (api.stringNew) ++resolved;
    g_apiResolved.store(resolved, std::memory_order_relaxed);
    return true;
}

// Cached UnityEngine class lookup (CoreModule hosts every class the natives
// need: Shader, Material, Renderer, RenderSettings, Texture).
inline void* unityClass(const char* namespaze, const char* name) {
    if (!detail::g_ready || !namespaze || !name) return nullptr;
    for (int i = 0; i < detail::g_classCount; ++i) {
        if (std::strcmp(detail::g_classes[i].namespaze, namespaze) == 0 &&
            std::strcmp(detail::g_classes[i].name, name) == 0) {
            return detail::g_classes[i].klass;
        }
    }

    // CoreModule hosts the UnityEngine classes; mscorlib (CoreLib) hosts the
    // System reflection classes (System.Type) used by the world recolor;
    // UnityEngine.UI hosts Graphic/Image for the HUD recolor. PhysicsModule
    // hosts Collider (Collider.get_bounds is part of the aimbot's function
    // validation - a missed assembly there silently killed the whole aim in
    // v6.6 when the dump-RVA fallback signature no longer matched). IMGUIModule
    // hosts GUI utilities; ParticleSystemModule completes the common set.
    const char* assemblies[] = {"UnityEngine.CoreModule.dll", "UnityEngine.dll", "mscorlib.dll",
                                "UnityEngine.UI.dll", "UnityEngine.PhysicsModule.dll",
                                "UnityEngine.IMGUIModule.dll",
                                "UnityEngine.ParticleSystemModule.dll", "UnityEngine.AnimationModule.dll"};
    void* klass = nullptr;
    for (const char* assemblyName : assemblies) {
        void* assembly = detail::g_api.domainAssemblyOpen(detail::g_api.domainGet(), assemblyName);
        if (!assembly) continue;
        void* image = detail::g_api.assemblyGetImage(assembly);
        if (!image) continue;
        klass = detail::g_api.classFromName(image, namespaze, name);
        if (klass) break;
    }
    if (!klass) return nullptr;

    // Sanity: the returned class must answer with the requested name through
    // the ESP runtime helper (guards against a wrong-ABI classFromName).
    if (!ed::nameEquals(ed::className(klass), name)) return nullptr;

    if (detail::g_classCount < detail::kMaxCachedClasses) {
        detail::g_classes[detail::g_classCount++] = {namespaze, name, klass};
    }
    g_metaClasses.fetch_add(1, std::memory_order_relaxed);
    return klass;
}

// Resolves a method's call target. |param0| optionally pins the first
// parameter name to disambiguate overloads (Material SetColor has
// (string, Color) and (int, Color); only the string overload is ABI-safe for
// the natives, and its first parameter is named "propertyName").
inline void* method(void* klass, const char* name, int argc, const char* param0 = nullptr) {
    if (!detail::g_ready || !klass || !name || argc < 0) return nullptr;
    const detail::Api& api = detail::g_api;

    void* methodInfo = nullptr;
    if (!param0 && api.classGetMethodFromName) {
        methodInfo = api.classGetMethodFromName(klass, name, argc);
    }
    if (!methodInfo && api.classGetMethods) {
        // Manual walk: picks the right overload via the parameter name and
        // also covers builds where class_get_method_from_name misbehaves.
        void* iter = nullptr;
        for (int guard = 0; guard < 2048; ++guard) {
            void* candidate = api.classGetMethods(klass, &iter);
            if (!candidate) break;
            const char* candidateName = api.methodGetName ? api.methodGetName(candidate) : nullptr;
            if (!candidateName || std::strcmp(candidateName, name) != 0) continue;
            if (api.methodGetParamCount) {
                const std::uint32_t count = api.methodGetParamCount(candidate);
                if (static_cast<int>(count) != argc) continue;
                if (param0 && !api.methodGetParamName) continue;
                if (param0 && api.methodGetParamName) {
                    const char* paramName = api.methodGetParamName(candidate, 0);
                    if (!paramName || std::strcmp(paramName, param0) != 0) continue;
                }
            } else if (param0) {
                continue;  // cannot disambiguate without param metadata
            }
            methodInfo = candidate;
            break;
        }
    }
    if (!methodInfo) return nullptr;

    // MethodInfo layout of this build (game/offsets.hpp method_info).
    if (!ed::readable(methodInfo, method_info::kKlass + 8)) return nullptr;
    std::uintptr_t target = 0;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(methodInfo) +
                                               method_info::kMethodPointer),
                       target)) {
        return nullptr;
    }
    if (!detail::textTarget(target)) return nullptr;
    g_metaMethods.fetch_add(1, std::memory_order_relaxed);
    return reinterpret_cast<void*>(target);
}

// il2cpp_string_new with the fixed table RVA as the last resort.
inline void* stringNew(const char* value) {
    if (!value) return nullptr;
    if (detail::g_api.stringNew) return detail::g_api.stringNew(value);
    return nullptr;
}

// Game-side class lookup (Assembly-CSharp). unityClass() above only walks the
// UnityEngine assemblies; the game's own classes (inventory, managers) live in
// Assembly-CSharp.dll. The runtime API table already proved this path in the
// aim module's scope-enum resolver, so the same call sequence is centralized
// here. Namespaces are NOT obfuscated in this build (Axlebolt.Standoff.*),
// which is what makes name lookups viable at all.
inline void* gameClass(const char* namespaze, const char* name) {
    if (!detail::g_ready || !namespaze || !name) return nullptr;
    for (int i = 0; i < detail::g_classCount; ++i) {
        if (std::strcmp(detail::g_classes[i].namespaze, namespaze) == 0 &&
            std::strcmp(detail::g_classes[i].name, name) == 0) {
            return detail::g_classes[i].klass;
        }
    }
    void* klass = nullptr;
    const char* assemblies[] = {"Assembly-CSharp.dll", "Assembly-CSharp"};
    for (const char* assemblyName : assemblies) {
        void* assembly = detail::g_api.domainAssemblyOpen(detail::g_api.domainGet(), assemblyName);
        if (!assembly) continue;
        void* image = detail::g_api.assemblyGetImage(assembly);
        if (!image) continue;
        klass = detail::g_api.classFromName(image, namespaze, name);
        if (klass) break;
    }
    if (!klass) return nullptr;
    if (!ed::nameEquals(ed::className(klass), name)) return nullptr;
    if (detail::g_classCount < detail::kMaxCachedClasses) {
        detail::g_classes[detail::g_classCount++] = {namespaze, name, klass};
    }
    g_metaClasses.fetch_add(1, std::memory_order_relaxed);
    return klass;
}

// --- FieldInfo walk ---------------------------------------------------------
// The API table exposes field_get_offset / field_get_type but no
// field_get_name, so the FieldInfo struct is read directly. This is the stock
// il2cpp layout (name @0, parent klass @8, type @16, offset @20, token @24);
// 1.1.0 obfuscated FieldInfo: name @0, offset @8, parent @16, type @24, stride 40.
// Every hop is readable()-guarded exactly like the rest of the runtime.
namespace field_info {
inline constexpr std::size_t kName = 0x00;
inline constexpr std::size_t kParent = 0x10;
inline constexpr std::size_t kType = 0x18;
inline constexpr std::size_t kOffset = 0x08;
inline constexpr std::size_t kTotal = 0x28;
}  // namespace field_info

// Copies a C string out of game memory with an upper bound; returns false when
// the pointer is not readable or the string runs past the cap.
inline bool readCString(const void* pointer, char* out, std::size_t cap) {
    if (!pointer || !out || cap == 0) return false;
    if (!ed::readable(pointer, 1)) return false;
    const char* text = static_cast<const char*>(pointer);
    std::size_t i = 0;
    for (; i + 1 < cap; ++i) {
        if (!ed::readable(text + i, 1)) return false;
        if (!ed::readValue(text + i, out[i])) return false;
        if (!out[i]) return true;
    }
    out[i] = '\0';
    return true;
}

// il2cpp_class_get_fields (iterator protocol, same as class_get_methods).
using ClassGetFieldsFn = void* (*)(void* klass, void** iter);
inline ClassGetFieldsFn classGetFields() {
    static ClassGetFieldsFn cached = nullptr;
    if (cached) return cached;
    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    if (unity) cached = reinterpret_cast<ClassGetFieldsFn>(dlsym(unity, "il2cpp_class_get_fields"));
    if (!cached) {
        cached = reinterpret_cast<ClassGetFieldsFn>(
            ed::resolveSymbol<ClassGetFieldsFn>(nullptr, "il2cpp_class_get_fields"));
    }
    if (!cached && offsets::il2cpp_api::kClassGetFields) {
        const std::uintptr_t address = detail::g_base + offsets::il2cpp_api::kClassGetFields;
        if (ed::readable(reinterpret_cast<const void*>(address), 16)) {
            cached = reinterpret_cast<ClassGetFieldsFn>(address);
        }
    }
    return cached;
}

// Returns the FieldInfo* of the |index|-th field of |klass| (nullptr when the
// class exposes fewer fields or the iterator is unusable).
inline void* fieldAt(void* klass, int index) {
    ClassGetFieldsFn getFields = classGetFields();
    if (!getFields || !klass || index < 0) return nullptr;
    void* iter = nullptr;
    void* field = nullptr;
    for (int guard = 0; guard < 512; ++guard) {
        field = getFields(klass, &iter);
        if (!field) return nullptr;
        if (guard == index) return field;
    }
    return nullptr;
}

inline bool fieldName(void* field, char* out, std::size_t cap) {
    if (!field) return false;
    void* name = nullptr;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(field) + field_info::kName),
                       name)) {
        return false;
    }
    return readCString(name, out, cap);
}

// Field storage offset (il2cpp encodes static-field offsets as -1).
inline bool fieldOffsetValue(void* field, std::int32_t& out) {
    if (!field) return false;
    std::uint32_t raw = 0;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(field) + field_info::kOffset),
                       raw)) {
        return false;
    }
    out = static_cast<std::int32_t>(raw);
    return true;
}

inline bool fieldIsStatic(void* field) {
    std::int32_t offset = 0;
    return fieldOffsetValue(field, offset) && offset < 0;
}

// il2cpp_field_static_get_value from the fixed table (dlsym first).
using FieldStaticGetValueFn = void (*)(void* field, void* value);
inline FieldStaticGetValueFn fieldStaticGetValue() {
    static FieldStaticGetValueFn cached = nullptr;
    if (cached) return cached;
    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    if (unity) {
        cached = reinterpret_cast<FieldStaticGetValueFn>(dlsym(unity, "il2cpp_field_static_get_value"));
    }
    if (!cached) {
        cached = reinterpret_cast<FieldStaticGetValueFn>(
            ed::resolveSymbol<FieldStaticGetValueFn>(nullptr, "il2cpp_field_static_get_value"));
    }
    if (!cached && offsets::il2cpp_api::kFieldStaticGetValue && detail::g_base) {
        const std::uintptr_t address = detail::g_base + offsets::il2cpp_api::kFieldStaticGetValue;
        if (ed::readable(reinterpret_cast<const void*>(address), 16)) {
            cached = reinterpret_cast<FieldStaticGetValueFn>(address);
        }
    }
    return cached;
}

// il2cpp_object_new from the fixed table (dlsym first).
using ObjectNewFn = void* (*)(void* klass);
inline ObjectNewFn objectNew() {
    static ObjectNewFn cached = nullptr;
    if (cached) return cached;
    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    if (unity) cached = reinterpret_cast<ObjectNewFn>(dlsym(unity, "il2cpp_object_new"));
    if (!cached) {
        cached = reinterpret_cast<ObjectNewFn>(
            ed::resolveSymbol<ObjectNewFn>(nullptr, "il2cpp_object_new"));
    }
    if (!cached && offsets::il2cpp_api::kObjectNew && detail::g_base) {
        const std::uintptr_t address = detail::g_base + offsets::il2cpp_api::kObjectNew;
        if (ed::readable(reinterpret_cast<const void*>(address), 16)) {
            cached = reinterpret_cast<ObjectNewFn>(address);
        }
    }
    return cached;
}

// METADATA-FIRST method resolution (v6.1).
//
// The dump.cs RVAs are exact only for the exact build the dump was taken
// from. A game hotfix (the same update that shipped the libc anti-tamper)
// moves libunity code around, and every stale RVA then lands on a DIFFERENT
// method: calls through it silently corrupt unrelated game state - crooked
// character models, a missing first-person rig, every toggle appearing dead.
// The live il2cpp class/method tables are authoritative on every build, so
// they are consulted FIRST; the dump RVA only serves as the fallback when
// metadata is unavailable. Every meta-vs-RVA mismatch bumps g_rvaDrift and
// the first few are logged loudly - SIGUSR2 diagnostics surface the count.
template <typename T>
inline T resolveMethod(std::uintptr_t rva, const char* namespaze, const char* klass,
                       const char* methodName, int argc, const char* param0 = nullptr) {
    T viaMeta = nullptr;
    if (void* handle = unityClass(namespaze, klass)) {
        if (void* target = method(handle, methodName, argc, param0)) {
            viaMeta = reinterpret_cast<T>(target);
        }
    }
    T viaRva = nullptr;
    if (rva && detail::g_base) {
        const std::uintptr_t address = detail::g_base + rva;
        // Visual call targets require successful metadata resolution.
        (void)address;
    }
    if (viaMeta && viaRva &&
        reinterpret_cast<void*>(viaMeta) != reinterpret_cast<void*>(viaRva)) {
        const int drift = g_rvaDrift.fetch_add(1, std::memory_order_relaxed);
        if (drift < 8) {
            __android_log_print(ANDROID_LOG_ERROR, astra::esp::kLogTag,
                                "RVA DRIFT: %s.%s::%s dump=%p live=%p - game moved since the dump; live metadata wins",
                                namespaze && namespaze[0] ? namespaze : "-",
                                klass ? klass : "?", methodName ? methodName : "?",
                                reinterpret_cast<void*>(viaRva),
                                reinterpret_cast<void*>(viaMeta));
        }
    }
    return viaMeta ? viaMeta : viaRva;
}

// Same contract as resolveMethod but resolves through Assembly-CSharp (the
// game's obfuscated classes: ios camera controller, jxy weapon fire, ...).
// Metadata is authoritative; the dump RVA is the fallback.
template <typename T>
inline T resolveGameMethod(std::uintptr_t rva, const char* namespaze, const char* klass,
                           const char* methodName, int argc, const char* param0 = nullptr) {
    T viaMeta = nullptr;
    if (void* handle = gameClass(namespaze, klass)) {
        if (void* target = method(handle, methodName, argc, param0)) {
            viaMeta = reinterpret_cast<T>(target);
        }
    }
    T viaRva = nullptr;
    if (rva && detail::g_base) {
        const std::uintptr_t address = detail::g_base + rva;
        // Visual call targets require successful metadata resolution.
        (void)address;
    }
    if (viaMeta && viaRva &&
        reinterpret_cast<void*>(viaMeta) != reinterpret_cast<void*>(viaRva)) {
        const int drift = g_rvaDrift.fetch_add(1, std::memory_order_relaxed);
        if (drift < 8) {
            __android_log_print(ANDROID_LOG_ERROR, astra::esp::kLogTag,
                                "RVA DRIFT: %s.%s::%s dump=%p live=%p - game moved since the dump; live metadata wins",
                                namespaze && namespaze[0] ? namespaze : "-",
                                klass ? klass : "?", methodName ? methodName : "?",
                                reinterpret_cast<void*>(viaRva),
                                reinterpret_cast<void*>(viaMeta));
        }
    }
    return viaMeta ? viaMeta : viaRva;
}

}  // namespace astra::il2cpp_meta
