#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <dlfcn.h>
#include <dirent.h>

#define NT_PRSTATUS 1
struct arm64_regs { unsigned long regs[31]; unsigned long sp, pc, pstate; };

static int wmem(pid_t pid, unsigned long addr, const void* buf, size_t len) {
    struct iovec l = { (void*)buf, len }, r = { (void*)addr, len };
    return process_vm_writev(pid, &l, 1, &r, 1, 0) == (ssize_t)len ? 0 : -1;
}

static unsigned long get_base(pid_t pid, const char* name) {
    char path[64], line[512]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* f = fopen(path, "r"); if (!f) return 0;
    unsigned long base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, name)) { sscanf(line, "%lx-", &base); break; }
    }
    fclose(f); return base;
}

// Знаходимо фоновий TID, щоб не чіпати головний потік гри
static int get_bg_tid(pid_t pid) {
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/task", pid);
    DIR* dir = opendir(path); if (!dir) return -1;
    struct dirent* entry; int last_tid = -1;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        int tid = atoi(entry->d_name);
        if (tid != pid) last_tid = tid; // Беремо будь-який потік, крім головного
    }
    closedir(dir);
    return last_tid != -1 ? last_tid : pid;
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0); // Вимикаємо буферизацію
    if (argc < 3) return 1;
    pid_t pid = atoi(argv[1]); const char* so = argv[2];

    void* local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) { printf("[!] dlsym failed\n"); return 1; }

    unsigned long local_libdl = get_base(getpid(), "libdl.so");
    unsigned long local_libc  = get_base(getpid(), "libc.so");
    unsigned long target_dlopen = 0;

    if (local_libdl) target_dlopen = get_base(pid, "libdl.so") + ((unsigned long)local_dlopen - local_libdl);
    else if (local_libc) target_dlopen = get_base(pid, "libc.so") + ((unsigned long)local_dlopen - local_libc);

    if (!target_dlopen) { printf("[!] dlopen not found\n"); return 1; }

    int tid = get_bg_tid(pid);
    printf("[+] Target TID: %d\n", tid);

    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) < 0) {
        printf("[!] ptrace attach: %s\n", strerror(errno)); return 1;
    }
    int st; waitpid(tid, &st, 0); printf("[+] Attached\n");

    struct arm64_regs r, orig; struct iovec io = { &r, sizeof(r) };
    if (ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &io) < 0) {
        ptrace(PTRACE_DETACH, tid, NULL, NULL); return 1;
    }
    memcpy(&orig, &r, sizeof(r));

    unsigned long path = (r.sp & ~0xF) - 0x1000;
    if (wmem(tid, path, so, strlen(so) + 1) < 0) { printf("[!] write path\n"); goto detach; }

    unsigned long ret_addr = orig.pc;
    memset(&r, 0, sizeof(r));
    r.regs[0] = path; r.regs[1] = 1; r.pc = target_dlopen; r.regs[30] = ret_addr; r.sp = orig.sp;
    if (ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &io) < 0) { printf("[!] setregs\n"); goto detach; }

    printf("[*] Calling dlopen...\n");
    while (1) {
        ptrace(PTRACE_CONT, tid, NULL, NULL);
        waitpid(tid, &st, 0);
        if (!WIFSTOPPED(st)) break;
        ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &io);
        if (r.pc == ret_addr) break;
    }

detach:
    memcpy(&r, &orig, sizeof(orig));
    ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, tid, NULL, NULL);
    printf("[+] Detached\n");
    return 0;
}
