#pragma once
// stealth/thread_mask.hpp
// Platform: ARM64 Android (API 21+)
//
// Renames the calling thread so that /proc/self/task/<tid>/comm shows a name
// that is indistinguishable from a legitimate Unity worker thread.
// Must be called at the top of hookWorker() before any blocking wait.
//
// Also provides a tiny helper to rename any subsequent std::thread before
// detaching it, so all our threads look like Unity-owned workers.

#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <android/log.h>

namespace astra::stealth::thread {

#define THR_TAG "UnityMain"
#define THR_LOGD(...) do {} while(0)

// Pool of names that appear in a normal Standoff 2 / Unity process.
// The kernel truncates /proc/self/task/<tid>/comm to 15 characters + NUL.
// pthread_setname_np enforces the same limit.
static const char* const kWorkerNames[] = {
    "UnityGfxDevice",
    "UnityMain",
    "Worker Thread",
    "Loading.AsyncR",
    "Bkg Job Exec",
    "Audio Thread",
};
static constexpr size_t kWorkerCount =
    sizeof(kWorkerNames) / sizeof(kWorkerNames[0]);

static const char* pickName() {
    const pid_t tid = gettid();
    return kWorkerNames[static_cast<size_t>(tid) % kWorkerCount];
}

inline void maskCurrent() {
    const char* name = pickName();
    const int rc = pthread_setname_np(pthread_self(), name);
    if (rc == 0) {
        THR_LOGD("thread %d renamed to '%s'", static_cast<int>(gettid()), name);
    } else {
        THR_LOGD("pthread_setname_np failed: rc=%d", rc);
    }
}

inline void maskThread(pthread_t handle, const char* preferredName = nullptr) {
    const char* name = preferredName ? preferredName : pickName();
    pthread_setname_np(handle, name);
}

struct Guard {
    explicit Guard(const char* name = nullptr) {
        if (name) pthread_setname_np(pthread_self(), name);
        else maskCurrent();
    }
};

} // namespace astra::stealth::thread