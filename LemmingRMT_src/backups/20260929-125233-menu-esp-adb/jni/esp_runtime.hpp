#pragma once

#include "game_offsets.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <vector>

namespace lemming::esp {

struct ScreenBox {
    float left;
    float top;
    float right;
    float bottom;
    bool enemy;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

enum class Status : int {
    WaitingForUnity = 0,
    ApiUnavailable,
    VContainerUnavailable,
    IcallUnavailable,
    WaitingForMatch,
    Active
};

inline std::atomic<int> g_status{static_cast<int>(Status::WaitingForUnity)};
inline std::atomic<int> g_playerCount{0};

inline const char* statusText() {
    switch (static_cast<Status>(g_status.load(std::memory_order_relaxed))) {
        case Status::WaitingForUnity: return "Ожидание libunity";
        case Status::ApiUnavailable: return "IL2CPP API не найден";
        case Status::VContainerUnavailable: return "VContainer resolver не готов";
        case Status::IcallUnavailable: return "Unity icall не найден";
        case Status::WaitingForMatch: return "Resolver готов — ожидание матча";
        case Status::Active: return "ESP активен";
    }
    return "ESP: неизвестное состояние";
}

inline int playerCount() {
    return g_playerCount.load(std::memory_order_relaxed);
}

namespace detail {

constexpr std::size_t kPtr = sizeof(void*);

struct MapRange {
    std::uintptr_t begin;
    std::uintptr_t end;
    bool readable;
};

inline std::vector<MapRange> g_maps;
inline std::chrono::steady_clock::time_point g_lastMapRefresh{};

inline void refreshMaps(bool force = false) {
    const auto now = std::chrono::steady_clock::now();
    if (!force && !g_maps.empty() && now - g_lastMapRefresh < std::chrono::seconds(1)) return;
    std::vector<MapRange> next;
    if (FILE* file = std::fopen("/proc/self/maps", "r")) {
        char line[512];
        while (std::fgets(line, sizeof(line), file)) {
            unsigned long long begin = 0, end = 0;
            char perms[5]{};
            if (std::sscanf(line, "%llx-%llx %4s", &begin, &end, perms) == 3 && begin < end) {
                next.push_back({static_cast<std::uintptr_t>(begin), static_cast<std::uintptr_t>(end), perms[0] == 'r'});
            }
        }
        std::fclose(file);
    }
    if (!next.empty()) g_maps.swap(next);
    g_lastMapRefresh = now;
}

inline bool readable(const void* pointer, std::size_t bytes = 1) {
    if (!pointer || bytes == 0) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    if (address < 0x10000 || address + bytes < address) return false;
    for (const auto& range : g_maps) {
        if (range.readable && address >= range.begin && address + bytes <= range.end) return true;
    }
    return false;
}

template <typename T>
inline bool readValue(const void* address, T& value) {
    if (!readable(address, sizeof(T))) return false;
    std::memcpy(&value, address, sizeof(T));
    return true;
}

inline void* readObject(const void* address) {
    void* value = nullptr;
    if (!readValue(address, value) || !readable(value, kPtr)) return nullptr;
    return value;
}

struct Api {
    using DomainGet = void* (*)();
    using DomainAssemblyOpen = void* (*)(void*, const char*);
    using AssemblyGetImage = void* (*)(void*);
    using ClassFromName = void* (*)(void*, const char*, const char*);
    using ClassGetFieldFromName = void* (*)(void*, const char*);
    using ClassGetFields = void* (*)(void*, void**);
    using ClassGetName = const char* (*)(void*);
    using ClassGetNamespace = const char* (*)(void*);
    using ClassGetParent = void* (*)(void*);
    using FieldGetOffset = std::size_t (*)(void*);
    using FieldGetType = void* (*)(void*);
    using FieldStaticGetValue = void (*)(void*, void*);
    using TypeGetName = char* (*)(void*);
    using Free = void (*)(void*);
    using ResolveIcall = void* (*)(const char*);
    using ThreadAttach = void* (*)(void*);

