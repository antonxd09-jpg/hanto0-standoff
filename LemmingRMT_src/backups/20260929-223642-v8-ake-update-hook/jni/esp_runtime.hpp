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
#include <cerrno>
#include <fcntl.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

#include <android/log.h>

namespace lemming::esp {

inline constexpr const char* kLogTag = "LemmingRMT";

struct ScreenBox {
    float left;
    float top;
    float right;
    float bottom;
    bool enemy;
    bool statsValid;
    bool weaponValid;
    std::uint8_t team;
    std::uint8_t weaponId;
    std::uint8_t magazineAmmo;
    std::uint16_t reserveAmmo;
    std::uint16_t hp;
    std::uint16_t maxHp;
    std::uint16_t armor;
    std::uint16_t maxArmor;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

enum class Status : int {
    WaitingForUnity = 0,
    UnsupportedArchitecture,
    ApiUnavailable,
    TypeInfoUnavailable,
    IcallUnavailable,
    ScanningPlayers,
    WaitingForMatch,
    Active
};

inline std::atomic<int> g_status{static_cast<int>(Status::WaitingForUnity)};
inline std::atomic<int> g_playerCount{0};
inline std::atomic<std::uint64_t> g_initAttempts{0};
inline std::atomic<std::uint64_t> g_scanCycles{0};
inline std::atomic<std::uint64_t> g_scannedBytes{0};
inline std::atomic<int> g_candidateCount{0};
inline std::atomic<int> g_netObjectCount{0};
inline std::atomic<std::uint64_t> g_registryReadFailures{0};
inline std::atomic<int> g_projectedCount{0};
inline std::atomic<int> g_localTeam{0};
inline std::atomic<int> g_statsReadCount{0};
inline std::atomic<int> g_weaponReadCount{0};

inline const char* statusTextFor(Status value) {
    switch (value) {
        case Status::WaitingForUnity: return "Ожидание libunity";
        case Status::UnsupportedArchitecture: return "ESP поддерживает только ARM64";
        case Status::ApiUnavailable: return "IL2CPP API не найден";
        case Status::TypeInfoUnavailable: return "TypeInfo NetObject ещё не готов";
        case Status::IcallUnavailable: return "Unity ICall не найден";
        case Status::ScanningPlayers: return "Поиск ClientWorld / NetObject";
        case Status::WaitingForMatch: return "Ожидание матча";
        case Status::Active: return "Базовый ESP активен";
    }
    return "ESP: неизвестное состояние";
}

inline const char* statusText() {
    return statusTextFor(static_cast<Status>(g_status.load(std::memory_order_relaxed)));
}

inline void setStatus(Status value, const char* detail = nullptr) {
    const int next = static_cast<int>(value);
    const int previous = g_status.exchange(next, std::memory_order_relaxed);
    if (previous != next) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag, "ESP status: %s%s%s",
                            statusTextFor(value), detail ? " | " : "", detail ? detail : "");
    }
}

inline int playerCount() {
    return g_playerCount.load(std::memory_order_relaxed);
}

inline int netObjectCount() {
    return g_netObjectCount.load(std::memory_order_relaxed);
}

inline int playerObjectCount() {
    return g_candidateCount.load(std::memory_order_relaxed);
}

namespace detail {

constexpr std::size_t kPtr = sizeof(void*);

struct MapRange {
    std::uintptr_t begin;
    std::uintptr_t end;
    bool readable;
    bool writable;
    bool scanEligible;
};

inline std::vector<MapRange> g_maps;
inline std::chrono::steady_clock::time_point g_lastMapRefresh{};
inline std::uint64_t g_mapGeneration = 0;

inline void refreshMaps(bool force = false) {
    const auto now = std::chrono::steady_clock::now();
    if (!force && !g_maps.empty() && now - g_lastMapRefresh < std::chrono::seconds(10)) return;

    std::vector<MapRange> next;
    if (FILE* file = std::fopen("/proc/self/maps", "r")) {
        char line[768];
        while (std::fgets(line, sizeof(line), file)) {
            unsigned long long begin = 0;
            unsigned long long end = 0;
            char perms[5]{};
            if (std::sscanf(line, "%llx-%llx %4s", &begin, &end, perms) != 3 || begin >= end) continue;
            const bool readable = perms[0] == 'r';
            const bool writable = perms[1] == 'w';
            const bool isPrivate = perms[3] == 'p';
            const bool fileBacked = std::strchr(line, '/') != nullptr;
            const bool stack = std::strstr(line, "[stack") != nullptr;
            const bool device = std::strstr(line, "/dev/") != nullptr;
            const auto size = static_cast<std::uint64_t>(end - begin);
            const bool eligible = readable && writable && isPrivate && !fileBacked && !stack && !device &&
                                  size >= 0x1000 && size <= (512ull * 1024ull * 1024ull);
            next.push_back({static_cast<std::uintptr_t>(begin), static_cast<std::uintptr_t>(end),
                            readable, writable, eligible});
        }
        std::fclose(file);
    }
    if (!next.empty()) {
        bool changed = next.size() != g_maps.size();
        if (!changed) {
            for (std::size_t i = 0; i < next.size(); ++i) {
                const MapRange& a = next[i];
                const MapRange& b = g_maps[i];
                if (a.begin != b.begin || a.end != b.end || a.readable != b.readable ||
                    a.writable != b.writable || a.scanEligible != b.scanEligible) {
                    changed = true;
                    break;
                }
            }
        }
        if (changed) {
            g_maps.swap(next);
            ++g_mapGeneration;
        }
    }
    g_lastMapRefresh = now;
}

inline bool plausibleAddress(const void* pointer, std::size_t bytes = 1) {
    if (!pointer || bytes == 0) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    return address >= 0x10000 && address + bytes >= address;
}

inline bool readable(const void* pointer, std::size_t bytes = 1) {
    if (!plausibleAddress(pointer, bytes)) return false;
    const auto address = reinterpret_cast<std::uintptr_t>(pointer);
    for (const auto& range : g_maps) {
        if (range.readable && address >= range.begin && address + bytes <= range.end) return true;
    }
    return false;
}

// Direct loads can fault when Unity unmaps a range between /proc/self/maps refreshes.
// Read through the kernel so stale snapshot entries return EFAULT instead of SIGSEGV.
inline int g_memoryReadBackend = 0;  // 0=unknown, 1=process_vm_readv, 2=/proc/self/mem, -1=unavailable
inline int g_selfMemFd = -1;

inline const char* memoryReadBackendName() {
    switch (g_memoryReadBackend) {
        case 1: return "process_vm_readv";
        case 2: return "proc_self_mem";
        case -1: return "unavailable";
        default: return "unknown";
    }
}

inline ssize_t safeReadMemory(void* destination, const void* source, std::size_t bytes) {
    if (!destination || !plausibleAddress(source, bytes)) return -1;

#if defined(__NR_process_vm_readv)
    if (g_memoryReadBackend == 0 || g_memoryReadBackend == 1) {
        iovec local{destination, bytes};
        iovec remote{const_cast<void*>(source), bytes};
        errno = 0;
        const auto result = static_cast<ssize_t>(
            ::syscall(__NR_process_vm_readv, ::getpid(), &local, 1, &remote, 1, 0));
        if (result >= 0) {
            if (g_memoryReadBackend == 0) {
                g_memoryReadBackend = 1;
                __android_log_print(ANDROID_LOG_INFO, kLogTag,
                                    "ESP safe reader ready: backend=%s", memoryReadBackendName());
            }
            return result;
        }
        if (errno != ENOSYS && errno != EPERM && errno != EACCES) return result;
        g_memoryReadBackend = 2;
    }
#else
    if (g_memoryReadBackend == 0) g_memoryReadBackend = 2;
#endif

#if defined(__LP64__)
    if (g_memoryReadBackend == 2) {
        if (g_selfMemFd < 0) {
            g_selfMemFd = ::open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
            if (g_selfMemFd >= 0) {
                __android_log_print(ANDROID_LOG_INFO, kLogTag,
                                    "ESP safe reader ready: backend=%s", memoryReadBackendName());
            }
        }
        if (g_selfMemFd >= 0) {
            return ::pread(g_selfMemFd, destination, bytes,
                           static_cast<off_t>(reinterpret_cast<std::uintptr_t>(source)));
        }
    }
#endif

    g_memoryReadBackend = -1;
    return -1;
}

template <typename T>
inline bool readValue(const void* address, T& value) {
    T copy{};
    if (safeReadMemory(&copy, address, sizeof(T)) != static_cast<ssize_t>(sizeof(T))) return false;
    value = copy;
    return true;
}

inline void* readObject(const void* address) {
    void* value = nullptr;
    if (!readValue(address, value) || !plausibleAddress(value, kPtr)) return nullptr;
    return value;
}

struct Api {
    using DomainGet = void* (*)();
    using DomainAssemblyOpen = void* (*)(void*, const char*);
    using AssemblyGetImage = void* (*)(void*);
    using ClassFromName = void* (*)(void*, const char*, const char*);
    using ClassGetFields = void* (*)(void*, void**);
    using ClassGetName = const char* (*)(void*);
    using ClassGetParent = void* (*)(void*);
    using FieldGetOffset = std::size_t (*)(void*);
    using FieldGetType = void* (*)(void*);
    using TypeGetName = char* (*)(void*);
    using Free = void (*)(void*);
    using ResolveIcall = void* (*)(const char*);
    using ThreadAttach = void* (*)(void*);

