from pathlib import Path
from datetime import datetime
import shutil, sys, re, json, hashlib

root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('/home/leftcode/Projects/LemmingRMT')
old_profile = 'Standoff2-1.0.0-arm64-il2cppdump-v8-method-slot-camera-snapshot'
new_profile = 'Standoff2-1.0.0-arm64-il2cppdump-v8-native-camera-cache'
names = ['jni/esp_runtime.hpp', 'jni/game_offsets.hpp', 'README.md', 'docs/ADB_DEBUG.md', 'Standoff2_1.0.0_ARM64_internal_offsets.txt']
originals = {name:(root/name).read_text() for name in names}
s = originals['jni/esp_runtime.hpp']
h = originals['jni/game_offsets.hpp']
assert old_profile in h, 'Unexpected source profile; no files changed'

def replace_once(text, old, new):
    assert text.count(old) == 1, f'Expected exactly one match: {old[:100]!r}, got {text.count(old)}'
    return text.replace(old, new, 1)

h = replace_once(h, old_profile, new_profile)
h = replace_once(h, 'inline constexpr std::uintptr_t kCameraGetMain = 0x53653C4;', '''// 0x53653C4 returns a GC handle, NOT a managed Camera object.
inline constexpr std::uintptr_t kCameraGetMain = 0x53653C4;  // diagnostics only
// Native Camera::GetMain; verified call target of the GC-handle wrapper.
inline constexpr std::uintptr_t kNativeCameraGetMain = 0x54C1338;''')
s = replace_once(s, 'inline int g_memoryReadBackend = 0;', 'inline std::atomic<int> g_memoryReadBackend{0};')
s = replace_once(s, 'inline int g_selfMemFd = -1;', 'inline std::atomic<int> g_selfMemFd{-1};\ninline std::mutex g_memoryReaderMutex;')
s = replace_once(s, '    if (g_memoryReadBackend == 2) {\n        if (g_selfMemFd < 0) {', '    if (g_memoryReadBackend == 2) {\n        std::lock_guard<std::mutex> lock(g_memoryReaderMutex);\n        if (g_selfMemFd < 0) {')
s = replace_once(s, '    CameraMainFn cameraMain{};', '    CameraMainFn cameraMain{};  // native Camera*, never a GC handle')
s = replace_once(s, '''    g_runtime.cameraMain = reinterpret_cast<Runtime::CameraMainFn>(
        unityBase + offsets::unity::kCameraGetMain);
    g_runtime.worldToCameraMatrix = reinterpret_cast<Runtime::CameraMatrixFn>(
        unityBase + offsets::unity::kCameraWorldToCameraMatrix);
    g_runtime.projectionMatrix = reinterpret_cast<Runtime::CameraMatrixFn>(
        unityBase + offsets::unity::kCameraProjectionMatrix);''', '''    g_runtime.cameraMain = reinterpret_cast<Runtime::CameraMainFn>(
        unityBase + offsets::unity::kNativeCameraGetMain);
    // Copy the game's already-updated caches. Do not trigger native Transform
    // recalculation through either matrix getter, even on UnityMain.
    g_runtime.worldToCameraMatrix = nullptr;
    g_runtime.projectionMatrix = nullptr;''')
