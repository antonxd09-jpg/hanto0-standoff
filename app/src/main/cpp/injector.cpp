// Language: C++20 | File: injector.cpp | Runtime: Android Root CLI (ARM64) | Target: com.axlebolt.standoff2
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <elf.h>
#include <asm/ptrace.h>

static pid_t find_pid(const char* pkg) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        pid_t pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        char path[64], cmd[256] = {0};
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        int fd = open(path, O_RDONLY);
        if (fd >= 0) {
            read(fd, cmd, sizeof(cmd) - 1);
            close(fd);
            if (strcmp(cmd, pkg) == 0) {
                closedir(dir);
                return pid;
            }
        }
    }
    closedir(dir);
    return -1;
}

static uintptr_t get_module_base(pid_t pid, const char* mod) {
    char path[64], line[512];
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* fp = fopen(path, "r");
    if (!fp) return 0;
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, mod)) {
            base = strtoull(line, nullptr, 16);
            break;
        }
    }
    fclose(fp);
    return base;
}

static uintptr_t get_remote_func(pid_t target_pid, const char* mod_name, void* local_func) {
    uintptr_t local_base = get_module_base(getpid(), mod_name);
    uintptr_t remote_base = get_module_base(target_pid, mod_name);
    if (!local_base || !remote_base) return 0;
    return remote_base + (reinterpret_cast<uintptr_t>(local_func) - local_base);
}

static bool get_regs(pid_t pid, user_pt_regs& regs) {
    iovec io{&regs, sizeof(regs)};
    return ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &io) == 0;
}

static bool set_regs(pid_t pid, const user_pt_regs& regs) {
    iovec io{const_cast<user_pt_regs*>(&regs), sizeof(regs)};
    return ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &io) == 0;
}

static void write_mem(pid_t pid, uintptr_t addr, const void* buf, size_t len) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/mem", pid);
    int fd = open(path, O_RDWR);
    if (fd >= 0) {
        pwrite(fd, buf, len, addr);
        close(fd);
    }
}

static uintptr_t call_remote(pid_t pid, uintptr_t func_addr, const uintptr_t* args, int argc) {
    user_pt_regs orig{}, regs{};
    get_regs(pid, orig);
    regs = orig;
    for (int i = 0; i < argc && i < 8; ++i) regs.regs[i] = args[i];
    regs.pc = func_addr;
    regs.regs[30] = 0;
    set_regs(pid, regs);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    int status = 0;
    waitpid(pid, &status, WUNTRACED);
    get_regs(pid, regs);
    set_regs(pid, orig);
    return regs.regs[0];
}

int main(int argc, char** argv) {
    if (argc < 3) return 1;
    pid_t pid = find_pid(argv[1]);
    if (pid <= 0) return 2;

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) return 3;
    int status = 0;
    waitpid(pid, &status, WUNTRACED);

    uintptr_t remote_mmap = get_remote_func(pid, "libc.so", reinterpret_cast<void*>(mmap));
    uintptr_t remote_dlopen = get_remote_func(pid, "libdl.so", reinterpret_cast<void*>(dlopen));
    if (!remote_dlopen) remote_dlopen = get_remote_func(pid, "linker64", reinterpret_cast<void*>(dlopen));

    uintptr_t mmap_args[6] = {0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, static_cast<uintptr_t>(-1), 0};
    uintptr_t remote_buf = call_remote(pid, remote_mmap, mmap_args, 6);
    if (remote_buf) {
        write_mem(pid, remote_buf, argv[2], strlen(argv[2]) + 1);
        uintptr_t dl_args[2] = {remote_buf, RTLD_NOW | RTLD_GLOBAL};
        call_remote(pid, remote_dlopen, dl_args, 2);
    }

    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
    return 0;
}