    DomainGet domainGet{};
    DomainAssemblyOpen domainAssemblyOpen{};
    AssemblyGetImage assemblyGetImage{};
    ClassFromName classFromName{};
    ClassGetFields classGetFields{};
    ClassGetName classGetName{};
    ClassGetParent classGetParent{};
    FieldGetOffset fieldGetOffset{};
    FieldGetType fieldGetType{};
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
    void* clientWorldClass{};
    void* registryClass{};
    void* storeClass{};
    void* netObjectClass{};
    void* playerClass{};
    void* transformsClass{};
    void* modelClass{};
    void* statsClass{};
    void* weaponPartClass{};
    void* weaponInstanceClass{};

    using CameraMainFn = std::uintptr_t (*)();
    using WorldToScreenFn = void (*)(std::uintptr_t, const Vec3*, int, Vec3*);
    using TransformPositionFn = void (*)(std::uintptr_t, Vec3*);
    using TransformPositionRotationFn = void (*)(std::uintptr_t, Vec3*, Quaternion*);
    using ComponentTransformFn = std::uintptr_t (*)(std::uintptr_t);
    using PlayerPositionFn = Vec3 (*)(void*, const void*);

    CameraMainFn cameraMain{};
    WorldToScreenFn worldToScreen{};
    TransformPositionFn transformPosition{};
    TransformPositionRotationFn transformPositionRotation{};
    ComponentTransformFn componentTransform{};
    PlayerPositionFn playerPosition{};
};

inline Runtime g_runtime;
inline std::chrono::steady_clock::time_point g_lastRetry{};
inline std::chrono::steady_clock::time_point g_lastFrame{};
inline std::chrono::steady_clock::time_point g_lastFrameLog{};

template <typename T>
inline T resolveSymbol(void* unity, const char* name) {
    if (unity) {
        if (void* value = dlsym(unity, name)) return reinterpret_cast<T>(value);
    }
    if (void* value = dlsym(RTLD_DEFAULT, name)) return reinterpret_cast<T>(value);
    return nullptr;
}

template <typename T>
inline T resolveApi(void* unity, const char* name, std::uintptr_t base, std::uintptr_t rva) {
    if (T symbol = resolveSymbol<T>(unity, name)) return symbol;
#if defined(__aarch64__)
    if (base && rva >= offsets::elf::kTextBegin && rva < offsets::elf::kTextEnd) {
        return reinterpret_cast<T>(base + rva);
    }
#else
    (void)base;
    (void)rva;
#endif
    return nullptr;
}

template <typename T>
inline T resolveIcallAny(std::initializer_list<const char*> names) {
    if (!g_runtime.api.resolveIcall) return nullptr;
    for (const char* name : names) {
        if (void* result = g_runtime.api.resolveIcall(name)) return reinterpret_cast<T>(result);
    }
    return nullptr;
}

inline void* objectClass(void* object) {
    return object ? readObject(object) : nullptr;
}

inline const char* className(void* klass) {
    if (!klass || !g_runtime.api.classGetName || !readable(klass, 0x20)) return nullptr;
    return g_runtime.api.classGetName(klass);
}

inline bool nameEquals(const char* value, const char* expected) {
    return value && expected && std::strcmp(value, expected) == 0;
}

inline bool nameContains(const char* value, const char* token) {
    return value && token && std::strstr(value, token) != nullptr;
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

inline void* classFromSlot(std::uintptr_t rva, const char* expectedName) {
#if !defined(__aarch64__)
    (void)rva;
    (void)expectedName;
    return nullptr;
#else
    if (!rva || !g_runtime.unityBase) return nullptr;
    void* klass = readObject(reinterpret_cast<void*>(g_runtime.unityBase + rva));
    if (!klass || !readObject(klass)) return nullptr;
    const char* name = className(klass);
    if (expectedName && !nameEquals(name, expectedName)) return nullptr;
    return klass;
#endif
}

inline bool readPosition(std::uintptr_t transform, Vec3& position) {
    position = {};
    if (!transform) return false;
    if (g_runtime.transformPosition) {
        g_runtime.transformPosition(transform, &position);
    } else if (g_runtime.transformPositionRotation) {
        Quaternion rotation{};
        g_runtime.transformPositionRotation(transform, &position, &rotation);
    } else {
        return false;
    }
    return std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
           std::fabs(position.x) < 100000.0f && std::fabs(position.y) < 100000.0f &&
           std::fabs(position.z) < 100000.0f;
}

inline bool initialize(std::uintptr_t unityBase) {
    ++g_initAttempts;
    refreshMaps(true);
    g_runtime = Runtime{};
    g_runtime.unityBase = unityBase;

#if !defined(__aarch64__)
    setStatus(Status::UnsupportedArchitecture);
    return false;
#else
    if (!unityBase) {
        setStatus(Status::WaitingForUnity);
        return false;
    }

    void* unity = dlopen("libunity.so", RTLD_NOW | RTLD_LOCAL);
    auto& api = g_runtime.api;
    using namespace offsets::il2cpp_api;
    api.domainGet = resolveApi<Api::DomainGet>(unity, "il2cpp_domain_get", unityBase, kDomainGet);
    api.domainAssemblyOpen = resolveApi<Api::DomainAssemblyOpen>(unity, "il2cpp_domain_assembly_open", unityBase, kDomainAssemblyOpen);
    api.assemblyGetImage = resolveApi<Api::AssemblyGetImage>(unity, "il2cpp_assembly_get_image", unityBase, kAssemblyGetImage);
    api.classFromName = resolveApi<Api::ClassFromName>(unity, "il2cpp_class_from_name", unityBase, kClassFromName);
    api.classGetFields = resolveApi<Api::ClassGetFields>(unity, "il2cpp_class_get_fields", unityBase, kClassGetFields);
    api.classGetName = resolveApi<Api::ClassGetName>(unity, "il2cpp_class_get_name", unityBase, kClassGetName);
    api.classGetParent = resolveApi<Api::ClassGetParent>(unity, "il2cpp_class_get_parent", unityBase, kClassGetParent);
    api.fieldGetOffset = resolveApi<Api::FieldGetOffset>(unity, "il2cpp_field_get_offset", unityBase, kFieldGetOffset);
    api.fieldGetType = resolveApi<Api::FieldGetType>(unity, "il2cpp_field_get_type", unityBase, kFieldGetType);
    api.typeGetName = resolveApi<Api::TypeGetName>(unity, "il2cpp_type_get_name", unityBase, kTypeGetName);
    api.freeFn = resolveApi<Api::Free>(unity, "il2cpp_free", unityBase, kFree);
    // The stripped 1.0.0 binary's candidate at kResolveIcall enters a type-name
    // formatter for these strings and crashes in strlen. Keep it disabled and
    // use the independently disassembled native camera/transform entry points.
    api.resolveIcall = nullptr;
    api.threadAttach = resolveApi<Api::ThreadAttach>(unity, "il2cpp_thread_attach", unityBase, kThreadAttach);

    if (!api.domainGet || !api.domainAssemblyOpen || !api.assemblyGetImage || !api.classFromName ||
        !api.classGetFields || !api.classGetName || !api.classGetParent || !api.fieldGetOffset ||
        !api.fieldGetType || !api.typeGetName || !api.threadAttach) {
        setStatus(Status::ApiUnavailable, "required functions missing");
        return false;
    }

    g_runtime.domain = api.domainGet();
    if (!g_runtime.domain) {
        setStatus(Status::ApiUnavailable, "domain is null");
        return false;
    }
    void* attachedThread = api.threadAttach(g_runtime.domain);
    if (!attachedThread) {
        setStatus(Status::ApiUnavailable, "thread attach returned null");
        return false;
    }
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "ESP IL2CPP thread attached: domain=%p thread=%p attachRva=0x%lx",
                        g_runtime.domain, attachedThread,
                        static_cast<unsigned long>(offsets::il2cpp_api::kThreadAttach));

    g_runtime.clientWorldClass = classFromSlot(offsets::typeinfo::kClientWorld, "njh");
    g_runtime.registryClass = classFromSlot(offsets::typeinfo::kRegistry, "gat");
    g_runtime.storeClass = classFromSlot(offsets::typeinfo::kStore, "gzx");
    g_runtime.netObjectClass = classFromSlot(offsets::typeinfo::kNetObject, "mfc");
    g_runtime.playerClass = classFromSlot(offsets::typeinfo::kPlayer, "ake");
    g_runtime.transformsClass = classFromSlot(offsets::typeinfo::kTransforms, "dwc");
    g_runtime.modelClass = classFromSlot(offsets::typeinfo::kModel, "jgi");
    g_runtime.statsClass = classFromSlot(offsets::typeinfo::kStats, "ylw");
    g_runtime.weaponPartClass = classFromSlot(offsets::typeinfo::kWeaponPart, "nrk");
    g_runtime.weaponInstanceClass = classFromSlot(offsets::typeinfo::kWeaponInstance, "qjf");

    const std::vector<const char*> images = {
        "Axlebolt.NetCode.Framework.dll", "Axlebolt.NetCode.Framework",
        "Axlebolt.Standoff.Game.dll", "Axlebolt.Standoff.Game",
        "Axlebolt.Standoff.Gameplay.dll", "Axlebolt.Standoff.Gameplay",
        "Axlebolt.Standoff.Gameplay.Player.dll", "Axlebolt.Standoff.Gameplay.Player",
        "Axlebolt.Standoff.Gameplay.Features.dll", "Axlebolt.Standoff.Gameplay.Features",
        "Assembly-CSharp.dll", "Assembly-CSharp"
    };
    const auto fallbackClass = [&](void*& target, const char* name) {
        if (!target) target = findClass(images, "", name);
    };
    fallbackClass(g_runtime.clientWorldClass, "njh");
    fallbackClass(g_runtime.registryClass, "gat");
    fallbackClass(g_runtime.storeClass, "gzx");
    fallbackClass(g_runtime.netObjectClass, "mfc");
    fallbackClass(g_runtime.playerClass, "ake");
    fallbackClass(g_runtime.transformsClass, "dwc");
    fallbackClass(g_runtime.modelClass, "jgi");
    fallbackClass(g_runtime.statsClass, "ylw");
    fallbackClass(g_runtime.weaponPartClass, "nrk");
    fallbackClass(g_runtime.weaponInstanceClass, "qjf");

    if (!g_runtime.registryClass || !g_runtime.storeClass || !g_runtime.playerClass) {
        setStatus(Status::TypeInfoUnavailable, "gat/gzx/ake class not initialized");
        return false;
    }

    g_runtime.cameraMain = reinterpret_cast<Runtime::CameraMainFn>(
        unityBase + offsets::unity::kCameraGetMain);
    g_runtime.worldToScreen = reinterpret_cast<Runtime::WorldToScreenFn>(
        unityBase + offsets::unity::kCameraWorldToScreen);
    g_runtime.componentTransform = reinterpret_cast<Runtime::ComponentTransformFn>(
        unityBase + offsets::unity::kComponentGetTransform);
    g_runtime.transformPositionRotation = reinterpret_cast<Runtime::TransformPositionRotationFn>(
        unityBase + offsets::unity::kTransformGetPositionAndRotation);
    g_runtime.playerPosition = reinterpret_cast<Runtime::PlayerPositionFn>(
        unityBase + offsets::managed_rva::kPlayerGetPosition);
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "ESP direct helpers ready: cameraMain=0x%lx worldToScreen=0x%lx componentTransform=0x%lx transformPosRot=0x%lx playerPosition=0x%lx",
                        static_cast<unsigned long>(offsets::unity::kCameraGetMain),
                        static_cast<unsigned long>(offsets::unity::kCameraWorldToScreen),
                        static_cast<unsigned long>(offsets::unity::kComponentGetTransform),
                        static_cast<unsigned long>(offsets::unity::kTransformGetPositionAndRotation),
                        static_cast<unsigned long>(offsets::managed_rva::kPlayerGetPosition));