    DomainGet domainGet{};
    DomainAssemblyOpen domainAssemblyOpen{};
    AssemblyGetImage assemblyGetImage{};
    ClassFromName classFromName{};
    ClassGetFieldFromName classGetFieldFromName{};
    ClassGetFields classGetFields{};
    ClassGetName classGetName{};
    ClassGetNamespace classGetNamespace{};
    ClassGetParent classGetParent{};
    FieldGetOffset fieldGetOffset{};
    FieldGetType fieldGetType{};
    FieldStaticGetValue fieldStaticGetValue{};
    TypeGetName typeGetName{};
    Free freeFn{};
    ResolveIcall resolveIcall{};
    ThreadAttach threadAttach{};
};

struct Runtime {
    std::uintptr_t unityBase{};
    bool ready{};
    Api api{};
    void* domain{};
    void* playerLoopHelperClass{};
    void* tickableLoopItemClass{};
    void* runnersField{};
    std::vector<void*> gameManagerClasses;
    std::vector<void*> playerClasses;

    using CameraMainInjected = std::uintptr_t (*)();
    using CameraWorldToScreenInjected = void (*)(std::uintptr_t, const Vec3*, int, Vec3*);
    using TransformGetPositionInjected = void (*)(std::uintptr_t, Vec3*);
    using ComponentGetTransformInjected = std::uintptr_t (*)(std::uintptr_t);

