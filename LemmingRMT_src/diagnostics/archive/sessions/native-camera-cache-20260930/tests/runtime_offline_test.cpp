#include "esp_runtime.hpp"
#include <cassert>
#include <limits>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdarg>

extern "C" int __android_log_print(int, const char*, const char*, ...) { return 0; }
using namespace lemming::esp;
using namespace lemming::esp::detail;
namespace off = lemming::offsets;
static std::uintptr_t testCamera = 0;
static int getterCalls = 0;
static int positionCalls = 0;
static std::uintptr_t nativeMain() { return testCamera; }
static void forbiddenMatrixGetter(std::uintptr_t, Matrix4x4*) { ++getterCalls; }
static Vec3 positionGetter(void*, const void*) { ++positionCalls; return {0,0,-10}; }

template<class T> void put(std::vector<unsigned char>& storage, std::size_t offset, T value) {
    assert(offset+sizeof(T) <= storage.size());
    std::memcpy(storage.data()+offset, &value, sizeof(T));
}
static void sampleCamera() {
    g_lastCameraCaptureAttempt = {};
    captureCameraSnapshotFromPlayerCallback();
}
int main() {
    prctl(PR_SET_NAME, "UnityMain", 0, 0, 0);
    int token = 5;
    int readToken = 0;
    assert(readValue(&token, readToken) && readToken == token);
    assert(!readValue(reinterpret_cast<void*>(0x12345678), readToken));
    std::vector<unsigned char> camera(0x600, 0), player(0x100, 0), part(0x40, 0),
        container(0x30, 0), list(0x30, 0), array(0x40, 0);
    std::array<std::uintptr_t, 64> vt{};
    g_runtime.unityBase = reinterpret_cast<std::uintptr_t>(vt.data()) - off::elf::kDataRelRoBegin;
    vt[0xD0/sizeof(std::uintptr_t)] = g_runtime.unityBase + off::elf::kTextBegin + 0x20;
    testCamera = reinterpret_cast<std::uintptr_t>(camera.data());
    put(camera, 0, reinterpret_cast<std::uintptr_t>(vt.data()));
    put(camera, 0x20, reinterpret_cast<std::uintptr_t>(part.data()));
    CameraMatrixCache cache{};
    for(int i=0; i<4; ++i) cache.worldToCamera.value[i*5] = 1;
    cache.projection.value[0] = 1;
    cache.projection.value[5] = 1;
    cache.projection.value[10] = -1.002f;
    cache.projection.value[11] = -1;
    cache.projection.value[14] = -0.2002f;
    put(camera, off::managed::kNativeCameraWorldToCameraMatrix, cache);
    g_runtime.cameraMain = nativeMain;
    g_runtime.worldToCameraMatrix = forbiddenMatrixGetter;
    g_runtime.projectionMatrix = forbiddenMatrixGetter;
    sampleCamera();
    assert(g_cameraSnapshotReady && g_cameraSnapshotCaptures == 1);
    assert(g_cameraSnapshotNative == testCamera && g_cameraSnapshotObject == 0);
    assert(getterCalls == 0);
    Vec3 screen{};
    assert(project(cache, {0,0,-10}, 1920,1080,screen));
    assert(std::fabs(screen.x-960) < 0.01f && std::fabs(screen.y-540) < 0.01f);
    assert(project(cache, {0,1.8f,-10}, 1920,1080,screen));
    assert(screen.y < 540);
    assert(!project(cache, {0,0,10}, 1920,1080,screen));
    testCamera = 0x12345678;
    sampleCamera();
    assert(g_cameraSnapshotFailures == 1 && g_cameraSnapshotCaptures == 1);
    testCamera = reinterpret_cast<std::uintptr_t>(camera.data());
    cache.worldToCamera.value[15] = 0;
    put(camera, off::managed::kNativeCameraWorldToCameraMatrix, cache);
    sampleCamera();
    assert(g_cameraSnapshotFailures == 2);
    cache.worldToCamera.value[15] = 1;
    cache.projection.value[0] = std::numeric_limits<float>::quiet_NaN();
    put(camera, off::managed::kNativeCameraWorldToCameraMatrix, cache);
    sampleCamera();
    assert(g_cameraSnapshotFailures == 3);
    prctl(PR_SET_NAME, "UnityGfxDeviceW", 0, 0, 0);
    sampleCamera();
    assert(g_cameraSnapshotFailures == 3 && g_cameraSnapshotCaptures == 1);
    prctl(PR_SET_NAME, "UnityMain", 0, 0, 0);
    int playerClass, transformClass, world;
    g_runtime.playerClass = &playerClass;
    g_runtime.transformsClass = &transformClass;
    g_runtime.playerPosition = positionGetter;
    put(player,0,g_runtime.playerClass);
    put(player,off::field::mfc::kWorld, static_cast<void*>(&world));
    put(player,off::field::mfc::kPartsContainer, static_cast<void*>(container.data()));
    put(container,off::field::part_container::kPartList,static_cast<void*>(list.data()));
    put(list,off::field::managed_list::kItems,static_cast<void*>(array.data()));
    put(list,off::field::managed_list::kSize,std::int32_t{1});
    put(array,off::managed::kArrayLength,std::uintptr_t{1});
    put(array,off::managed::kArrayData,static_cast<void*>(part.data()));
    put(part,0,g_runtime.transformsClass);
    capturePlayerPoseFromCallback(player.data());
    assert(positionCalls == 1 && g_playerPoseCaptures == 1);
    Vec3 pose{};
    assert(readPlayerPoseSnapshot(player.data(),pose) && pose.z == -10);
    g_playerPoses[0].captured -= std::chrono::seconds(2);
    assert(!readPlayerPoseSnapshot(player.data(),pose));
    capturePlayerPoseFromCallback(player.data());
    put(player,off::field::mfc::kWorld,static_cast<void*>(part.data()));
    assert(!readPlayerPoseSnapshot(player.data(),pose));
    put(list,off::field::managed_list::kSize,std::int32_t{1000});
    capturePlayerPoseFromCallback(player.data());
    assert(positionCalls == 2);
    prctl(PR_SET_NAME,"UnityGfxDeviceW",0,0,0);
    capturePlayerPoseFromCallback(player.data());
    assert(positionCalls == 2 && getterCalls == 0);
    std::array<std::thread, 4> threads;
    for(auto& thread:threads) thread=std::thread([&]{
        for(int n=0;n<1000;++n) { int output=0; assert(readValue(&token,output) && output==token); }
    });
    for(auto& thread:threads) thread.join();
    std::cout << "PASS: safe-reader valid/invalid/concurrent reads; native camera success; invalid-pointer, invalid-matrix, NaN, wrong-thread rejection; camera getters never called; projection front/back/Y; pose capture, stale-world, expiry and list bounds\n";
}