    if (!g_runtime.cameraMain || !g_runtime.worldToScreen || !g_runtime.componentTransform ||
        !g_runtime.playerPosition ||
        (!g_runtime.transformPosition && !g_runtime.transformPositionRotation)) {
        setStatus(Status::IcallUnavailable, "camera/transform helper missing");
        return false;
    }

    g_runtime.ready = true;
    setStatus(Status::ScanningPlayers);
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "ESP resolver ready: profile=%s base=%p clientWorld=%p registry=%p store=%p netObject=%p player=%p transforms=%p model=%p stats=%p weaponPart=%p weapon=%p",
                        offsets::kProfileName, reinterpret_cast<void*>(unityBase),
                        g_runtime.clientWorldClass, g_runtime.registryClass, g_runtime.storeClass,
                        g_runtime.netObjectClass, g_runtime.playerClass, g_runtime.transformsClass,
                        g_runtime.modelClass, g_runtime.statsClass, g_runtime.weaponPartClass,
                        g_runtime.weaponInstanceClass);
    return true;
#endif
}

inline bool classMatches(void* object, void* expected) {
    if (!object || !expected) return false;
    void* klass = objectClass(object);
    for (std::size_t depth = 0; klass && depth < 12; ++depth) {
        if (klass == expected) return true;
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
    static constexpr std::array<const char*, 16> tokens = {
        "dwc", "jgi", "jsx", "pdd", "ake", "Player", "Character", "Binding",
        "View", "Visual", "Controller", "Presenter", "Transform", "Model", "Rig", "Avatar"
    };
    for (const char* token : tokens) {
        if (typeName.find(token) != std::string::npos) return true;
    }
    return false;
}

inline bool unityComponent(void* object) {
    void* klass = objectClass(object);
    for (std::size_t depth = 0; klass && depth < 16; ++depth) {
        const char* name = className(klass);
        if (nameEquals(name, "Component") || nameEquals(name, "Behaviour") ||
            nameEquals(name, "MonoBehaviour") || nameEquals(name, "Transform")) return true;
        klass = g_runtime.api.classGetParent(klass);
    }
    return false;
}

inline std::uintptr_t transformFromManaged(void* object, int depth,
                                           std::array<void*, 12>& visited,
                                           std::size_t& visitedCount) {
    if (!object || depth > 4 || visitedCount >= visited.size()) return 0;
    if (std::find(visited.begin(), visited.begin() + static_cast<std::ptrdiff_t>(visitedCount), object) !=
        visited.begin() + static_cast<std::ptrdiff_t>(visitedCount)) return 0;
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
            if (std::uintptr_t transform = g_runtime.componentTransform(reinterpret_cast<std::uintptr_t>(native))) {
                return transform;
            }
        }
    }

    for (void* cursor = klass; cursor && depth <= 4; cursor = g_runtime.api.classGetParent(cursor)) {
        void* iterator = nullptr;
        for (std::size_t fieldIndex = 0; fieldIndex < 192; ++fieldIndex) {
            void* field = g_runtime.api.classGetFields(cursor, &iterator);
            if (!field) break;
            const std::size_t offset = g_runtime.api.fieldGetOffset(field);
            if (offset < 2 * kPtr || offset > 0x900) continue;
            const std::string typeName = fieldTypeName(field);
            if (typeName.empty()) continue;
            void* child = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object) + offset));
            if (!child || !objectClass(child)) continue;

            if (typeName.find("UnityEngine.Transform") != std::string::npos ||
                nameEquals(className(objectClass(child)), "Transform")) {
                void* native = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(child) + 2 * kPtr));
                if (native) return reinterpret_cast<std::uintptr_t>(native);
            }

            const bool knownPart = objectClass(child) == g_runtime.transformsClass ||
                                   objectClass(child) == g_runtime.modelClass;
            if (depth < 4 && (knownPart || followReferenceType(typeName))) {
                if (std::uintptr_t nested = transformFromManaged(child, depth + 1, visited, visitedCount)) {
                    return nested;
                }
            }
        }
    }
    return 0;
}