    CameraMainInjected cameraMain{};
    CameraWorldToScreenInjected worldToScreen{};
    TransformGetPositionInjected transformPosition{};
    ComponentGetTransformInjected componentTransform{};
};

inline Runtime g_runtime;
inline std::chrono::steady_clock::time_point g_lastRetry{};
inline std::chrono::steady_clock::time_point g_lastFrame{};

template <typename T>
inline T resolveSymbol(void* unity, void* il2cpp, const char* name) {
    if (unity) {
        if (void* value = dlsym(unity, name)) return reinterpret_cast<T>(value);
    }
    if (il2cpp) {
        if (void* value = dlsym(il2cpp, name)) return reinterpret_cast<T>(value);
    }
    if (void* value = dlsym(RTLD_DEFAULT, name)) return reinterpret_cast<T>(value);
    return nullptr;
}

template <typename T>
inline T resolveApi(void* unity, void* il2cpp, const char* name, std::uintptr_t unityBase, std::uintptr_t arm64Rva) {
    if (T exported = resolveSymbol<T>(unity, il2cpp, name)) return exported;
#if defined(__aarch64__)
    if (unityBase && arm64Rva) return reinterpret_cast<T>(unityBase + arm64Rva);
#else
    (void)unityBase;
    (void)arm64Rva;
#endif
    return nullptr;
}

inline void* objectClass(void* object) {
    return object ? readObject(object) : nullptr;
}

inline const char* className(void* klass) {
    if (!klass || !g_runtime.api.classGetName || !readable(klass, kPtr)) return nullptr;
    return g_runtime.api.classGetName(klass);
}

inline bool nameEquals(const char* value, const char* expected) {
    return value && expected && std::strcmp(value, expected) == 0;
}

inline bool nameContains(const char* value, const char* needle) {
    return value && needle && std::strstr(value, needle) != nullptr;
}

inline void* findClass(const std::vector<const char*>& assemblies, const char* namespaze, const char* name) {
    for (const char* assemblyName : assemblies) {
        void* assembly = g_runtime.api.domainAssemblyOpen(g_runtime.domain, assemblyName);
        if (!assembly) continue;
        void* image = g_runtime.api.assemblyGetImage(assembly);
        if (!image) continue;
        if (void* klass = g_runtime.api.classFromName(image, namespaze, name)) return klass;
    }
    return nullptr;
}

inline void appendClass(std::vector<void*>& classes, void* klass) {
    if (klass && std::find(classes.begin(), classes.end(), klass) == classes.end()) classes.push_back(klass);
}

template <typename T, std::size_t N>
inline T resolveIcallAny(const std::array<const char*, N>& names) {
    for (const char* name : names) {
        if (void* value = g_runtime.api.resolveIcall(name)) return reinterpret_cast<T>(value);
    }
    return nullptr;
}

inline bool initialize(std::uintptr_t unityBase) {
    refreshMaps(true);
    g_runtime = Runtime{};
    g_runtime.unityBase = unityBase;
    if (!unityBase) {
        g_status.store(static_cast<int>(Status::WaitingForUnity), std::memory_order_relaxed);
        return false;
    }

    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    void* il2cpp = dlopen("libil2cpp.so", RTLD_NOW | RTLD_LOCAL);
    auto& api = g_runtime.api;
    using namespace lemming::offsets::il2cpp_api;
    api.domainGet = resolveApi<Api::DomainGet>(unity, il2cpp, "il2cpp_domain_get", unityBase, kDomainGet);
    api.domainAssemblyOpen = resolveApi<Api::DomainAssemblyOpen>(unity, il2cpp, "il2cpp_domain_assembly_open", unityBase, kDomainAssemblyOpen);
    api.assemblyGetImage = resolveApi<Api::AssemblyGetImage>(unity, il2cpp, "il2cpp_assembly_get_image", unityBase, kAssemblyGetImage);
    api.classFromName = resolveApi<Api::ClassFromName>(unity, il2cpp, "il2cpp_class_from_name", unityBase, kClassFromName);
    api.classGetFieldFromName = resolveApi<Api::ClassGetFieldFromName>(unity, il2cpp, "il2cpp_class_get_field_from_name", unityBase, kClassGetFieldFromName);
    api.classGetFields = resolveApi<Api::ClassGetFields>(unity, il2cpp, "il2cpp_class_get_fields", unityBase, kClassGetFields);
    api.classGetName = resolveApi<Api::ClassGetName>(unity, il2cpp, "il2cpp_class_get_name", unityBase, kClassGetName);
    api.classGetNamespace = resolveApi<Api::ClassGetNamespace>(unity, il2cpp, "il2cpp_class_get_namespace", unityBase, kClassGetNamespace);
    api.classGetParent = resolveApi<Api::ClassGetParent>(unity, il2cpp, "il2cpp_class_get_parent", unityBase, kClassGetParent);
    api.fieldGetOffset = resolveApi<Api::FieldGetOffset>(unity, il2cpp, "il2cpp_field_get_offset", unityBase, kFieldGetOffset);
    api.fieldGetType = resolveApi<Api::FieldGetType>(unity, il2cpp, "il2cpp_field_get_type", unityBase, kFieldGetType);
    api.fieldStaticGetValue = resolveApi<Api::FieldStaticGetValue>(unity, il2cpp, "il2cpp_field_static_get_value", unityBase, kFieldStaticGetValue);
    api.typeGetName = resolveApi<Api::TypeGetName>(unity, il2cpp, "il2cpp_type_get_name", unityBase, kTypeGetName);
    api.freeFn = resolveApi<Api::Free>(unity, il2cpp, "il2cpp_free", unityBase, kFree);
    api.resolveIcall = resolveApi<Api::ResolveIcall>(unity, il2cpp, "il2cpp_resolve_icall", unityBase, kResolveIcall);
    api.threadAttach = resolveApi<Api::ThreadAttach>(unity, il2cpp, "il2cpp_thread_attach", unityBase, kThreadAttach);

    if (!api.domainGet || !api.domainAssemblyOpen || !api.assemblyGetImage || !api.classFromName ||
        !api.classGetFieldFromName || !api.classGetFields || !api.classGetName || !api.classGetParent ||
        !api.fieldGetOffset || !api.fieldGetType || !api.fieldStaticGetValue || !api.typeGetName ||
        !api.resolveIcall || !api.threadAttach) {
        g_status.store(static_cast<int>(Status::ApiUnavailable), std::memory_order_relaxed);
        return false;
    }

    g_runtime.domain = api.domainGet();
    if (!g_runtime.domain) {
        g_status.store(static_cast<int>(Status::ApiUnavailable), std::memory_order_relaxed);
        return false;
    }
    api.threadAttach(g_runtime.domain);

    const std::vector<const char*> vcontainerImages = {"VContainer.dll", "VContainer"};
    g_runtime.playerLoopHelperClass = findClass(vcontainerImages, "VContainer.Unity", "PlayerLoopHelper");
    g_runtime.tickableLoopItemClass = findClass(vcontainerImages, "VContainer.Unity", "TickableLoopItem");
    if (g_runtime.playerLoopHelperClass) {
        g_runtime.runnersField = api.classGetFieldFromName(g_runtime.playerLoopHelperClass, "Runners");
    }
    if (!g_runtime.playerLoopHelperClass || !g_runtime.tickableLoopItemClass || !g_runtime.runnersField) {
        g_status.store(static_cast<int>(Status::VContainerUnavailable), std::memory_order_relaxed);
        return false;
    }

    const std::vector<const char*> gameImages = {"Axlebolt.Standoff.Game.dll", "Axlebolt.Standoff.Game"};
    appendClass(g_runtime.gameManagerClasses, findClass(gameImages, "", "BCCFCAHEGCBEGCH"));
    appendClass(g_runtime.gameManagerClasses, findClass(gameImages, "", "npo"));

    const std::vector<const char*> playerImages = {
        "Axlebolt.Standoff.Gameplay.Player.dll", "Axlebolt.Standoff.Gameplay.Player",
        "Assembly-CSharp.dll", "Assembly-CSharp", "Axlebolt.Standoff.Player.dll"
    };
    appendClass(g_runtime.playerClasses, findClass(playerImages, "", "HDCFCFEBEGCHHBA"));
    appendClass(g_runtime.playerClasses, findClass(playerImages, "", "ake"));
    appendClass(g_runtime.playerClasses, findClass(playerImages, "Axlebolt.Standoff.Player", "PlayerController"));

    g_runtime.cameraMain = resolveIcallAny<Runtime::CameraMainInjected>(std::array{
        "UnityEngine.Camera::get_main_Injected()",
        "UnityEngine.Camera::get_main_Injected"
    });
    g_runtime.worldToScreen = resolveIcallAny<Runtime::CameraWorldToScreenInjected>(std::array{
        "UnityEngine.Camera::WorldToScreenPoint_Injected(System.IntPtr,UnityEngine.Vector3&,UnityEngine.Camera/MonoOrStereoscopicEye,UnityEngine.Vector3&)",
        "UnityEngine.Camera::WorldToScreenPoint_Injected"
    });
    g_runtime.transformPosition = resolveIcallAny<Runtime::TransformGetPositionInjected>(std::array{
        "UnityEngine.Transform::get_position_Injected(System.IntPtr,UnityEngine.Vector3&)",
        "UnityEngine.Transform::get_position_Injected"
    });
    g_runtime.componentTransform = resolveIcallAny<Runtime::ComponentGetTransformInjected>(std::array{
        "UnityEngine.Component::get_transform_Injected(System.IntPtr)",
        "UnityEngine.Component::get_transform_Injected"
    });
    if (!g_runtime.cameraMain || !g_runtime.worldToScreen || !g_runtime.transformPosition || !g_runtime.componentTransform) {
        g_status.store(static_cast<int>(Status::IcallUnavailable), std::memory_order_relaxed);
        return false;
    }

    g_runtime.ready = true;
    g_status.store(static_cast<int>(Status::WaitingForMatch), std::memory_order_relaxed);
    return true;
}

struct ArrayView {
    std::uintptr_t elements{};
    std::size_t count{};
};

inline bool arrayView(void* object, ArrayView& view) {
    void* klass = objectClass(object);
    const char* name = className(klass);
    if (!name || !nameContains(name, "[]")) return false;
    std::uintptr_t length = 0;
    const auto base = reinterpret_cast<std::uintptr_t>(object);
    if (!readValue(reinterpret_cast<void*>(base + 3 * kPtr), length) || length > 4096) return false;
    const auto elements = base + 4 * kPtr;
    if (length > 0 && !readable(reinterpret_cast<void*>(elements), length * kPtr)) return false;
    view = {elements, static_cast<std::size_t>(length)};
    return true;
}

inline bool collectObjects(void* collection, std::size_t limit, std::vector<void*>& objects) {
    objects.clear();
    if (!collection) return false;
    ArrayView view{};
    if (!arrayView(collection, view)) {
        const auto base = reinterpret_cast<std::uintptr_t>(collection);
        bool found = false;
        for (std::size_t slot = 0; slot < 6 && !found; ++slot) {
            void* candidate = readObject(reinterpret_cast<void*>(base + (2 + slot) * kPtr));
            if (candidate && arrayView(candidate, view)) found = true;
        }
        if (!found) return false;
    }
    const std::size_t count = std::min(view.count, limit);
    objects.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        void* object = nullptr;
        if (!readValue(reinterpret_cast<void*>(view.elements + index * kPtr), object)) continue;
        if (object && readable(object, 2 * kPtr) && objectClass(object)) objects.push_back(object);
    }
    return true;
}

inline bool classMatches(void* object, const std::vector<void*>& classes) {
    void* klass = objectClass(object);
    for (std::size_t depth = 0; klass && depth < 12; ++depth) {
        if (std::find(classes.begin(), classes.end(), klass) != classes.end()) return true;
        klass = g_runtime.api.classGetParent(klass);
    }
    return false;
}

inline bool isPlayerObject(void* object) {
    if (!object) return false;
    if (!g_runtime.playerClasses.empty() && classMatches(object, g_runtime.playerClasses)) return true;
    const char* name = className(objectClass(object));
    if (!name) return false;
    if (nameEquals(name, "HDCFCFEBEGCHHBA") || nameEquals(name, "ake") || nameEquals(name, "PlayerController")) return true;
    return nameContains(name, "Player") && !nameContains(name, "PlayerLoop") && !nameContains(name, "Manager");
}

inline void* findEntityCollection() {
    void* runners = nullptr;
    g_runtime.api.fieldStaticGetValue(g_runtime.runnersField, &runners);
    std::vector<void*> loopRunners;
    if (!collectObjects(runners, 32, loopRunners)) return nullptr;

    std::vector<void*> loopItems;
    std::vector<void*> entries;
    std::vector<void*> entities;
    for (void* loopRunner : loopRunners) {
        void* freeList = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(loopRunner) + 2 * kPtr));
        if (!collectObjects(freeList, 128, loopItems)) continue;
        for (void* loopItem : loopItems) {
            void* klass = objectClass(loopItem);
            const char* name = className(klass);
            if (klass != g_runtime.tickableLoopItemClass && !nameEquals(name, "TickableLoopItem")) continue;
            void* entryList = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(loopItem) + 2 * kPtr));
            if (!collectObjects(entryList, 256, entries)) continue;
            for (void* gameManager : entries) {
                const bool namedManager = classMatches(gameManager, g_runtime.gameManagerClasses);
                const char* managerName = className(objectClass(gameManager));
                const bool fallbackManager = nameEquals(managerName, "BCCFCAHEGCBEGCH") || nameEquals(managerName, "npo");
                void* clientWorld = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(gameManager) + 2 * kPtr));
                void* worldState = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(clientWorld) + 8 * kPtr));
                void* entityList = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(worldState) + 6 * kPtr));
                void* entityCollection = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(entityList) + 3 * kPtr));
                if (!collectObjects(entityCollection, 512, entities)) continue;
                const bool hasPlayer = std::any_of(entities.begin(), entities.end(), [](void* object) { return isPlayerObject(object); });
                if (hasPlayer || ((namedManager || fallbackManager) && !entities.empty())) return entityCollection;
            }
        }
    }
    return nullptr;
}

