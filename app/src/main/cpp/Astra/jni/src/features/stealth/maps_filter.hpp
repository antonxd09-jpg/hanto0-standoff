#pragma once
// stealth/maps_filter.hpp
// Platform: ARM64 Android (API 21+)
// Hooks read(2) via GOT patch on libunity.so so that any caller reading
// /proc/self/maps never sees lines that contain our library path.
// All other read() calls are forwarded unchanged.
// No functional code is modified; ESP / aim / menu continue to work normally.

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <android/log.h>
#include <atomic>
#include <string>

namespace astra::stealth::maps {

#define MAPS_TAG "UnityMain"
#define MAPS_LOGD(...) do {} while(0)
#define MAPS_LOGE(...) do {} while(0)

// ── state ────────────────────────────────────────────────────────────────────

using ReadFn = ssize_t(*)(int, void*, size_t);

static ReadFn   g_realRead        = nullptr;
static std::atomic<bool> g_active { false };

// Per-fd scratchpad: we track only the one fd that is currently open on
// /proc/self/maps so we don't touch any other fd.
static std::atomic<int> g_mapsFd { -1 };

// Tokens we strip from /proc/self/maps output.
// Add as many as needed; matching is simple substring search per line.
static const char* const kStripTokens[] = {
    "libastra",    // cheat .so name (libastra.so, libastra_dbg.so …)
    "Astra",       // capital variant / path component
    "vpnService",  // inject helper binary visible in maps on some loaders
};
static constexpr size_t kStripCount = sizeof(kStripTokens) / sizeof(kStripTokens[0]);

// ── helpers ──────────────────────────────────────────────────────────────────

// Returns true if the line (NUL-terminated or length-bounded) should be hidden.
static bool shouldStrip(const char* line, size_t len) {
    for (size_t i = 0; i < kStripCount; ++i) {
        const char* tok = kStripTokens[i];
        const size_t tokLen = std::strlen(tok);
        if (tokLen > len) continue;
        // Case-insensitive substring match would be safer but strstr is fine here
        // because our name is lowercase ASCII.
        for (size_t j = 0; j + tokLen <= len; ++j) {
            if (std::memcmp(line + j, tok, tokLen) == 0) return true;
        }
    }
    return false;
}

// Given a raw buffer read from /proc/self/maps, rewrite it in-place
// removing any lines that match kStripTokens.
// Returns the new (shorter or equal) number of valid bytes.
// Returns true if the line is an anonymous RWX/RX region (typical thunk/hook artifact).
static bool isAnonRxRegion(const char* line, size_t len) {
    // Format: "addr-addr r-xp 00000000 00:00 0 " (no path after the inode col)
    // We detect: perms contain "x", offset is 00000000, device 00:00, inode 0, no path
    char perms[5]{};
    unsigned long long off = 0;
    unsigned int devMaj = 0, devMin = 0;
    unsigned long long inode = 0;
    if (std::sscanf(line, "%*llx-%*llx %4s %llx %x:%x %llu",
                    perms, &off, &devMaj, &devMin, &inode) < 5) return false;
    if (perms[2] != 'x') return false;           // must be executable
    if (devMaj != 0 || devMin != 0) return false; // must be anonymous (no device)
    if (inode != 0) return false;                 // must have no inode
    // Check no path follows (line ends with spaces/newline after inode)
    const char* p = line;
    // skip past inode field
    int fields = 0;
    while (*p && fields < 5) { while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++; if (*p) { p++; fields++; } }
    while (*p == ' ' || *p == '\t') p++;
    return (*p == '\n' || *p == '\0'); // no path -> anonymous
}

static ssize_t filterMapsBuffer(char* buf, ssize_t total) {
    if (total <= 0) return total;

    char* read_ptr  = buf;
    char* write_ptr = buf;
    const char* end = buf + total;

    while (read_ptr < end) {
        // Find end of current line.
        const char* newline = reinterpret_cast<const char*>(
            std::memchr(read_ptr, '\n', static_cast<size_t>(end - read_ptr)));
        const char* line_end = newline ? newline + 1 : end;
        size_t line_len = static_cast<size_t>(line_end - read_ptr);

        if (!shouldStrip(read_ptr, line_len) && !isAnonRxRegion(read_ptr, line_len)) {
            if (write_ptr != read_ptr)
                std::memmove(write_ptr, read_ptr, line_len);
            write_ptr += line_len;
        }
        read_ptr = const_cast<char*>(line_end);
    }
    return static_cast<ssize_t>(write_ptr - buf);
}

// ── hooked read ──────────────────────────────────────────────────────────────

static ssize_t hookedRead(int fd, void* buf, size_t count) {
    if (!g_realRead) return ::read(fd, buf, count);

    const ssize_t result = g_realRead(fd, buf, count);

    // Only intercept the fd we know points to /proc/self/maps.
    if (result <= 0 || fd != g_mapsFd.load(std::memory_order_relaxed))
        return result;

    return filterMapsBuffer(reinterpret_cast<char*>(buf), result);
}

// ── open / close tracking via __libc_init / linker hooks ────────────────────
// We cannot hook open() portably without a second PLT patch, so instead we
// provide a thin wrapper that the entry-point calls around fopen/fgets loops
// that read /proc/self/maps.  Any caller that uses open()+read() directly
// is covered by tracking which fd is the maps fd at the moment read() fires.
//
// Strategy: intercept open() in the same PLT pass as read() so that when
// open("/proc/self/maps",...) is called we record the returned fd, and on
// close() we clear it.

using OpenFn  = int(*)(const char*, int, ...);
using CloseFn = int(*)(int);

static OpenFn  g_realOpen  = nullptr;
static CloseFn g_realClose = nullptr;
static std::atomic<int> g_openSlots  { 0 };
static std::atomic<int> g_readSlots  { 0 };

static int hookedOpen(const char* path, int flags, ...) {
    // Forward first.
    va_list ap;
    va_start(ap, flags);
    const mode_t mode = va_arg(ap, mode_t);
    va_end(ap);

    const int fd = g_realOpen ? g_realOpen(path, flags, mode) : ::open(path, flags, mode);
    if (fd >= 0 && path && std::strstr(path, "/proc/self/maps")) {
        g_mapsFd.store(fd, std::memory_order_relaxed);
    }
    return fd;
}

static int hookedClose(int fd) {
    if (fd == g_mapsFd.load(std::memory_order_relaxed))
        g_mapsFd.store(-1, std::memory_order_relaxed);
    return g_realClose ? g_realClose(fd) : ::close(fd);
}

// ── GOT patch helpers (reuse the same pattern as entry.cpp) ─────────────────

static bool patchGotSlot(void** slot, void* replacement, void** original) {
    if (!slot || !replacement) return false;
    void* current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (current == replacement) return false;
    if (original && !*original && current) *original = current;

    const long pgsz = sysconf(_SC_PAGESIZE);
    if (pgsz <= 0) return false;
    const uintptr_t page = reinterpret_cast<uintptr_t>(slot) &
                           ~(static_cast<uintptr_t>(pgsz) - 1u);
    if (mprotect(reinterpret_cast<void*>(page),
                 static_cast<size_t>(pgsz),
                 PROT_READ | PROT_WRITE) != 0)
        return false;
    __atomic_store_n(slot, replacement, __ATOMIC_RELEASE);
    __builtin___clear_cache(reinterpret_cast<char*>(slot),
                            reinterpret_cast<char*>(slot) + sizeof(void*));
    mprotect(reinterpret_cast<void*>(page),
             static_cast<size_t>(pgsz),
             PROT_READ);
    return true;
}

struct MapsHookTarget {
    const char* symbol;
    void*       replacement;
    void**      original;
    std::atomic<int>* slots;
};

template<typename Rel>
static void patchRel(uintptr_t base,
                     const Rel* relocs, size_t count,
                     const ElfW(Sym)* syms, const char* strtab,
                     const MapsHookTarget* targets, size_t nTargets) {
    for (size_t i = 0; i < count; ++i) {
        const unsigned type = ELF64_R_TYPE(relocs[i].r_info);
        if (type != R_AARCH64_JUMP_SLOT && type != R_AARCH64_GLOB_DAT) continue;
        const unsigned si = ELF64_R_SYM(relocs[i].r_info);
        const char* name = strtab + syms[si].st_name;
        for (size_t t = 0; t < nTargets; ++t) {
            if (std::strcmp(name, targets[t].symbol) != 0) continue;
            auto** slot = reinterpret_cast<void**>(
                base + static_cast<uintptr_t>(relocs[i].r_offset));
            if (patchGotSlot(slot, targets[t].replacement, targets[t].original))
                targets[t].slots->fetch_add(1, std::memory_order_relaxed);
        }
    }
}

static int patchModule(dl_phdr_info* info, size_t, void*) {
    const char* name = info->dlpi_name ? info->dlpi_name : "";
    if (!std::strstr(name, "libunity.so") &&
        !std::strstr(name, "libil2cpp.so") &&
        !std::strstr(name, "libmain.so"))
        return 0;

    const uintptr_t base = static_cast<uintptr_t>(info->dlpi_addr);
    const ElfW(Dyn)* dyn = nullptr;
    uintptr_t minA = UINTPTR_MAX, maxA = 0;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type == PT_LOAD) {
            minA = std::min(minA, base + (uintptr_t)ph.p_vaddr);
            maxA = std::max(maxA, base + (uintptr_t)(ph.p_vaddr + ph.p_memsz));
        } else if (ph.p_type == PT_DYNAMIC) {
            dyn = reinterpret_cast<const ElfW(Dyn)*>(base + ph.p_vaddr);
        }
    }
    if (!dyn || minA == UINTPTR_MAX) return 0;

    const ElfW(Sym)* syms   = nullptr;
    const char*      strtab = nullptr;
    const ElfW(Rela)* pltRela = nullptr; size_t pltRelaSz = 0;
    const ElfW(Rela)* rela   = nullptr; size_t relaSz    = 0;
    uintptr_t pltRelType = 0;

    for (const ElfW(Dyn)* d = dyn; d->d_tag != DT_NULL; ++d) {
        uintptr_t v = (uintptr_t)d->d_un.d_ptr;
        auto abs = [&](uintptr_t x) -> uintptr_t {
            return (x >= minA && x < maxA) ? x : base + x;
        };
        switch (d->d_tag) {
            case DT_SYMTAB:  syms   = reinterpret_cast<const ElfW(Sym)*>(abs(v)); break;
            case DT_STRTAB:  strtab = reinterpret_cast<const char*>(abs(v));      break;
            case DT_JMPREL:  pltRela= reinterpret_cast<const ElfW(Rela)*>(abs(v));break;
            case DT_PLTRELSZ:pltRelaSz = (size_t)d->d_un.d_val; break;
            case DT_PLTREL:  pltRelType= (uintptr_t)d->d_un.d_val; break;
            case DT_RELA:    rela   = reinterpret_cast<const ElfW(Rela)*>(abs(v));break;
            case DT_RELASZ:  relaSz  = (size_t)d->d_un.d_val; break;
            default: break;
        }
    }
    if (!syms || !strtab) return 0;

    MapsHookTarget targets[] = {
        { "read",  reinterpret_cast<void*>(hookedRead),
          reinterpret_cast<void**>(&g_realRead),  &g_readSlots  },
        { "open",  reinterpret_cast<void*>(hookedOpen),
          reinterpret_cast<void**>(&g_realOpen),  &g_openSlots  },
        { "close", reinterpret_cast<void*>(hookedClose),
          reinterpret_cast<void**>(&g_realClose), &g_openSlots  },
    };
    const size_t nT = sizeof(targets) / sizeof(targets[0]);

    if (pltRela && pltRelaSz && pltRelType == DT_RELA)
        patchRel(base, pltRela, pltRelaSz / sizeof(ElfW(Rela)), syms, strtab, targets, nT);
    if (rela && relaSz)
        patchRel(base, rela, relaSz / sizeof(ElfW(Rela)), syms, strtab, targets, nT);

    return 0;
}

// ── public API ───────────────────────────────────────────────────────────────

inline void install() {
    if (g_active.exchange(true)) return;   // idempotent

    void* libc = dlopen("libc.so", RTLD_NOW | RTLD_NOLOAD);
    if (!libc) libc = dlopen("libc.so", RTLD_NOW | RTLD_LOCAL);
    if (libc) {
        if (!g_realRead)  g_realRead  = reinterpret_cast<ReadFn> (dlsym(libc, "read"));
        if (!g_realOpen)  g_realOpen  = reinterpret_cast<OpenFn> (dlsym(libc, "open"));
        if (!g_realClose) g_realClose = reinterpret_cast<CloseFn>(dlsym(libc, "close"));
    }

    dl_iterate_phdr(patchModule, nullptr);

    MAPS_LOGD("maps filter installed: readSlots=%d openSlots=%d",
              g_readSlots.load(), g_openSlots.load());
}

} // namespace astra::stealth::maps