inline std::uintptr_t transformFromManaged(void* object) {
    std::array<void*, 12> visited{};
    std::size_t count = 0;
    return transformFromManaged(object, 0, visited, count);
}

inline std::size_t g_scanRangeIndex = 0;
inline std::uintptr_t g_scanAddress = 0;
inline std::uint64_t g_scanMapGeneration = 0;
inline std::vector<MapRange> g_scanMaps;
inline std::vector<std::uintptr_t> g_scanBuffer;
inline std::vector<void*> g_playerObjects;
inline void* g_clientWorldObject = nullptr;
inline void* g_registryObject = nullptr;
inline void* g_storeObject = nullptr;
inline std::chrono::steady_clock::time_point g_scanPauseUntil{};
inline std::chrono::steady_clock::time_point g_lastScanProgressLog{};
inline std::chrono::steady_clock::time_point g_lastNetObjectLog{};
inline std::uint64_t g_scanCycleBytes = 0;
inline std::uint64_t g_scanReadFailures = 0;

inline void resetRootScan() {
    g_scanRangeIndex = 0;
    g_scanAddress = 0;
    g_scanMaps.clear();
    g_scanMaps.reserve(g_maps.size());
    for (const auto& range : g_maps) {
        if (range.scanEligible) g_scanMaps.push_back(range);
    }
    g_scanCycleBytes = 0;
    g_scanMapGeneration = g_mapGeneration;
}

