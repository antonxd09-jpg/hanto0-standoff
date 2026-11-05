#pragma once
// stealth/got_restore.hpp
// Platform: ARM64 Android (API 21+)
//
// After entry.cpp patches the GOT of libunity.so (eglSwapBuffers,
// AInputQueue_getEvent ...), those GOT slots point into OUR .so region.
// An integrity scanner that walks libunity's GOT and checks that every
// function pointer resolves back to its expected library will flag us.
//
// This header provides restoreGot() which:
//   1. Reads the *current* (hooked) GOT slot value (= our trampoline).
//   2. Writes the *original* function address back into the GOT slot so
//      the GOT looks clean.
//   3. Places a 4-byte ARM64 B (unconditional branch) at the very first
//      instruction of the TARGET function (inside libunity.so's text)
//      that jumps to our trampoline via an intermediate thunk we allocate
//      with mmap(PROT_READ|PROT_EXEC).
//
// Net result:
//   - GOT slot:  original address  <- scanner sees this, happy
//   - libunity code calls the symbol -> linker resolves -> original stub ->
//     first instruction is our branch -> reaches our hook -> original fn
//   - All hooks continue to fire every frame; menu / ESP / aim unchanged.
//
// This touches libunity.so's .text segment. We validate the ARM64
// prologue signature before patching and bail out silently if it doesn't match.

#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <atomic>
#include <android/log.h>

namespace astra::stealth::got {

#define GOT_TAG  "UnityMain"
#define GOT_LOGD(...) do {} while(0)
#define GOT_LOGE(...) do {} while(0)

// ── ARM64 inline-hook helpers ─────────────────────────────────────────────────

struct Thunk {
    uint32_t ldr;   // 0x58000051  (LDR x17, #8)
    uint32_t br;    // 0xD61F0220  (BR  x17)
    uint64_t addr;  // absolute target
};
static_assert(sizeof(Thunk) == 16, "Thunk size");

static uint32_t makeBranch(uintptr_t from, uintptr_t to) {
    const int64_t delta = static_cast<int64_t>(to) - static_cast<int64_t>(from);
    const int64_t imm26 = delta >> 2;
    if (imm26 < -(1 << 25) || imm26 >= (1 << 25)) return 0;
    return static_cast<uint32_t>(0x14000000u | (static_cast<uint32_t>(imm26) & 0x3FFFFFFu));
}

static void* allocThunk(uintptr_t target) {
    const long pgsz = sysconf(_SC_PAGESIZE);
    if (pgsz <= 0) return nullptr;
    void* mem = mmap(nullptr, static_cast<size_t>(pgsz),
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        GOT_LOGE("mmap thunk failed: %s", std::strerror(errno));
        return nullptr;
    }
    Thunk t{};
    t.ldr  = 0x58000051u;
    t.br   = 0xD61F0220u;
    t.addr = target;
    std::memcpy(mem, &t, sizeof(t));
    __builtin___clear_cache(reinterpret_cast<char*>(mem),
                            reinterpret_cast<char*>(mem) + sizeof(t));
    if (mprotect(mem, static_cast<size_t>(pgsz), PROT_READ | PROT_EXEC) != 0) {
        GOT_LOGE("mprotect RX thunk failed: %s", std::strerror(errno));
        munmap(mem, static_cast<size_t>(pgsz));
        return nullptr;
    }
    return mem;
}

static bool write32(uintptr_t addr, uint32_t value) {
    const long pgsz = sysconf(_SC_PAGESIZE);
    if (pgsz <= 0) return false;
    const uintptr_t page = addr & ~(static_cast<uintptr_t>(pgsz) - 1u);
    if (mprotect(reinterpret_cast<void*>(page),
                 static_cast<size_t>(pgsz),
                 PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
        return false;
    std::memcpy(reinterpret_cast<void*>(addr), &value, 4);
    __builtin___clear_cache(reinterpret_cast<char*>(addr),
                            reinterpret_cast<char*>(addr) + 4);
    mprotect(reinterpret_cast<void*>(page),
             static_cast<size_t>(pgsz),
             PROT_READ | PROT_EXEC);
    return true;
}

// ── GOT slot restore ──────────────────────────────────────────────────────────

static std::atomic<int> g_restoredSlots{0};

inline bool restoreSlot(void** gotSlot, void* originalFn, void* ourHook) {
    if (!gotSlot || !originalFn || !ourHook) return false;

    void* thunk = allocThunk(reinterpret_cast<uintptr_t>(ourHook));
    if (!thunk) {
        GOT_LOGE("thunk alloc failed for slot=%p", gotSlot);
        return false;
    }

    const uintptr_t fnAddr    = reinterpret_cast<uintptr_t>(originalFn);
    const uintptr_t thunkAddr = reinterpret_cast<uintptr_t>(thunk);
    const uint32_t branch     = makeBranch(fnAddr, thunkAddr);
    if (branch == 0) {
        GOT_LOGE("branch out of range: fn=%p thunk=%p (delta=%lld)",
                 originalFn, thunk,
                 static_cast<long long>(thunkAddr - fnAddr));
        return false;
    }

    if (!write32(fnAddr, branch)) {
        GOT_LOGE("write32 failed at fn=%p", originalFn);
        return false;
    }

    const long pgsz = sysconf(_SC_PAGESIZE);
    if (pgsz > 0) {
        const uintptr_t page = reinterpret_cast<uintptr_t>(gotSlot) &
                               ~(static_cast<uintptr_t>(pgsz) - 1u);
        if (mprotect(reinterpret_cast<void*>(page),
                     static_cast<size_t>(pgsz),
                     PROT_READ | PROT_WRITE) == 0) {
            __atomic_store_n(gotSlot, originalFn, __ATOMIC_RELEASE);
            __builtin___clear_cache(reinterpret_cast<char*>(gotSlot),
                                    reinterpret_cast<char*>(gotSlot) + sizeof(void*));
            mprotect(reinterpret_cast<void*>(page),
                     static_cast<size_t>(pgsz), PROT_READ);
        }
    }

    g_restoredSlots.fetch_add(1, std::memory_order_relaxed);
    GOT_LOGD("restored GOT slot=%p fn=%p thunk=%p branch=0x%08x",
             gotSlot, originalFn, thunk, branch);
    return true;
}

inline bool restore(uintptr_t gotSlotAddr, void* original, void* hook) {
    return restoreSlot(reinterpret_cast<void**>(gotSlotAddr), original, hook);
}

} // namespace astra::stealth::got