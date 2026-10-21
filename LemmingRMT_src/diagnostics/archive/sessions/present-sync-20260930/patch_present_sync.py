from pathlib import Path
from datetime import datetime
import hashlib,shutil,json
p=Path('/home/leftcode/Projects/LemmingRMT')
names=['jni/esp_runtime.hpp','jni/game_offsets.hpp','jni/main.cpp','README.md','docs/ADB_DEBUG.md','Standoff2_1.0.0_ARM64_internal_offsets.txt']
texts={name:(p/name).read_text() for name in names}
assert hashlib.sha256(texts['jni/esp_runtime.hpp'].encode()).hexdigest()=='d10f659b118b64b68ef7fddeb3516d5f063f467f812b92f4088492f0d52268f9'
assert hashlib.sha256(texts['jni/main.cpp'].encode()).hexdigest()=='48fd77557fb5a68a9d0814cfdec1543f3bcea3e3107cac638fca5b2bbf423e47'
old_profile='Standoff2-1.0.0-arm64-il2cppdump-v8-native-camera-cache'
new_profile='Standoff2-1.0.0-arm64-il2cppdump-v8-present-sync'
s=texts['jni/esp_runtime.hpp']; m=texts['jni/main.cpp']
def one(t,a,b):
 assert t.count(a)==1,(a[:100],t.count(a))
 return t.replace(a,b,1)
s=one(s,'inline std::uintptr_t g_cameraSnapshotNative = 0;','''inline std::uintptr_t g_cameraSnapshotNative = 0;
inline std::uintptr_t g_cameraSnapshotVtable = 0;
inline std::uintptr_t g_cameraSnapshotGameObject = 0;
inline std::atomic<std::uint64_t> g_presentCameraReads{0};
inline std::atomic<std::uint64_t> g_presentCameraRejects{0};
inline std::atomic<int> g_mainCameraAgeMs{0};''')
s=one(s,'''        g_cameraSnapshotNative = nativeCamera;
        g_cameraSnapshotTime = now;''','''        g_cameraSnapshotNative = nativeCamera;
        g_cameraSnapshotVtable = vtable;
        g_cameraSnapshotGameObject = gameObject;
        g_cameraSnapshotTime = now;''')
helper=r'''// Re-copy matrix VALUES at presentation time; never execute Unity methods here.
// The main callback leases a typed camera identity. Checking both header fields
// before/after the two identical copies rejects freed/reused objects and tears.
inline bool readPresentCameraMatrices(std::uintptr_t nativeCamera,
                                      std::uintptr_t expectedVtable,
                                      std::uintptr_t expectedGameObject,
                                      CameraMatrixCache& output) {
    if (!nativeCamera || !expectedGameObject ||
        expectedVtable < g_runtime.unityBase + offsets::elf::kDataRelRoBegin ||
        expectedVtable >= g_runtime.unityBase + offsets::elf::kDataEnd) return false;
    const auto headerMatches = [&]() {
        std::uintptr_t vtable = 0;
        std::uintptr_t gameObject = 0;
        return readValue(reinterpret_cast<void*>(nativeCamera), vtable) &&
               readValue(reinterpret_cast<void*>(nativeCamera + 0x20), gameObject) &&
               vtable == expectedVtable && gameObject == expectedGameObject;
    };
    const auto address = reinterpret_cast<void*>(nativeCamera +
        offsets::managed::kNativeCameraWorldToCameraMatrix);
    for (int attempt = 0; attempt < 2; ++attempt) {
        if (!headerMatches()) return false;
        CameraMatrixCache candidate{};
        CameraMatrixCache verification{};
        if (!readValue(address, candidate) || !readValue(address, verification)) return false;
        if (!headerMatches()) return false;
        if (std::memcmp(&candidate, &verification, sizeof(candidate)) != 0) continue;
        const auto& view = candidate.worldToCamera.value;
        if (!cameraMatrixFinite(candidate.worldToCamera) ||
            !cameraMatrixFinite(candidate.projection) ||
            std::fabs(view[3]) > 0.001f || std::fabs(view[7]) > 0.001f ||
            std::fabs(view[11]) > 0.001f || std::fabs(view[15] - 1.0f) > 0.01f ||
            std::fabs(candidate.projection.value[0]) < 0.001f ||
            std::fabs(candidate.projection.value[5]) < 0.001f) return false;
        output = candidate;
        return true;
    }
    return false;
}

inline bool readCameraForPresent(CameraMatrixCache& output,
                                 std::uintptr_t& nativeCamera) {
    std::uintptr_t vtable = 0;
    std::uintptr_t gameObject = 0;
    std::chrono::steady_clock::time_point mainCaptureTime{};
    {
        std::lock_guard<std::mutex> lock(g_cameraSnapshotMutex);
        if (!g_cameraSnapshotReady.load(std::memory_order_acquire)) return false;
        nativeCamera = g_cameraSnapshotNative;
        vtable = g_cameraSnapshotVtable;
        gameObject = g_cameraSnapshotGameObject;
        mainCaptureTime = g_cameraSnapshotTime;
    }
    const auto age = std::chrono::steady_clock::now() - mainCaptureTime;
    const auto ageMs = std::chrono::duration_cast<std::chrono::milliseconds>(age).count();
    g_mainCameraAgeMs.store(static_cast<int>(std::min<std::int64_t>(ageMs, 10000)),
                           std::memory_order_relaxed);
    if (age >= std::chrono::milliseconds(250) ||
        !readPresentCameraMatrices(nativeCamera, vtable, gameObject, output)) {
        g_presentCameraRejects.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    g_presentCameraReads.fetch_add(1, std::memory_order_relaxed);
    return true;
}

'''
s=one(s,'struct PlayerPoseCache {',helper+'struct PlayerPoseCache {')
start=s.index('    // eglSwapBuffers runs on UnityGfxDeviceW. It must not call Camera.main,')
end=s.index('    // Component_GetTransform(Camera) remains disabled:',start)
s=s[:start]+'''    // Screen projection is re-evaluated for EVERY presentation. UnityMain only
    // publishes camera identity; safely re-copy matrix values here so the early
    // player's update snapshot cannot add an extra camera-frame delay.
    CameraMatrixCache cameraMatrices{};
    std::uintptr_t cameraObject = 0;
    std::uintptr_t nativeCamera = 0;
    if (!readCameraForPresent(cameraMatrices, nativeCamera)) {
        setStatus(Status::WaitingForMatch, "waiting for fresh presentation camera");
        if (frameLogDue) {
            __android_log_print(ANDROID_LOG_WARN, kLogTag,
                "ESP present camera unavailable: reads=%llu rejects=%llu mainAgeMs=%d getters=0 oldBoxes=0",
                static_cast<unsigned long long>(g_presentCameraReads.load(std::memory_order_relaxed)),
                static_cast<unsigned long long>(g_presentCameraRejects.load(std::memory_order_relaxed)),
                g_mainCameraAgeMs.load(std::memory_order_relaxed));
            g_lastFrameLog = now;
        }
        return true;
    }

'''+s[end:]
s=one(s,'    if (now - detail::g_lastFrame < milliseconds(33)) return true;\n    detail::g_lastFrame = now;','''    // No 33ms screen-coordinate cache. A static world point must be projected
    // again even when actor/HP/weapon data did not change since the last tick.
    detail::g_lastFrame = now;''')