inline bool likelyObjectWords(const std::uintptr_t* words, std::size_t wordCount,
                              std::size_t index, void* expectedClass) {
    if (!words || !expectedClass || index + 1 >= wordCount ||
        words[index] != reinterpret_cast<std::uintptr_t>(expectedClass)) {
        return false;
    }
    const std::uintptr_t monitor = words[index + 1];
    if (monitor == 0) return true;
    return (monitor & (kPtr - 1)) == 0 &&
           plausibleAddress(reinterpret_cast<void*>(monitor), kPtr);
}

inline bool validateStoreLayout(void* store, void*& list, void*& array, int& count) {
    list = nullptr;
    array = nullptr;
    count = 0;
    if (!store || objectClass(store) != g_runtime.storeClass) return false;

    const auto storeBase = reinterpret_cast<std::uintptr_t>(store);
    list = readObject(reinterpret_cast<void*>(storeBase + offsets::field::gzx::kObjects));
    if (!list) return false;

    std::int32_t rawCount = 0;
    const auto listBase = reinterpret_cast<std::uintptr_t>(list);
    if (!readValue(reinterpret_cast<void*>(listBase + offsets::field::yjk::kCount), rawCount) ||
        rawCount < 0 || rawCount > 4096) {
        return false;
    }

    array = readObject(reinterpret_cast<void*>(listBase + offsets::field::yjk::kItems));
    if (rawCount == 0) {
        count = 0;
        return true;
    }
    if (!array) return false;

    std::uintptr_t arrayLength = 0;
    if (!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(array) +
                                           offsets::managed::kArrayLength), arrayLength) ||
        arrayLength < static_cast<std::uintptr_t>(rawCount) || arrayLength > 8192) {
        return false;
    }
    count = rawCount;
    return true;
}

inline bool validateRegistry(void* registry, void*& clientWorld, void*& store,
                             void*& list, void*& array, int& count) {
    clientWorld = nullptr;
    store = nullptr;
    list = nullptr;
    array = nullptr;
    count = 0;
    if (!registry || objectClass(registry) != g_runtime.registryClass) return false;

    const auto registryBase = reinterpret_cast<std::uintptr_t>(registry);
    store = readObject(reinterpret_cast<void*>(registryBase + offsets::field::gat::kStore));
    if (!validateStoreLayout(store, list, array, count)) return false;

    void* world = readObject(reinterpret_cast<void*>(registryBase + offsets::field::gat::kWorld));
    if (world && (!g_runtime.clientWorldClass || objectClass(world) == g_runtime.clientWorldClass)) {
        clientWorld = world;
    }
    return true;
}

inline bool adoptRegistry(void* registry, void* hintedWorld = nullptr) {
    void* world = nullptr;
    void* store = nullptr;
    void* list = nullptr;
    void* array = nullptr;
    int count = 0;
    if (!validateRegistry(registry, world, store, list, array, count)) return false;
    if (!world && hintedWorld &&
        (!g_runtime.clientWorldClass || objectClass(hintedWorld) == g_runtime.clientWorldClass)) {
        world = hintedWorld;
    }

    g_clientWorldObject = world;
    g_registryObject = registry;
    g_storeObject = store;
    g_netObjectCount.store(count, std::memory_order_relaxed);
    g_scanMaps.clear();
    g_scanRangeIndex = 0;
    g_scanAddress = 0;
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "ESP NetObject registry ready: clientWorld=%p registry=%p store=%p objects=%d reader=%s",
                        g_clientWorldObject, g_registryObject, g_storeObject, count,
                        memoryReadBackendName());
    return true;
}

inline bool tryRootCandidate(void* candidate, void* candidateClass) {
    if (!candidate || !candidateClass) return false;
    if (candidateClass == g_runtime.registryClass) return adoptRegistry(candidate);
    if (candidateClass == g_runtime.clientWorldClass) {
        void* registry = readObject(reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(candidate) + offsets::field::njh::kRegistry));
        return adoptRegistry(registry, candidate);
    }
    return false;
}

