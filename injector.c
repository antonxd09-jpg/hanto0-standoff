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

int main(int argc, char** argv) {
    if (argc < 3) return 1;
    pid_t pid = atoi(argv[1]); const char* so = argv[2];
    printf("[*] PID: %d, SO: %s\n", pid, so);

    void* local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) { printf("[!] dlsym failed\n"); return 1; }

    unsigned long local_libdl = get_base(getpid(), "libdl.so");
    unsigned long local_libc  = get_base(getpid(), "libc.so");
    unsigned long target_dlopen = 0;

    if (local_libdl) {
        unsigned long offset = (unsigned long)local_dlopen - local_libdl;
        unsigned long target_libdl = get_base(pid, "libdl.so");
        target_dlopen = target_libdl + offset;
    } else if (local_libc) {
        unsigned long offset = (unsigned long)local_dlopen - local_libc;
        unsigned long target_libc = get_base(pid, "libc.so");
        target_dlopen = target_libc + offset;
    }

    printf("[+] target_dlopen: %lx\n", target_dlopen);
    if (!target_dlopen) { printf("[!] dlopen not found\n"); return 1; }

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        printf("[!] ptrace attach: %s\n", strerror(errno)); return 1;
    }
    int st; waitpid(pid, &st, 0); printf("[+] Attached\n");

    struct arm64_regs r, orig; struct iovec io = { &r, sizeof(r) };
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &io) < 0) {
        ptrace(PTRACE_DETACH, pid, NULL, NULL); return 1;
    }
    memcpy(&orig, &r, sizeof(r));

    unsigned long path = (r.sp & ~0xF) - 0x400;
    if (wmem(pid, path, so, strlen(so) + 1) < 0) { printf("[!] write path\n"); goto detach; }

    // Налаштовуємо регістри для виклику dlopen
    unsigned long ret_addr = orig.pc;
    memset(&r, 0, sizeof(r));
    r.regs[0] = path; r.regs[1] = 2; // RTLD_NOW
    r.pc = target_dlopen; 
    r.regs[30] = ret_addr; // Куди повертатися після dlopen
    r.sp = orig.sp;
    
    if (ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io) < 0) { goto detach; }

    printf("[*] Calling dlopen...\n");
    
    // Чекаємо завершення dlopen, ігноруючи системні сигнали
    while (1) {
        ptrace(PTRACE_CONT, pid, NULL, NULL);
        waitpid(pid, &st, 0);
        if (!WIFSTOPPED(st)) break;
        
        ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &io);
        if (r.pc == ret_addr) {
            // dlopen завершився
            printf("[+] dlopen returned handle: %lx\n", r.regs[0]);
            break;
        }
        // Якщо це просто сигнал (sig 33), продовжуємо далі
    }

detach:
    memcpy(&r, &orig, sizeof(orig));
    ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Detached\n");
    return 0;
}