s = replace_once(s, 'ESP direct helpers ready: cameraMain=0x%lx viewMatrix=0x%lx projectionMatrix=0x%lx', 'ESP direct helpers ready: nativeCameraMain=0x%lx viewMatrix(disabled)=0x%lx projectionMatrix(disabled)=0x%lx')
s = replace_once(s, 'static_cast<unsigned long>(offsets::unity::kCameraGetMain),', 'static_cast<unsigned long>(offsets::unity::kNativeCameraGetMain),')
s = replace_once(s, '    if (!g_runtime.cameraMain || !g_runtime.worldToCameraMatrix || !g_runtime.projectionMatrix ||\n', '    if (!g_runtime.cameraMain ||\n')
s = replace_once(s, 'inline constexpr std::size_t kRuntimeVtableSearchEntries = 128;', 'inline constexpr std::size_t kRuntimeVtableSearchEntries = 256;')
s = replace_once(s, '''// Camera APIs are never invoked by UnityGfxDeviceW. The verified ake callback
// runs in the game's own managed execution path; only a UnityMain invocation is
// allowed to refresh matrices. The render hook consumes this immutable snapshot.''', '''// UnityMain obtains a typed native Camera* and safely copies its matrix caches.
// eglSwapBuffers only consumes these snapshots. No matrix/W2S/Transform getters
// are called for the camera on any thread.''')
start = s.index('inline void captureCameraSnapshotFromPlayerCallback() {')
end = s.index('\ninline void trackPlayerFromUpdateHook(void* player) {', start)
s = s[:start] + r'''inline bool isUnityMainCallback() {
    char name[16]{};
    (void)prctl(PR_GET_NAME, name, 0, 0, 0);
    return std::strncmp(name, "UnityMain", 9) == 0;
}

inline void captureCameraSnapshotFromPlayerCallback() {
    if (!isUnityMainCallback()) {
        if (!g_nonUnityMainHookThreadLogged.exchange(true, std::memory_order_relaxed)) {
            __android_log_print(ANDROID_LOG_WARN, kLogTag,
                                "ESP camera capture blocked on non-game thread: tid=%d",
                                static_cast<int>(syscall(SYS_gettid)));
        }
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> captureLock(g_cameraCaptureMutex, std::try_to_lock);
    if (!captureLock.owns_lock() ||
        now - g_lastCameraCaptureAttempt < std::chrono::milliseconds(16)) return;
    g_lastCameraCaptureAttempt = now;
    std::uintptr_t nativeCamera = 0;
    auto fail = [&](const char* reason) {
        const auto failures = g_cameraSnapshotFailures.fetch_add(1, std::memory_order_relaxed) + 1;
        if (failures == 1 || now - g_lastCameraCaptureLog >= std::chrono::seconds(5)) {
            g_lastCameraCaptureLog = now;
            __android_log_print(ANDROID_LOG_WARN, kLogTag,
                "ESP camera cache rejected: reason=%s native=%p failures=%llu tid=%d",
                reason, reinterpret_cast<void*>(nativeCamera),
                static_cast<unsigned long long>(failures), static_cast<int>(syscall(SYS_gettid)));
        }
    };
    if (!g_runtime.cameraMain) { fail("missing-native-main"); return; }
    // 0x54C1338 returns Camera*. 0x53653C4 returns a GC handle, so reading
    // handle+0x10 was the precise source of the 2026-09-30 UnityMain crash.
    nativeCamera = g_runtime.cameraMain();
    if (!nativeCamera) { fail("no-main-camera"); return; }
    std::uintptr_t vtable = 0;
    std::uintptr_t gameObject = 0;
    std::uintptr_t activeMethod = 0;
    if (!readValue(reinterpret_cast<void*>(nativeCamera), vtable) ||
        vtable < g_runtime.unityBase + offsets::elf::kDataRelRoBegin ||
        vtable >= g_runtime.unityBase + offsets::elf::kDataEnd ||
        !readValue(reinterpret_cast<void*>(vtable + 0xD0), activeMethod) ||
        activeMethod < g_runtime.unityBase + offsets::elf::kTextBegin ||
        activeMethod >= g_runtime.unityBase + offsets::elf::kTextEnd ||
        !readValue(reinterpret_cast<void*>(nativeCamera + 0x20), gameObject) ||
        !plausibleAddress(reinterpret_cast<void*>(gameObject), kPtr)) {
        fail("native-camera-header"); return;
    }
    CameraMatrixCache candidate{};
    if (!readValue(reinterpret_cast<void*>(nativeCamera +
            offsets::managed::kNativeCameraWorldToCameraMatrix), candidate)) {
        fail("matrix-copy"); return;
    }
    std::uintptr_t currentVtable = 0;
    std::uintptr_t currentGameObject = 0;
    if (!readValue(reinterpret_cast<void*>(nativeCamera), currentVtable) ||
        !readValue(reinterpret_cast<void*>(nativeCamera + 0x20), currentGameObject) ||
        currentVtable != vtable || currentGameObject != gameObject) {
        fail("camera-changed-during-copy"); return;
    }
    const auto& view = candidate.worldToCamera.value;
    if (!cameraMatrixFinite(candidate.worldToCamera) ||
        !cameraMatrixFinite(candidate.projection) ||
        std::fabs(view[3]) > 0.001f || std::fabs(view[7]) > 0.001f ||
        std::fabs(view[11]) > 0.001f || std::fabs(view[15] - 1.0f) > 0.01f ||
        std::fabs(candidate.projection.value[0]) < 0.001f ||
        std::fabs(candidate.projection.value[5]) < 0.001f) {
        fail("invalid-or-uninitialized-matrices"); return;
    }
    const std::uint64_t captures =
        g_cameraSnapshotCaptures.fetch_add(1, std::memory_order_relaxed) + 1;
    g_cameraSnapshotTid.store(static_cast<pid_t>(syscall(SYS_gettid)), std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> snapshotLock(g_cameraSnapshotMutex);
        g_cameraSnapshot = candidate;
        g_cameraSnapshotObject = 0;  // no managed Camera reference is retained
        g_cameraSnapshotNative = nativeCamera;
        g_cameraSnapshotTime = now;
        g_cameraSnapshotReady.store(true, std::memory_order_release);
    }
    if (captures == 1 || now - g_lastCameraCaptureLog >= std::chrono::seconds(5)) {
        g_lastCameraCaptureLog = now;
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
            "ESP UnityMain camera snapshot: source=native-cache tid=%d captures=%llu native=%p gameObject=%p getters=0 viewDiag=(%.3f,%.3f,%.3f,%.3f) projDiag=(%.3f,%.3f,%.3f,%.3f)",
            static_cast<int>(syscall(SYS_gettid)), static_cast<unsigned long long>(captures),
            reinterpret_cast<void*>(nativeCamera), reinterpret_cast<void*>(gameObject),
            view[0], view[5], view[10], view[15],
            candidate.projection.value[0], candidate.projection.value[5],
            candidate.projection.value[10], candidate.projection.value[15]);
    }
}

struct PlayerPoseCache {
    void* player{};
    void* world{};
    Vec3 position{};
    std::chrono::steady_clock::time_point captured{};
};
inline std::mutex g_playerPoseMutex;
inline std::array<PlayerPoseCache, 128> g_playerPoses{};
inline std::atomic<std::uint64_t> g_playerPoseCaptures{0};

// Only a live game's own callback may invoke ake::xdzx(). The render thread
// consumes copied positions and never follows Unity's native Transform chain.
inline void capturePlayerPoseFromCallback(void* player) {
    if (!player || !g_runtime.playerPosition || !isUnityMainCallback()) return;
    if (objectClass(player) != g_runtime.playerClass) return;
    const auto base = reinterpret_cast<std::uintptr_t>(player);
    void* world = readObject(reinterpret_cast<void*>(base + offsets::field::mfc::kWorld));
    void* parts = readObject(reinterpret_cast<void*>(base + offsets::field::mfc::kPartsContainer));
    if (!world || !parts || !g_runtime.transformsClass) return;
    void* list = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(parts) +
                                                    offsets::field::part_container::kPartList));
    if (!list) return;
    const auto listBase = reinterpret_cast<std::uintptr_t>(list);
    std::int32_t count = 0;
    if (!readValue(reinterpret_cast<void*>(listBase + offsets::field::managed_list::kSize), count) ||
        count <= 0 || count > 64) return;
    void* array = readObject(reinterpret_cast<void*>(listBase + offsets::field::managed_list::kItems));
    if (!array) return;
    const auto arrayBase = reinterpret_cast<std::uintptr_t>(array);
    std::uintptr_t length = 0;
    if (!readValue(reinterpret_cast<void*>(arrayBase + offsets::managed::kArrayLength), length) ||
        length < static_cast<std::uintptr_t>(count) || length > 256) return;
    bool transformsPresent = false;
    for (std::int32_t i = 0; i < count; ++i) {
        void* part = readObject(reinterpret_cast<void*>(arrayBase + offsets::managed::kArrayData +
                                                       static_cast<std::size_t>(i) * kPtr));
        if (objectClass(part) == g_runtime.transformsClass) { transformsPresent = true; break; }
    }
    if (!transformsPresent) return;
    const Vec3 position = g_runtime.playerPosition(player, nullptr);
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
        std::fabs(position.x) >= 100000.0f || std::fabs(position.y) >= 100000.0f ||
        std::fabs(position.z) >= 100000.0f) return;
    const auto now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(g_playerPoseMutex);
        PlayerPoseCache* destination = nullptr;
        for (auto& pose : g_playerPoses) {
            if (pose.player == player) { destination = &pose; break; }
            if (!destination && (!pose.player || now - pose.captured > std::chrono::seconds(1)))
                destination = &pose;
        }
        if (!destination) return;
        *destination = {player, world, position, now};
    }
    const auto captures = g_playerPoseCaptures.fetch_add(1, std::memory_order_relaxed) + 1;
    if (captures == 1) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
            "ESP UnityMain position snapshot: player=%p world=(%.3f,%.3f,%.3f) tid=%d renderGetters=0",
            player, position.x, position.y, position.z, static_cast<int>(syscall(SYS_gettid)));
    }
}

inline bool readPlayerPoseSnapshot(void* player, Vec3& position) {
    if (!player) return false;
    const auto now = std::chrono::steady_clock::now();
    void* world = readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player) +
                                                    offsets::field::mfc::kWorld));
    if (!world) return false;
    std::lock_guard<std::mutex> lock(g_playerPoseMutex);
    for (const auto& pose : g_playerPoses) {
        if (pose.player == player && pose.world == world &&
            now - pose.captured < std::chrono::milliseconds(500)) {
            position = pose.position;
            return true;
        }
    }
    return false;
}
''' + s[end:]
s = replace_once(s, '''    const bool localOwned = *reinterpret_cast<volatile std::uint8_t*>(
        reinterpret_cast<std::uintptr_t>(player) + offsets::field::mfc::kLocalOwned) != 0;''', '''    std::uint8_t localFlag = 0;
    if (!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player) +
                                          offsets::field::mfc::kLocalOwned), localFlag)) return;
    const bool localOwned = localFlag != 0;''')