inline bool scanRegistryStep(std::size_t budgetBytes = 4 * 1024 * 1024) {
    if (g_registryObject) return true;
    if (!g_runtime.registryClass || !g_runtime.storeClass || g_maps.empty()) return false;
    const auto now = std::chrono::steady_clock::now();
    if (now < g_scanPauseUntil) return false;
    if (g_scanMaps.empty()) resetRootScan();

    constexpr std::size_t kMaxReadChunk = 1024 * 1024;
    constexpr std::uintptr_t kPageSize = 4096;
    std::size_t consumed = 0;
    bool found = false;
    while (!found && consumed < budgetBytes && g_scanRangeIndex < g_scanMaps.size()) {
        const MapRange& range = g_scanMaps[g_scanRangeIndex];
        std::uintptr_t cursor = g_scanAddress ? g_scanAddress :
            (range.begin + (kPtr - 1)) & ~(static_cast<std::uintptr_t>(kPtr) - 1);
        if (cursor < range.begin) cursor = range.begin;
        if (!range.scanEligible || cursor >= range.end || range.end - cursor < 2 * kPtr) {
            ++g_scanRangeIndex;
            g_scanAddress = 0;
            continue;
        }

        const std::size_t remainingBudget = budgetBytes - consumed;
        const std::size_t remainingRange = static_cast<std::size_t>(range.end - cursor);
        std::size_t requested = std::min({remainingBudget, remainingRange, kMaxReadChunk});
        requested &= ~(kPtr - 1);
        if (requested < 2 * kPtr) {
            ++g_scanRangeIndex;
            g_scanAddress = 0;
            continue;
        }

        g_scanBuffer.resize(requested / kPtr);
        const ssize_t copiedResult = safeReadMemory(
            g_scanBuffer.data(), reinterpret_cast<const void*>(cursor), requested);
        std::size_t advanced = 0;
        if (copiedResult > 0) {
            const std::size_t copied = std::min(requested, static_cast<std::size_t>(copiedResult));
            const std::size_t wordCount = copied / kPtr;
            for (std::size_t index = 0; index + 1 < wordCount; ++index) {
                void* matchedClass = nullptr;
                if (likelyObjectWords(g_scanBuffer.data(), wordCount, index,
                                      g_runtime.registryClass)) {
                    matchedClass = g_runtime.registryClass;
                } else if (likelyObjectWords(g_scanBuffer.data(), wordCount, index,
                                             g_runtime.clientWorldClass)) {
                    matchedClass = g_runtime.clientWorldClass;
                }
                if (!matchedClass) continue;
                void* candidate = reinterpret_cast<void*>(cursor + index * kPtr);
                if (tryRootCandidate(candidate, matchedClass)) {
                    found = true;
                    break;
                }
            }
            advanced = copied & ~(kPtr - 1);
        }

        if (advanced == 0) {
            ++g_scanReadFailures;
            const std::uintptr_t nextPage = (cursor + kPageSize) & ~(kPageSize - 1);
            const std::size_t pageAdvance = static_cast<std::size_t>(
                std::min(range.end, nextPage > cursor ? nextPage : cursor + kPageSize) - cursor);
            advanced = std::min(requested, std::max<std::size_t>(kPtr, pageAdvance));
        }

        consumed += advanced;
        g_scanCycleBytes += advanced;
        g_scannedBytes.fetch_add(advanced, std::memory_order_relaxed);
        const std::uintptr_t next = cursor + advanced;
        if (!found) {
            if (next >= range.end) {
                ++g_scanRangeIndex;
                g_scanAddress = 0;
            } else {
                g_scanAddress = next;
            }
        }
    }

    if (found) return true;
    if (now - g_lastScanProgressLog >= std::chrono::seconds(5)) {
        g_lastScanProgressLog = now;
        std::uint64_t eligibleBytes = 0;
        for (const auto& range : g_scanMaps) eligibleBytes += range.end - range.begin;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP NetObject root scan: range=%zu/%zu cycleMiB=%.1f eligibleMiB=%.1f snapshotGen=%llu currentGen=%llu reader=%s readFailures=%llu",
                            g_scanRangeIndex, g_scanMaps.size(),
                            static_cast<double>(g_scanCycleBytes) / 1048576.0,
                            static_cast<double>(eligibleBytes) / 1048576.0,
                            static_cast<unsigned long long>(g_scanMapGeneration),
                            static_cast<unsigned long long>(g_mapGeneration),
                            memoryReadBackendName(),
                            static_cast<unsigned long long>(g_scanReadFailures));
    }
    if (g_scanRangeIndex >= g_scanMaps.size()) {
        const auto cycle = g_scanCycles.fetch_add(1, std::memory_order_relaxed) + 1;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP NetObject root scan complete: cycle=%llu no registry, totalMiB=%.1f",
                            static_cast<unsigned long long>(cycle),
                            static_cast<double>(g_scanCycleBytes) / 1048576.0);
        g_scanPauseUntil = now + std::chrono::seconds(1);
        resetRootScan();
    }
    return false;
}

inline void invalidateRegistry(const char* reason) {
    ++g_registryReadFailures;
    __android_log_print(ANDROID_LOG_WARN, kLogTag,
                        "ESP NetObject registry invalidated: reason=%s clientWorld=%p registry=%p store=%p failures=%llu",
                        reason ? reason : "unknown", g_clientWorldObject, g_registryObject,
                        g_storeObject,
                        static_cast<unsigned long long>(
                            g_registryReadFailures.load(std::memory_order_relaxed)));
    g_clientWorldObject = nullptr;
    g_registryObject = nullptr;
    g_storeObject = nullptr;
    g_playerObjects.clear();
    g_netObjectCount.store(0, std::memory_order_relaxed);
    g_candidateCount.store(0, std::memory_order_relaxed);
    resetRootScan();
    g_scanPauseUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
}

inline bool refreshPlayersFromNetObjects() {
    if (!g_registryObject && !scanRegistryStep()) {
        g_playerObjects.clear();
        g_candidateCount.store(0, std::memory_order_relaxed);
        return false;
    }

    void* world = nullptr;
    void* store = nullptr;
    void* list = nullptr;
    void* array = nullptr;
    int count = 0;
    if (!validateRegistry(g_registryObject, world, store, list, array, count)) {
        invalidateRegistry("layout validation failed");
        return false;
    }
    if (world) g_clientWorldObject = world;
    g_storeObject = store;
    g_netObjectCount.store(count, std::memory_order_relaxed);

    std::vector<void*> players;
    if (count > 0) {
        const std::size_t bytes = static_cast<std::size_t>(count) * kPtr;
        std::vector<std::uintptr_t> objects(static_cast<std::size_t>(count));
        const void* data = reinterpret_cast<void*>(
            reinterpret_cast<std::uintptr_t>(array) + offsets::managed::kArrayData);
        const ssize_t copied = safeReadMemory(objects.data(), data, bytes);
        if (copied != static_cast<ssize_t>(bytes)) {
            invalidateRegistry("short NetObject array read");
            return false;
        }

        players.reserve(32);
        for (const std::uintptr_t raw : objects) {
            void* object = reinterpret_cast<void*>(raw);
            if (!plausibleAddress(object, 2 * kPtr)) continue;
            // yjk<krm> stores interface references as ordinary managed object pointers.
            // Filter the registry snapshot by the exact concrete player class (ake).
            if (objectClass(object) != g_runtime.playerClass) continue;
            if (std::find(players.begin(), players.end(), object) == players.end()) {
                players.push_back(object);
                if (players.size() >= 128) break;
            }
        }
    }

    g_playerObjects.swap(players);
    g_candidateCount.store(static_cast<int>(g_playerObjects.size()), std::memory_order_relaxed);
    const auto now = std::chrono::steady_clock::now();
    if (now - g_lastNetObjectLog >= std::chrono::seconds(5)) {
        g_lastNetObjectLog = now;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP NetObject snapshot: clientWorld=%p registry=%p store=%p total=%d ake=%zu",
                            g_clientWorldObject, g_registryObject, g_storeObject, count,
                            g_playerObjects.size());
    }
    return true;
}

