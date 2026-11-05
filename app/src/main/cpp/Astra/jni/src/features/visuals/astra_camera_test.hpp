#pragma once
// Third-person ported from the old project's TPS:
//   * game view switch: ake.iwqc(aad ThirdPerson/FirstPerson), byte ake+0x80
//   * camera: player position + camera.forward * -distance, +height, old 0.688 smoothing
// Bound to Standoff 2 1.1.0 (dump/libunity verified).
#include "features/visuals/esp_runtime.hpp"
#include "dobby.h"
#include "imgui.h"
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <android/log.h>

namespace astra_camera_test {

namespace ed = astra::esp::detail;
using V = astra::esp::Vec3;

namespace rva {
inline constexpr std::uintptr_t kSyzn                 = 0xa58d414;
inline constexpr std::uintptr_t kTransformGetPosition = 0x6937790;
inline constexpr std::uintptr_t kTransformSetPosition = 0x69378bc;
inline constexpr std::uintptr_t kTransformGetForward  = 0x694b74c;
inline constexpr std::uintptr_t kAkeSetView           = 0x9c90b68; // ake.iwqc(aad)
}

using FnGetVec = V (*)(void*, const void*);
using FnSetVec = void (*)(void*, const void*, V);
using FnSetView = void (*)(void*, int, const void*);

inline FnGetVec tf_get_position = nullptr;
inline FnSetVec tf_set_position = nullptr;
inline FnGetVec tf_get_forward = nullptr;
inline FnSetView ake_set_view = nullptr;

inline void (*orig_syzn)(void*, const void*) = nullptr;
inline std::atomic<int> state_syzn{0}, result_syzn{0};
inline std::atomic<bool> enabled{false}, restore_requested{false};
inline std::atomic<long long> lease{0};
inline std::atomic<std::uintptr_t> base_seen{0}, last_syzn_obj{0};
inline std::atomic<unsigned long long> calls_syzn{0}, applied_camera{0}, rejected_camera{0};
inline std::atomic<unsigned long long> view_sets{0};

inline std::mutex lock;
inline V settings{0.0f, 1.5f, 3.0f}; // x unused, y height, z distance
inline V smoothed{};
inline bool smooth_valid = false;
inline std::uintptr_t smooth_player = 0;
inline V last_before{}, last_after{}, last_forward{};
inline bool last_camera_valid = false;
inline int last_view_mode = -1;
inline int fps_mode = -1;
inline int tps_mode = -1;
inline char save_status[384] = "Not saved";

inline long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline bool finite3(V v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
           std::fabs(v.x) < 100000.f && std::fabs(v.y) < 100000.f && std::fabs(v.z) < 100000.f;
}
inline bool validPtr(std::uintptr_t p) {
    return p != 0 && (p & 7) == 0 && p > 0x10000ULL && p < 0x7FFFFFFFE000ULL;
}
inline bool name_equals(void* object, const char* expected) {
    if (!object || !expected) return false;
    void* klass = ed::objectClass(object);
    if (!klass) return false;
    const char* name = ed::className(klass);
    if (!name) return false;
    for (std::size_t i = 0; i < 64; ++i) {
        char c = 0;
        if (!ed::readValue(name + i, c) || c != expected[i]) return false;
        if (!c) return true;
    }
    return false;
}

inline bool resolveFns(std::uintptr_t base) {
    if (!base) return false;
    tf_get_position = reinterpret_cast<FnGetVec>(base + rva::kTransformGetPosition);
    tf_set_position = reinterpret_cast<FnSetVec>(base + rva::kTransformSetPosition);
    tf_get_forward = reinterpret_cast<FnGetVec>(base + rva::kTransformGetForward);
    ake_set_view = reinterpret_cast<FnSetView>(base + rva::kAkeSetView);
    return tf_get_position && tf_set_position && tf_get_forward && ake_set_view;
}

inline int readViewMode(void* player) {
    if (!player) return -1;
    std::uint8_t mode = 0;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player) + 0x80), mode))
        return -1;
    return mode;
}

inline void setViewMode(void* player, int mode) {
    if (!player || !ake_set_view || mode < 0) return;
    ake_set_view(player, mode, nullptr);
    view_sets.fetch_add(1, std::memory_order_relaxed);
}

inline void rememberModes(void* player) {
    if (fps_mode >= 0 && tps_mode >= 0) return;
    const int current = readViewMode(player);
    if (current < 0) return;
    last_view_mode = current;
    // Default state is first person. Two-value enum: the other value is TPS.
    fps_mode = current;
    tps_mode = (current == 0) ? 1 : 0;
}

inline void resetSmoothing() {
    std::lock_guard<std::mutex> g(lock);
    smoothed = V{};
    smooth_valid = false;
    smooth_player = 0;
}