s = replace_once(s, '''    // Capture after the original tick so transforms/camera state are current.
    captureCameraSnapshotFromPlayerCallback();''', '''    // Capture only on the live game's thread, after the original callback.
    capturePlayerPoseFromCallback(player);
    captureCameraSnapshotFromPlayerCallback();''')
s = replace_once(s, '''        const bool exactMethodInfo = methodInfo == targetMethodInfo ||
                                     methodInfoMatchesTarget(methodInfo);''', '''        const bool exactMethodInfo =
            (targetMethodInfo && methodInfo == targetMethodInfo) || methodInfoMatchesTarget(methodInfo);''')
s = replace_once(s, '''    if (targetMethodInfo && targetMethodSlot != kInvalidMethodSlot &&
        targetMethodSlot < kMaxVtableEntries) {''', '''    if (targetMethodInfo && targetMethodClass == classBase &&
        std::strcmp(targetMethodName, "opmn") == 0 &&
        targetMethodSlot != kInvalidMethodSlot && targetMethodSlot < kMaxVtableEntries) {''')
s = replace_once(s, '''        // xdzx() is ake's canonical world position and performs the same typed
        // rct part lookup used by the game. Call it only with a live parts owner.
        Vec3 feet = g_runtime.playerPosition(entry.object, nullptr);''', '''        // No managed/native position getter is allowed on UnityGfxDeviceW.
        Vec3 feet{};
        if (!readPlayerPoseSnapshot(entry.object, feet)) { ++skipBadPosition; continue; }''')