inline bool unityComponent(void* object) {
    void* klass = objectClass(object);
    for (std::size_t depth = 0; klass && depth < 16; ++depth) {
        const char* name = className(klass);
        if (nameEquals(name, "Component") || nameEquals(name, "Behaviour") || nameEquals(name, "MonoBehaviour") || nameEquals(name, "Transform")) return true;
        klass = g_runtime.api.classGetParent(klass);
    }
    return false;
}

inline std::string fieldTypeName(void* field) {
    if (!field) return {};
    void* type = g_runtime.api.fieldGetType(field);
    if (!type) return {};
    char* raw = g_runtime.api.typeGetName(type);
    if (!raw) return {};
    std::string result(raw);
    if (g_runtime.api.freeFn) g_runtime.api.freeFn(raw);
    return result;
}

inline bool followReferenceType(const std::string& typeName) {
    static constexpr std::array<const char*, 10> tokens = {
        "jsx", "pdd", "Player", "Character", "Binding", "View", "Visual", "Controller", "Presenter", "Transform"
    };
    for (const char* token : tokens) {
        if (typeName.find(token) != std::string::npos) return true;
    }
    return false;
}

inline std::uintptr_t transformFromManaged(void* object, int depth, std::array<void*, 8>& visited, std::size_t& visitedCount) {
    if (!object || depth > 3 || visitedCount >= visited.size()) return 0;
    if (std::find(visited.begin(), visited.begin() + static_cast<std::ptrdiff_t>(visitedCount), object) != visited.begin() + static_cast<std::ptrdiff_t>(visitedCount)) return 0;
    visited[visitedCount++] = object;

    void* klass = objectClass(object);
    const char* ownName = className(klass);
    if (nameEquals(ownName, "Transform")) {
        void* native = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object) + 2 * kPtr));
        return reinterpret_cast<std::uintptr_t>(native);
    }
    if (unityComponent(object)) {
        void* native = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object) + 2 * kPtr));
        if (native) {
            if (std::uintptr_t transform = g_runtime.componentTransform(reinterpret_cast<std::uintptr_t>(native))) return transform;
        }
    }

    for (void* cursor = klass; cursor && depth <= 3; cursor = g_runtime.api.classGetParent(cursor)) {
        void* iterator = nullptr;
        for (std::size_t fieldIndex = 0; fieldIndex < 128; ++fieldIndex) {
            void* field = g_runtime.api.classGetFields(cursor, &iterator);
            if (!field) break;
            const std::size_t offset = g_runtime.api.fieldGetOffset(field);
            if (offset < 2 * kPtr || offset > 0x600) continue;
            const std::string typeName = fieldTypeName(field);
            if (typeName.empty()) continue;
            void* child = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object) + offset));
            if (!child) continue;
            if (typeName.find("UnityEngine.Transform") != std::string::npos || typeName.find("Component.Transform") != std::string::npos) {
                void* native = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(child) + 2 * kPtr));
                if (native) return reinterpret_cast<std::uintptr_t>(native);
            }
            if (depth < 3 && followReferenceType(typeName)) {
                if (std::uintptr_t nested = transformFromManaged(child, depth + 1, visited, visitedCount)) return nested;
            }
        }
    }
    return 0;
}