s=one(s,'now - pose.captured < std::chrono::milliseconds(500)', 'now - pose.captured < std::chrono::milliseconds(250)')
s=one(s,'// eglSwapBuffers only consumes these snapshots. No matrix/W2S/Transform getters', '// eglSwapBuffers refreshes matrix values by safe reads. No matrix/W2S/Transform getters')
s=one(s,'''    if (frameLogDue) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP pose cache:''','''    if (frameLogDue) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
            "ESP present sync: source=present-native-cache reads=%llu rejects=%llu mainAgeMs=%d projection=each-present screenCacheMs=0 getters=0",
            static_cast<unsigned long long>(g_presentCameraReads.load(std::memory_order_relaxed)),
            static_cast<unsigned long long>(g_presentCameraRejects.load(std::memory_order_relaxed)),
            g_mainCameraAgeMs.load(std::memory_order_relaxed));
        __android_log_print(ANDROID_LOG_INFO, kLogTag,
                            "ESP pose cache:''')
m=one(m,'''        std::vector<EspScreenBox> nextEspBoxes;
        { std::lock_guard<std::mutex> lock(g_espMutex); nextEspBoxes = g_espScreenBoxes; }''','''        // Start empty: no previous-frame pixel coordinates may leak into this frame.
        std::vector<EspScreenBox> nextEspBoxes;''')
m=one(m,'''        if (espUpdated) {
            std::lock_guard<std::mutex> lock(g_espMutex);
            g_espScreenBoxes.swap(nextEspBoxes);
        }''','''        {
            std::lock_guard<std::mutex> lock(g_espMutex);
            g_espScreenBoxes.swap(nextEspBoxes);
        }''')
m=one(m,'    const float height = 390.0f * scale;', '    const float height = 412.0f * scale;')
m=one(m,'''        ImGui::TextDisabled("Drawing needs UnityMain camera and position snapshots.");''','''        ImGui::TextDisabled("World projection: every presented frame, no 30 FPS cache.");
        ImGui::Text("Present camera: %llu | Rejects: %llu | Main age: %d ms",
                    static_cast<unsigned long long>(lemming::esp::detail::g_presentCameraReads.load()),
                    static_cast<unsigned long long>(lemming::esp::detail::g_presentCameraRejects.load()),
                    lemming::esp::detail::g_mainCameraAgeMs.load());''')