struct PlayerSnapshot {
    bool localOwned{};
    bool statsValid{};
    bool weaponValid{};
    std::uint8_t team{};
    std::uint8_t weaponId{};
    std::uint8_t magazineAmmo{};
    std::uint16_t reserveAmmo{};
    std::uint16_t hp{};
    std::uint16_t maxHp{};
    std::uint16_t armor{};
    std::uint16_t maxArmor{};
};

struct PlayerEntry {
    void* object{};
    PlayerSnapshot snapshot{};
};

inline bool validTeam(std::uint8_t team) {
    return team == 1 || team == 2;
}

inline bool matchesClassOrName(void* object, void* expected, const char* fallbackName) {
    if (!object) return false;
    if (expected && classMatches(object, expected)) return true;
    return nameEquals(className(objectClass(object)), fallbackName);
}

inline void* findWeaponPart(void* player) {
    if (!player) return nullptr;
    const auto base = reinterpret_cast<std::uintptr_t>(player);
    void* container = readObject(reinterpret_cast<void*>(base + offsets::field::mfc::kPartsContainer));
    if (!container) return nullptr;
    void* array = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(container) +
                                                    offsets::field::part_container::kItems));
    if (!array) return nullptr;

    std::uintptr_t length = 0;
    if (!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(array) +
                                           offsets::managed::kArrayLength), length) ||
        length == 0 || length > 64) {
        return nullptr;
    }
    const auto data = reinterpret_cast<std::uintptr_t>(array) + offsets::managed::kArrayData;
    if (!readable(reinterpret_cast<void*>(data), static_cast<std::size_t>(length) * kPtr)) return nullptr;
    for (std::uintptr_t i = 0; i < length; ++i) {
        void* part = readObject(reinterpret_cast<void*>(data + i * kPtr));
        if (matchesClassOrName(part, g_runtime.weaponPartClass, "nrk")) return part;
    }
    return nullptr;
}

inline bool readPlayerSnapshot(void* player, PlayerSnapshot& out) {
    out = {};
    if (!player) return false;
    const auto base = reinterpret_cast<std::uintptr_t>(player);
    std::uint8_t localOwned = 0;
    if (!readValue(reinterpret_cast<void*>(base + offsets::field::ake::kTeam), out.team) ||
        !readValue(reinterpret_cast<void*>(base + offsets::field::mfc::kLocalOwned), localOwned)) {
        return false;
    }
    out.localOwned = localOwned != 0;

    void* stats = readObject(reinterpret_cast<void*>(base + offsets::field::ake::kStats));
    if (matchesClassOrName(stats, g_runtime.statsClass, "ylw")) {
        const auto statsBase = reinterpret_cast<std::uintptr_t>(stats);
        const bool readStats =
            readValue(reinterpret_cast<void*>(statsBase + offsets::field::ylw::kHp), out.hp) &&
            readValue(reinterpret_cast<void*>(statsBase + offsets::field::ylw::kMaxHp), out.maxHp) &&
            readValue(reinterpret_cast<void*>(statsBase + offsets::field::ylw::kArmor), out.armor) &&
            readValue(reinterpret_cast<void*>(statsBase + offsets::field::ylw::kMaxArmor), out.maxArmor);
        out.statsValid = readStats && out.maxHp > 0 && out.maxHp <= 10000 && out.hp <= 10000 &&
                         out.maxArmor <= 5000 && out.armor <= 5000;
    }

    void* weaponPart = findWeaponPart(player);
    if (weaponPart) {
        void* weapon = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(weaponPart) +
                                                        offsets::field::nrk::kActiveWeapon));
        if (matchesClassOrName(weapon, g_runtime.weaponInstanceClass, "qjf")) {
            const auto weaponBase = reinterpret_cast<std::uintptr_t>(weapon);
            out.weaponValid =
                readValue(reinterpret_cast<void*>(weaponBase + offsets::field::qjf::kWeaponId), out.weaponId) &&
                readValue(reinterpret_cast<void*>(weaponBase + offsets::field::qjf::kMagazineAmmo), out.magazineAmmo) &&
                readValue(reinterpret_cast<void*>(weaponBase + offsets::field::qjf::kReserveAmmo), out.reserveAmmo) &&
                out.reserveAmmo <= 5000;
        }
    }
    return true;
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
    if (!std::isfinite(screen.x) || !std::isfinite(screen.y) || !std::isfinite(screen.z) || screen.z <= 0.05f) {
        return false;
    }
    screen.y = static_cast<float>(height) - screen.y;
    return screen.x > -static_cast<float>(width) && screen.x < static_cast<float>(width) * 2.0f &&
           screen.y > -static_cast<float>(height) && screen.y < static_cast<float>(height) * 2.0f;
}