inline void applyCameraOffset(void* self) {
    if (!self || !tf_get_position || !tf_set_position || !tf_get_forward) {
        ++rejected_camera; return;
    }
    void* transform = nullptr;
    if (!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(self) + 0x18), transform) ||
        !validPtr(reinterpret_cast<std::uintptr_t>(transform)) ||
        !name_equals(transform, "Transform")) {
        ++rejected_camera; return;
    }
    void* local = nullptr;
    {
        std::lock_guard<std::mutex> g(ed::g_hookPlayerMutex);
        local = ed::g_hookLocalPlayer;
    }
    if (!local) { ++rejected_camera; return; }

    V playerPosition{};
    if (!ed::readPlayerPoseSnapshot(local, playerPosition) || !finite3(playerPosition)) {
        ++rejected_camera; return;
    }
    V cameraBefore = tf_get_position(transform, nullptr);
    V forward = tf_get_forward(transform, nullptr);
    if (!finite3(forward)) { ++rejected_camera; return; }
    const float len2 = forward.x * forward.x + forward.y * forward.y + forward.z * forward.z;
    if (!(len2 > 0.01f)) { ++rejected_camera; return; }

    V s;
    {
        std::lock_guard<std::mutex> g(lock);
        s = settings;
    }
    // Exactly the old TPS target:
    // pos + forward * -distance; y = player.y + height + forward.y * -distance.
    V target{
        playerPosition.x - forward.x * s.z,
        playerPosition.y + s.y - forward.y * s.z,
        playerPosition.z - forward.z * s.z
    };
    if (!finite3(target)) { ++rejected_camera; return; }

    V next = target;
    {
        std::lock_guard<std::mutex> g(lock);
        if (!smooth_valid || smooth_player != reinterpret_cast<std::uintptr_t>(local)) {
            smoothed = target;
            smooth_valid = true;
            smooth_player = reinterpret_cast<std::uintptr_t>(local);
        } else {
            const float a = std::max(0.05f, std::min(0.99f, 0.688f));
            smoothed.x += (target.x - smoothed.x) * a;
            smoothed.y += (target.y - smoothed.y) * a;
            smoothed.z += (target.z - smoothed.z) * a;
        }
        next = smoothed;
        last_before = cameraBefore;
        last_forward = forward;
        last_after = next;
        last_camera_valid = true;
    }
    if (!finite3(next)) { ++rejected_camera; return; }
    tf_set_position(transform, nullptr, next);
    ++applied_camera;
}

inline void hook_syzn(void* self, const void* method) {
    calls_syzn.fetch_add(1, std::memory_order_relaxed);
    auto fn = orig_syzn;
    if (!fn) return;
    fn(self, method);

    if (!enabled.load(std::memory_order_relaxed)) {
        if (restore_requested.exchange(false)) {
            void* local = nullptr;
            {
                std::lock_guard<std::mutex> g(ed::g_hookPlayerMutex);
                local = ed::g_hookLocalPlayer;
            }
            if (local && fps_mode >= 0) setViewMode(local, fps_mode);
            resetSmoothing();
        }
        return;
    }
    const auto age = nowMs() - lease.load(std::memory_order_relaxed);
    if (age < 0 || age > 500 || !ed::isUnityMainCallback()) return;
    applyCameraOffset(self);
}

inline void onPlayer(void* player) {
#ifdef __aarch64__
    if (!player || !ed::isUnityMainCallback() || !ed::g_runtime.ready) return;
    if (ed::objectClass(player) != ed::g_runtime.playerClass) return;
    auto p = reinterpret_cast<std::uintptr_t>(player);
    std::uint8_t local = 0;
    if (!ed::readValue(reinterpret_cast<void*>(p + astra::offsets::field::mfc::kLocalOwned), local) || !local) return;
    if (!ed::readObject(reinterpret_cast<void*>(p + astra::offsets::field::mfc::kWorld))) return;

    base_seen.store(ed::g_runtime.unityBase);
    resolveFns(ed::g_runtime.unityBase);
    last_syzn_obj.store(p);

    int expected = 0;
    if (state_syzn.compare_exchange_strong(expected, 1)) {
        auto target = reinterpret_cast<void*>(ed::g_runtime.unityBase + rva::kSyzn);
        const std::array<std::uint32_t, 4> wanted{0xfc1d0fea, 0x6d00a3e9, 0xf9000ffe, 0xa9024ff4};
        std::array<std::uint32_t, 4> actual{};
        bool read = ed::readValue(target, actual);
        if (!read || actual != wanted) {
            state_syzn.store(-1);
        } else {
            int r = DobbyHook(target, reinterpret_cast<void*>(&hook_syzn),
                              reinterpret_cast<void**>(&orig_syzn));
            result_syzn.store(r);
            state_syzn.store(r == 0 && orig_syzn ? 2 : -2);
            __android_log_print(ANDROID_LOG_INFO, "AstraCamV3",
                                "hook=%d result=%d base=%p", state_syzn.load(), r,
                                reinterpret_cast<void*>(ed::g_runtime.unityBase));
        }
    }

    if (enabled.load(std::memory_order_relaxed)) {
        lease.store(nowMs(), std::memory_order_relaxed);
        rememberModes(player);
        const int current = readViewMode(player);
        last_view_mode = current;
        if (tps_mode >= 0 && current != tps_mode)
            setViewMode(player, tps_mode);
        if (smooth_player != p) resetSmoothing();
    } else if (restore_requested.exchange(false)) {
        if (fps_mode >= 0) setViewMode(player, fps_mode);
        resetSmoothing();
    }
#else
    (void)player;
    state_syzn.store(-3);
#endif
}