updates=dict(texts)
updates['jni/esp_runtime.hpp']=s; updates['jni/main.cpp']=m
updates['jni/game_offsets.hpp']=one(texts['jni/game_offsets.hpp'],old_profile,new_profile)
updates['README.md']+='''\n\n### Presentation sync (2026-09-30)\n\nThe reported working native-camera-cache build had a 33ms screen-box throttle.\nThe new present-sync build recomputes projection before every EGL presentation.\nA leased native camera identity is published by UnityMain; its cached matrix\nvalues are double-copied safely at presentation time without calling Unity.\nNo old screen boxes are drawn on a failed update. The overlay is still Dear\nImGui in the game's own swap hook, NOT Unity scene geometry or an external app.\nExact GPU frame/view-projection matching is not claimed: if render queues still\ncause residual drift, capture the rendering pass's own camera matrices next.\n'''
updates['docs/ADB_DEBUG.md']+='''\n\n## Presentation-sync drift correction\n\nLive native-camera-cache build confirmed on 2026-09-30: hook callbacks, camera\ncaptures and enemy projection active; user confirmed visible ESP. Source had\n`update()` reuse old screen boxes for 33ms, reducing geometry refresh to 30 FPS.\n\nNew profile: Standoff2-1.0.0-arm64-il2cppdump-v8-present-sync.\nExpected `ESP present sync` log: `projection=each-present screenCacheMs=0 getters=0`.\nPresent camera counters should increase with active overlay render frames.\nMain age measures pointer-identity lease age, NOT rendered matrix age.\nMatrices are double-copied from native+0x70/+0xB0 immediately before projection.\nIdentity is checked before/after; unusable or changed caches reject the frame.\nMain identity and player-position leases expire at 250ms.\nDo not call Camera.main/Transform/matrix getters from UnityGfxDeviceW.\nTest sharp left/right camera rotations while looking at stationary targets;\ncompare motion lag and watch for rejects or frame-rate regression.\nUnity CPU cached matrices are not proven to equal a queued GPU frame's camera.\nResidual drift requires render-pass matrix capture, not smoothing or prediction.\n'''
updates['Standoff2_1.0.0_ARM64_internal_offsets.txt']+='''\n\n[V8_PRESENT_SYNC_2026_09_30]\nPROFILE=Standoff2-1.0.0-arm64-il2cppdump-v8-present-sync\nPROJECTION_RATE=EVERY_EGL_PRESENT\nOLD_SCREEN_BOX_CACHE_33MS=REMOVED\nCAMERA_SOURCE=SAFE_NATIVE_MATRIX_COPY_AT_PRESENT\nCAMERA_COPY=2_IDENTICAL_0x80_COPIES_WITH_HEADER_CHECK_BEFORE_AND_AFTER\nCAMERA_IDENTITY_SOURCE=UnityMain_native_Camera_GetMain_0x54C1338\nMAIN_IDENTITY_LEASE_MS=250\nPLAYER_POSITION_LEASE_MS=250\nRENDER_UNITY_API_CALLS=0\nDIRECT_W2S_CALLS=0\nINVALID_PRESENT_CAMERA=EMPTY_BOXES_NO_STALE_FALLBACK\nACTUAL_RENDER_PASS_VP_MATCH=NOT_YET_MEASURED\n'''
assert 'milliseconds(33)' not in s
assert 'nextEspBoxes = g_espScreenBoxes' not in m
for call in ['g_runtime.worldToCameraMatrix(','g_runtime.projectionMatrix(','g_runtime.worldToScreen(']: assert call not in s
helpertext=s[s.index('inline bool readPresentCameraMatrices('):s.index('struct PlayerPoseCache {')]
for call in ['g_runtime.cameraMain(','g_runtime.playerPosition(','g_runtime.componentTransform(']:assert call not in helpertext
backup=p/'backups'/(datetime.now().strftime('%Y%m%d-%H%M%S')+'-before-present-sync');backup.mkdir(parents=True)
for name in names+['out/arm64-v8a/liblemmingrmt.so','obj/local/arm64-v8a/liblemmingrmt.so']:
 dst=backup/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p/name,dst)
for name,text in updates.items():(p/name).write_text(text)
d=p/'diagnostics/present-sync-20260930';d.mkdir(parents=True,exist_ok=True)
(d/'backup-path.txt').write_text(str(backup)+'\n')
(d/'analysis.txt').write_text('''User confirmed visible ESP on previous native-camera-cache build.\nADB showed active hook/pose/camera captures and enemy projection.\nSource root cause for obvious screen drift: 33ms update throttle with reused pixel boxes.\nChanges: each-present projection, presentation-time safe matrix refresh, no old boxes\non failures, native identity/generation-header guard, two identical matrix copies.\nNo unsafe native camera or transform methods on render.\nThis is a synchronization improvement, not conversion into Unity scene objects.\nFull GPU-pass matrix agreement still needs live visual validation.\n''')
print('APPLIED',new_profile);print('BACKUP',backup)