inline std::uintptr_t transformFromManaged(void* object) {
    std::array<void*, 8> visited{};
    std::size_t count = 0;
    return transformFromManaged(object, 0, visited, count);
}

inline bool finiteVec(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
           std::fabs(value.x) < 100000.0f && std::fabs(value.y) < 100000.0f && std::fabs(value.z) < 100000.0f;
}

inline float distanceSquared(const Vec3& a, const Vec3& b) {
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    const float z = a.z - b.z;
    return x * x + y * y + z * z;
}

inline bool project(std::uintptr_t camera, const Vec3& world, int width, int height, Vec3& screen) {
    screen = {};
    g_runtime.worldToScreen(camera, &world, 0, &screen);
    if (!finiteVec(screen) || screen.z <= 0.05f) return false;
    screen.y = static_cast<float>(height) - screen.y;
    return screen.x > -static_cast<float>(width) && screen.x < static_cast<float>(width) * 2.0f &&
           screen.y > -static_cast<float>(height) && screen.y < static_cast<float>(height) * 2.0f;
}

inline bool produceBoxes(int width, int height, std::vector<ScreenBox>& boxes) {
    boxes.clear();
    void* collection = findEntityCollection();
    if (!collection) {
        g_playerCount.store(0, std::memory_order_relaxed);
        g_status.store(static_cast<int>(Status::WaitingForMatch), std::memory_order_relaxed);
        return true;
    }

    const std::uintptr_t camera = g_runtime.cameraMain();
    if (!camera) {
        g_playerCount.store(0, std::memory_order_relaxed);
        g_status.store(static_cast<int>(Status::WaitingForMatch), std::memory_order_relaxed);
        return true;
    }

    Vec3 cameraPosition{};
    bool haveCameraPosition = false;
    if (const std::uintptr_t cameraTransform = g_runtime.componentTransform(camera)) {
        g_runtime.transformPosition(cameraTransform, &cameraPosition);
        haveCameraPosition = finiteVec(cameraPosition);
    }

    std::vector<void*> entities;
    if (!collectObjects(collection, 512, entities)) return true;
    for (void* entity : entities) {
        if (!isPlayerObject(entity)) continue;
        const std::uintptr_t transform = transformFromManaged(entity);
        if (!transform) continue;
        Vec3 feet{};
        g_runtime.transformPosition(transform, &feet);
        if (!finiteVec(feet)) continue;
        if (haveCameraPosition && distanceSquared(cameraPosition, feet) < 6.25f) continue;
        const Vec3 head{feet.x, feet.y + 1.78f, feet.z};
        Vec3 feetScreen{}, headScreen{};
        if (!project(camera, feet, width, height, feetScreen) || !project(camera, head, width, height, headScreen)) continue;
        const float boxHeight = std::fabs(feetScreen.y - headScreen.y);
        if (boxHeight < 8.0f || boxHeight > static_cast<float>(height) * 1.5f) continue;
        const float boxWidth = boxHeight * 0.48f;
        const float centerX = (feetScreen.x + headScreen.x) * 0.5f;
        boxes.push_back({centerX - boxWidth * 0.5f, std::min(feetScreen.y, headScreen.y),
                         centerX + boxWidth * 0.5f, std::max(feetScreen.y, headScreen.y), true});
        if (boxes.size() >= 64) break;
    }
    g_playerCount.store(static_cast<int>(boxes.size()), std::memory_order_relaxed);
    g_status.store(static_cast<int>(Status::Active), std::memory_order_relaxed);
    return true;
}

}  // namespace detail

inline bool update(std::uintptr_t unityBase, int width, int height, std::vector<ScreenBox>& boxes) {
    using namespace std::chrono;
    const auto now = steady_clock::now();
    detail::refreshMaps();
    if (!detail::g_runtime.ready || detail::g_runtime.unityBase != unityBase) {
        if (now - detail::g_lastRetry < seconds(1)) return false;
        detail::g_lastRetry = now;
        if (!detail::initialize(unityBase)) return false;
    }
    static thread_local bool attached = false;
    if (!attached) {
        detail::g_runtime.api.threadAttach(detail::g_runtime.domain);
        attached = true;
    }
    if (now - detail::g_lastFrame < milliseconds(33)) return true;
    detail::g_lastFrame = now;
    return detail::produceBoxes(width, height, boxes);
}

inline bool ready() {
    return detail::g_runtime.ready;
}

}  // namespace lemming::esp