s = replace_once(s, '''            } else {
                ++iterator;
            }
        }
        result = g_hookPlayerObjects;''', '''            } else {
                ++iterator;
            }
        }
        result = g_hookPlayerObjects;''')  # ensures the cleanup anchor still exists
# Diagnostic counters must not preserve a stale projected value while waiting.
s = replace_once(s, '    boxes.clear();\n    const bool hookInstalled', '    boxes.clear();\n    g_projectedCount.store(0, std::memory_order_relaxed);\n    const bool hookInstalled')
s = replace_once(s, '    if (frameLogDue) {\n        __android_log_print(ANDROID_LOG_INFO, kLogTag,\n                            "ESP player frame:', '    if (frameLogDue) {\n        __android_log_print(ANDROID_LOG_INFO, kLogTag,\n                            "ESP pose cache: captures=%llu renderPositionGetters=0 cameraGetters=0",\n                            static_cast<unsigned long long>(g_playerPoseCaptures.load(std::memory_order_relaxed)));\n        __android_log_print(ANDROID_LOG_INFO, kLogTag,\n                            "ESP player frame:')

updates = {'jni/esp_runtime.hpp':s, 'jni/game_offsets.hpp':h}
for name in ['README.md','docs/ADB_DEBUG.md','Standoff2_1.0.0_ARM64_internal_offsets.txt']:
    updates[name] = originals[name].replace(old_profile,new_profile)
