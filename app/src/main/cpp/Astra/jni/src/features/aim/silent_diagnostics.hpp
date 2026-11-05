#pragma once
#include "features/aim/core.hpp"
#include "features/aim/debug.hpp"
#include "features/aim/input_init.hpp"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>

namespace silent_diag {

inline char status[384] = "Not saved";

inline long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void save() {
    auto c = astra::aim::configuration();
    char report[8192];
    std::snprintf(report, sizeof(report),
        "AstraSilent / 1.1.0 ARM64 / shot+command / sync-v2\n"
        "enabled=%d fov=%.2f maxDistance=%.1f bodies=%02x multipoints=%d pointScale=%.2f\n"
        "visibleCheck=%d prediction=%d predictionMs=%.0f smoothing=%.3f lockTarget=%d pauseInMenu=%d\n"
        "hook[ready=%d attempts=%llu installReason=%s] state=%d reason=%s(%d) applications=%llu\n"
        "calls=%llu commands=%llu commandRejected=%llu lastCmd[pitch=%.2f yaw=%.2f target=%p]\n"
        "point[bone=%d leadCm=%.2f live=%llu centerFallback=%llu rejected=%llu bodyFallback=%llu recoilComp=%llu recoil=%.2f/%.2f noPhase=%llu shotSpoof=%llu] attackRaw=%d silentAttempts=%llu firing=%llu\n"
        "weapon[noRecoilPatch=%d mult=%.3f point=%.3f/%.3f writes=%llu nonZero=%llu]\n"
        "input[ready=%d attempts=%llu reason=%s] commandsCompleted=%llu commandRejected=%llu gate[calls=%llu ours=%llu begin=%llu end=%llu] skmi=%d beginReason=%d look[hook=%d subs=%llu] beginFail[owv=%llu world=%llu token=%llu ro=%llu look=%llu]\n"
        "actors[fresh=%d volumes=%d] selection[calls=%llu actors=%llu fresh=%llu live=%llu volumes=%llu accepted=%llu fovReject=%llu rangeReject=%llu]\n"
        "visibility[ready=%d rays=%llu clear=%llu target=%llu blocked=%llu unknown=%llu]\n"
        "capture[calls=%llu hitboxes=%llu storedActors=%llu storedVolumes=%llu]\n"
        "sync[matched=%llu missingOrStale=%llu staleLook=%llu originMismatch=%llu targetMoved=%llu lookReadbackFailed=%llu]\n",
        c.silent.enabled, c.silent.fov, c.silent.maxDistance, c.silent.bodies,
        c.silent.multipoints, c.silent.pointScale,
        c.silent.visibleCheck, c.silent.prediction, c.silent.predictionMs,
        c.silent.smoothing, c.silent.lockTarget, c.pauseInMenu,
        astra::aim::g_silentHookReady.load(),
        (unsigned long long)astra::aim::debug::silentAttempts.load(),
        astra::aim::debug::installReason(astra::aim::debug::silentInstallReason.load()),
        astra::aim::g_silentState.load(),
        astra::aim::debug::reason(astra::aim::debug::silentReason.load()),
        astra::aim::debug::silentReason.load(),
        (unsigned long long)astra::aim::g_silentApplications.load(),
        (unsigned long long)astra::aim::debug::silentCalls.load(),
        (unsigned long long)astra::aim::debug::silentCommands.load(),
        (unsigned long long)astra::aim::debug::silentCommandRejected.load(),
        astra::aim::debug::silentCommandPitch.load(),
        astra::aim::debug::silentCommandYaw.load(),
        reinterpret_cast<void*>(astra::aim::debug::silentCommandTarget.load()),
        astra::aim::debug::silentBone.load(),
        astra::aim::debug::silentLeadCm.load(),
        (unsigned long long)astra::aim::debug::silentLiveRefresh.load(),
        (unsigned long long)astra::aim::debug::silentCenterFallback.load(),
        (unsigned long long)astra::aim::debug::silentPointRejected.load(),
        (unsigned long long)astra::aim::debug::silentBodyFallback.load(),
        (unsigned long long)astra::aim::debug::silentRecoilComp.load(),
        astra::aim::debug::recoilPitch.load(),
        astra::aim::debug::recoilYaw.load(),
        (unsigned long long)astra::aim::debug::silentNoPhase.load(),
        (unsigned long long)astra::aim::debug::silentShotSpoof.load(),
        astra::aim::debug::silentAttackRaw.load(),
        (unsigned long long)astra::aim::debug::silentCommandAttempts.load(),
        (unsigned long long)astra::aim::debug::silentFiringSeen.load(),
        astra::aim::debug::noRecoilPatch.load(),
        astra::aim::debug::weaponMult.load(),
        astra::aim::debug::weaponPointX.load(),
        astra::aim::debug::weaponPointY.load(),
        (unsigned long long)astra::aim::debug::weaponNoRecoilWrites.load(),
        (unsigned long long)astra::aim::debug::recoilNonZero.load(),
        astra::aim::g_inputHookReady.load(),
        (unsigned long long)astra::aim::debug::inputAttempts.load(),
        astra::aim::input_init::reasonText(astra::aim::g_inputInstallReason.load()),
        (unsigned long long)astra::aim::debug::commandCompleted.load(),
        (unsigned long long)astra::aim::debug::commandRejected.load(),
        (unsigned long long)astra::aim::debug::commandGateCalls.load(),
        (unsigned long long)astra::aim::debug::commandOurs.load(),
        (unsigned long long)astra::aim::debug::commandBegin.load(),
        (unsigned long long)astra::aim::debug::commandEndCalls.load(),
        astra::aim::debug::commandSkmi.load(),
        astra::aim::debug::commandBeginReason.load(),
        astra::aim::debug::commandLookHookReady.load(),
        (unsigned long long)astra::aim::debug::commandLookSubstitutions.load(),
        (unsigned long long)astra::aim::debug::beginFailOwv.load(),
        (unsigned long long)astra::aim::debug::beginFailWorld.load(),
        (unsigned long long)astra::aim::debug::beginFailToken.load(),
        (unsigned long long)astra::aim::debug::beginFailWritable.load(),
        (unsigned long long)astra::aim::debug::beginFailLook.load(),
        astra::aim::debug::freshActors.load(),
        astra::aim::debug::freshVolumes.load(),
        (unsigned long long)astra::aim::debug::selectCalls.load(),
        (unsigned long long)astra::aim::debug::selectActors.load(),
        (unsigned long long)astra::aim::debug::selectFresh.load(),
        (unsigned long long)astra::aim::debug::selectLive.load(),
        (unsigned long long)astra::aim::debug::selectVolumes.load(),
        (unsigned long long)astra::aim::debug::accepted.load(),
        (unsigned long long)astra::aim::debug::fovReject.load(),
        (unsigned long long)astra::aim::debug::rangeReject.load(),
        astra::aim::g_visibilityReady.load(),
        (unsigned long long)astra::aim::debug::visibilityRays.load(),
        (unsigned long long)astra::aim::debug::visibilityClear.load(),
        (unsigned long long)astra::aim::debug::visibilityTarget.load(),
        (unsigned long long)astra::aim::debug::visibilityBlocked.load(),
        (unsigned long long)astra::aim::debug::visibilityUnknown.load(),
        (unsigned long long)astra::aim::debug::captureCalls.load(),
        (unsigned long long)astra::aim::debug::hitboxes.load(),
        (unsigned long long)astra::aim::debug::storedActors.load(),
        (unsigned long long)astra::aim::debug::storedVolumes.load(),
        (unsigned long long)astra::aim::debug::silentPlanMatched.load(),
        (unsigned long long)astra::aim::debug::silentPlanRejected.load(),
        (unsigned long long)astra::aim::debug::silentStaleLookRejected.load(),
        (unsigned long long)astra::aim::debug::silentOriginMismatch.load(),
        (unsigned long long)astra::aim::debug::silentPlanTargetRejected.load(),
        (unsigned long long)astra::aim::debug::silentLookReadbackFailed.load());
    __android_log_print(ANDROID_LOG_INFO, "AstraSilent", "%s", report);

    char path[260];
    const char* dirs[] = {"/storage/emulated/0/Download", "/storage/emulated/0",
                          "/sdcard/Download", "/data/local/tmp"};
    FILE* file = nullptr;
    for (const char* dir : dirs) {
        std::snprintf(path, sizeof(path), "%s/astra_silent_%lld.txt", dir, nowMs());
        file = std::fopen(path, "w");
        if (file) break;
    }
    if (!file) {
        std::snprintf(status, sizeof(status), "Save failed: %s", std::strerror(errno));
        return;
    }
    const std::size_t length = std::strlen(report);
    const bool ok = std::fwrite(report, 1, length, file) == length;
    const int closed = std::fclose(file);
    std::snprintf(status, sizeof(status), "%s: %s",
                  ok && closed == 0 ? "Saved" : "Write failed", path);
}

} // namespace silent_diag