inline void save() {
    V s, before, after, forward;
    {
        std::lock_guard<std::mutex> g(lock);
        s = settings; before = last_before; after = last_after; forward = last_forward;
    }
    char report[4096];
    std::snprintf(report, sizeof(report),
        "AstraCamV3-TPS\n"
        "hook=%d result=%d base=%p local=%p lease=%lld calls=%llu\n"
        "camera applied=%llu rejected=%llu valid=%d\n"
        "settings(distance=%.2f height=%.2f)\n"
        "player_pos=(%.3f %.3f %.3f) forward=(%.3f %.3f %.3f)\n"
        "camera_before=(%.3f %.3f %.3f) camera_after=(%.3f %.3f %.3f)\n"
        "view mode=%d fps=%d tps=%d sets=%llu\n",
        state_syzn.load(), result_syzn.load(),
        reinterpret_cast<void*>(base_seen.load()),
        reinterpret_cast<void*>(last_syzn_obj.load()),
        nowMs() - lease.load(), calls_syzn.load(),
        applied_camera.load(), rejected_camera.load(), last_camera_valid,
        s.z, s.y,
        before.x, before.y, before.z, forward.x, forward.y, forward.z,
        last_before.x, last_before.y, last_before.z, after.x, after.y, after.z,
        last_view_mode, fps_mode, tps_mode, view_sets.load());
    __android_log_print(ANDROID_LOG_INFO, "AstraCamV3", "%s", report);

    char path[260];
    const char* dirs[] = {"/storage/emulated/0/Download", "/storage/emulated/0",
                          "/sdcard/Download", "/data/local/tmp"};
    FILE* file = nullptr;
    for (const char* dir : dirs) {
        std::snprintf(path, sizeof(path), "%s/astra_tps_%lld.txt", dir, nowMs());
        file = std::fopen(path, "w");
        if (file) break;
    }
    if (!file) {
        std::snprintf(save_status, sizeof(save_status), "Save failed: %s", std::strerror(errno));
        return;
    }
    const std::size_t length = std::strlen(report);
    const bool ok = std::fwrite(report, 1, length, file) == length;
    const int closed = std::fclose(file);
    std::snprintf(save_status, sizeof(save_status), "%s: %s",
                  ok && closed == 0 ? "Saved" : "Write failed", path);
}

inline void reset() {
    enabled.store(false);
    restore_requested.store(true);
    lease.store(0);
    resetSmoothing();
    std::lock_guard<std::mutex> g(lock);
    settings = V{0.0f, 1.5f, 3.0f};
    last_before = V{};
    last_after = V{};
    last_forward = V{};
    last_camera_valid = false;
}

inline void controls() {
    ImGui::PushID("astra-cam-v3");
    bool on = enabled.load();
    if (ImGui::Checkbox("Third person",&on)) {
        enabled.store(on);
        restore_requested.store(!on);
        if (on) lease.store(nowMs());
        resetSmoothing();
    }

    V s;
    { std::lock_guard<std::mutex> g(lock); s = settings; }
    bool changed = false;
    changed |= ImGui::SliderFloat("Distance",&s.z,0.0f,6.0f,"%.2f");
    changed |= ImGui::SliderFloat("Height",&s.y,0.0f,3.0f,"%.2f");
    if (changed) { std::lock_guard<std::mutex> g(lock); settings = s; }

    ImGui::Spacing();
    const int state = state_syzn.load();
    ImGui::Text("Hook:%s(%d)  cam:%llu  view:%d->%d",
                state == 2 ? "OK" : state == -1 ? "mis" : state == -2 ? "fail"
                         : state == 1 ? "wait" : "off",
                state, applied_camera.load(), last_view_mode, tps_mode);
    ImGui::Text("Modes: fps=%d tps=%d  sets:%llu", fps_mode, tps_mode, view_sets.load());
    ImGui::Text("Camera valid:%d  rej:%llu", int(last_camera_valid), rejected_camera.load());
    if (ImGui::Button("Save diagnostics")) save();
    ImGui::TextWrapped("%s", save_status);
    ImGui::PopID();
}

} // namespace astra_camera_test