updates['docs/ADB_DEBUG.md'] += '''\n\n## 2026-09-30 native-camera-cache correction\n\n- The previous build installed the hook successfully on UnityMain. Crash was in\n  camera matrix capture, not hook installation: 0x536520C -> 0x5A7C2FC -> 0x55493B0.\n- 0x53653C4 returns a GC handle (native Camera+0x18), not a managed Camera*.\n  Its managed wrapper at 0x6D28F70 explicitly converts the handle through 0x67EC810.\n- Active camera source is native Camera::GetMain, RVA 0x54C1338.\n- Matrix getters and direct W2S are disabled on every thread. Matrix cache at\n  native Camera+0x70 (view) and +0xB0 (projection) is copied through the safe reader.\n- Position getter runs only on UnityMain after the original ake::opmn callback,\n  after validating the parts list and dwc part. Render consumes cached positions.\n- Expected logs: `ESP ake update hook ready`, `ESP UnityMain position snapshot`,\n  `ESP UnityMain camera snapshot: source=native-cache`, `ESP player frame ... projected=N`.\n- Rejected camera samples include a reason and pointer. No sample => no drawing.\n- Build success and offline tests do NOT prove runtime stability/visible ESP.\n  A new live ADB match test is required.\n'''
updates['Standoff2_1.0.0_ARM64_internal_offsets.txt'] += '''\n\n[V8_NATIVE_CAMERA_CACHE_CORRECTION_2026_09_30]\nCAMERA_GET_MAIN_0x53653C4_RETURN=GC_HANDLE_NOT_MANAGED_CAMERA\nGC_HANDLE_UNMARSHAL_MANAGED=0x67EC810 (used by get_main at 0x6D28F70)\nNATIVE_CAMERA_GET_MAIN=0x54C1338\nNATIVE_CAMERA_GET_MAIN_RETURN=native_Camera_pointer\nCAMERA_MAIN_GC_HANDLE_PLUS_0x10=REJECTED_INVALID_UNWRAP\nCAMERA_MATRIX_COPY=view_plus_0x70_and_projection_plus_0xB0_kernel_safe_read\nCAMERA_MATRIX_GETTER_CALLS=0_ON_ALL_THREADS\nDIRECT_W2S_CALLS=0_ON_ALL_THREADS\nRENDER_POSITION_GETTER_CALLS=0\nPOSITION_CAPTURE=UnityMain_after_ake_opmn_original_with_validated_dwc_part\nPOSITION_FRESHNESS_MS=500\nCAMERA_SNAPSHOT_FRESHNESS_MS=1000\nPREVIOUS_CRASH_PC=0x55493B0\nPREVIOUS_CRASH_CALLBACK_THREAD=UnityMain\nPREVIOUS_HOOK_STATE=installed_and_called\nRUNTIME_RETEST=REQUIRED_NOT_YET_PASSED\n'''
updates['README.md'] += '''\n\n### Native camera cache fix (2026-09-30)\n\nThis build removes the incorrect GC-handle-as-object camera unwrap. Native\nCamera::GetMain (0x54C1338) is used only on UnityMain; safe matrix copies and\nmain-thread player position snapshots are consumed by the render path.\nDirect camera matrix getters, W2S, and render-thread position getters are off.\nOnly ARM64 is built. Live runtime stability and visible ESP still require testing.\n'''
# Validate all edits before creating a backup or modifying the project.
assert 'g_runtime.worldToCameraMatrix(' not in s
assert 'g_runtime.projectionMatrix(' not in s
assert 'g_runtime.worldToScreen(' not in s
produce = s[s.index('inline bool produceBoxes('):s.index('}  // namespace detail')]
assert 'g_runtime.playerPosition(' not in produce
assert 'g_runtime.cameraMain(' not in produce
assert 'g_runtime.componentTransform(' not in produce
assert s.count('g_runtime.playerPosition(player, nullptr)') == 1
assert 'kNativeCameraGetMain = 0x54C1338' in h
backup = root/'backups'/(datetime.now().strftime('%Y%m%d-%H%M%S')+'-before-native-camera-cache')
backup.mkdir(parents=True,exist_ok=False)
for name in names:
    dst=backup/name; dst.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(root/name,dst)
for name in ['out/arm64-v8a/liblemmingrmt.so','obj/local/arm64-v8a/liblemmingrmt.so']:
    src=root/name
    if src.exists():
        dst=backup/name; dst.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(src,dst)
for name,text in updates.items():
    (root/name).write_text(text)
print('PATCH_APPLIED',new_profile)
print('BACKUP',backup)
print('CAMERA_GETTER_CALLS=0 DIRECT_W2S_CALLS=0 RENDER_POSITION_GETTER_CALLS=0')
print('SOURCE_SHA256',hashlib.sha256(s.encode()).hexdigest())