inline bool produceBoxes(int width, int height, std::vector<ScreenBox>& boxes) {
    boxes.clear();
    const bool registryReady = refreshPlayersFromNetObjects();

    if (g_playerObjects.empty()) {
        g_playerCount.store(0, std::memory_order_relaxed);
        g_projectedCount.store(0, std::memory_order_relaxed);
        g_localTeam.store(0, std::memory_order_relaxed);
        setStatus(registryReady ? Status::WaitingForMatch : Status::ScanningPlayers);
        return true;
    }

    std::vector<PlayerEntry> entries;
    entries.reserve(g_playerObjects.size());
    int localTeam = 0;
    int statsReads = 0;
    int weaponReads = 0;
    for (void* player : g_playerObjects) {
        if (!classMatches(player, g_runtime.playerClass)) continue;
        PlayerEntry entry{};
        entry.object = player;
        if (!readPlayerSnapshot(player, entry.snapshot)) continue;
        if (entry.snapshot.statsValid) ++statsReads;
        if (entry.snapshot.weaponValid) ++weaponReads;
        if (entry.snapshot.localOwned && validTeam(entry.snapshot.team)) localTeam = entry.snapshot.team;
        entries.push_back(entry);
    }
    if (!validTeam(static_cast<std::uint8_t>(localTeam))) {
        localTeam = g_localTeam.load(std::memory_order_relaxed);
    } else {
        g_localTeam.store(localTeam, std::memory_order_relaxed);
    }
    g_statsReadCount.store(statsReads, std::memory_order_relaxed);
    g_weaponReadCount.store(weaponReads, std::memory_order_relaxed);

    if (!validTeam(static_cast<std::uint8_t>(localTeam))) {
        g_playerCount.store(0, std::memory_order_relaxed);
        g_projectedCount.store(0, std::memory_order_relaxed);
        setStatus(Status::WaitingForMatch, "local team not resolved");
        return true;
    }

    const std::uintptr_t camera = g_runtime.cameraMain();
    if (!camera) {
        setStatus(Status::WaitingForMatch, "main camera is null");
        return true;
    }

    Vec3 cameraPosition{};
    bool haveCameraPosition = false;
    if (const std::uintptr_t cameraTransform = g_runtime.componentTransform(camera)) {
        haveCameraPosition = readPosition(cameraTransform, cameraPosition);
    }

    int validPlayers = 0;
    for (const PlayerEntry& entry : entries) {
        const PlayerSnapshot& snapshot = entry.snapshot;
        if (snapshot.localOwned || !validTeam(snapshot.team) || snapshot.team == localTeam) continue;
        if (snapshot.statsValid && snapshot.hp == 0) continue;

        // xdzx() is ake's dump-confirmed canonical world position. Calling it on a
        // live object from the registry avoids the old guessed transform walk.
        Vec3 feet = g_runtime.playerPosition(entry.object, nullptr);
        if (!std::isfinite(feet.x) || !std::isfinite(feet.y) || !std::isfinite(feet.z) ||
            std::fabs(feet.x) >= 100000.0f || std::fabs(feet.y) >= 100000.0f ||
            std::fabs(feet.z) >= 100000.0f) continue;
        if (haveCameraPosition && distanceSquared(cameraPosition, feet) < 6.25f) continue;

        const Vec3 head{feet.x, feet.y + offsets::managed::kFallbackPlayerHeight, feet.z};
        Vec3 feetScreen{};
        Vec3 headScreen{};
        if (!project(camera, feet, width, height, feetScreen) ||
            !project(camera, head, width, height, headScreen)) continue;

        const float boxHeight = std::fabs(feetScreen.y - headScreen.y);
        if (boxHeight < 8.0f || boxHeight > static_cast<float>(height) * 1.5f) continue;
        const float boxWidth = boxHeight * 0.48f;
        const float centerX = (feetScreen.x + headScreen.x) * 0.5f;
        boxes.push_back({centerX - boxWidth * 0.5f, std::min(feetScreen.y, headScreen.y),
                         centerX + boxWidth * 0.5f, std::max(feetScreen.y, headScreen.y),
                         true, snapshot.statsValid, snapshot.weaponValid, snapshot.team,
                         snapshot.weaponId, snapshot.magazineAmmo, snapshot.reserveAmmo,
                         snapshot.hp, snapshot.maxHp, snapshot.armor, snapshot.maxArmor});
        ++validPlayers;
        if (boxes.size() >= 64) break;
    }

    g_playerCount.store(validPlayers, std::memory_order_relaxed);
    g_projectedCount.store(static_cast<int>(boxes.size()), std::memory_order_relaxed);
    setStatus(boxes.empty() ? Status::WaitingForMatch : Status::Active);

    const auto now = std::chrono::steady_clock::now();
    if (now - g_lastFrameLog >= std::chrono::seconds(5)) {
        g_lastFrameLog = now;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP NetObject frame: total=%d ake=%zu snapshots=%zu localTeam=%d enemies=%d projected=%zu stats=%d weapon=%d camera=%p registry=%p",
                            g_netObjectCount.load(std::memory_order_relaxed),
                            g_playerObjects.size(), entries.size(), localTeam, validPlayers, boxes.size(),
                            statsReads, weaponReads, reinterpret_cast<void*>(camera), g_registryObject);
    }
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

inline void logDiagnostics(const char* reason = "manual") {
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "ESP diagnostics[%s]: profile=%s status=%s ready=%d init=%llu rootScans=%llu scannedMiB=%.1f netObjects=%d ake=%d enemies=%d projected=%d localTeam=%d stats=%d weapon=%d registryFailures=%llu roots[world=%p registry=%p store=%p] classes[world=%p registry=%p store=%p netObject=%p player=%p stats=%p nrk=%p qjf=%p]",
                        reason ? reason : "manual", offsets::kProfileName, statusText(), ready() ? 1 : 0,
                        static_cast<unsigned long long>(g_initAttempts.load(std::memory_order_relaxed)),
                        static_cast<unsigned long long>(g_scanCycles.load(std::memory_order_relaxed)),
                        static_cast<double>(g_scannedBytes.load(std::memory_order_relaxed)) / 1048576.0,
                        g_netObjectCount.load(std::memory_order_relaxed),
                        g_candidateCount.load(std::memory_order_relaxed),
                        g_playerCount.load(std::memory_order_relaxed),
                        g_projectedCount.load(std::memory_order_relaxed),
                        g_localTeam.load(std::memory_order_relaxed),
                        g_statsReadCount.load(std::memory_order_relaxed),
                        g_weaponReadCount.load(std::memory_order_relaxed),
                        static_cast<unsigned long long>(
                            g_registryReadFailures.load(std::memory_order_relaxed)),
                        detail::g_clientWorldObject, detail::g_registryObject, detail::g_storeObject,
                        detail::g_runtime.clientWorldClass, detail::g_runtime.registryClass,
                        detail::g_runtime.storeClass, detail::g_runtime.netObjectClass,
                        detail::g_runtime.playerClass, detail::g_runtime.statsClass,
                        detail::g_runtime.weaponPartClass, detail::g_runtime.weaponInstanceClass);
}

}  // namespace lemming::esp
