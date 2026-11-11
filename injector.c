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
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 3) return 1;
    pid_t pid = atoi(argv[1]); const char* so = argv[2];

    void* local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) { printf("[!] dlsym failed\n"); return 1; }

    Dl_info info;
    if (!dladdr(local_dlopen, &info)) { printf("[!] dladdr failed\n"); return 1; }
    
    printf("[*] local_dlopen in %s\n", info.dli_fname);

    unsigned long target_base = get_base(pid, info.dli_fname);
    if (!target_base) { printf("[!] target_base not found for %s\n", info.dli_fname); return 1; }
    
    unsigned long target_dlopen = target_base + ((unsigned long)local_dlopen - (unsigned long)info.dli_fbase);
    printf("[+] target_dlopen at %lx\n", target_dlopen);

    pid_t tid = pid; // Головний потік

    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) < 0) {
        printf("[!] ptrace attach: %s\n", strerror(errno)); return 1;
    }
    int st; waitpid(tid, &st, 0); 
    printf("[+] Attached\n");

    struct arm64_regs orig_r; struct iovec io = { &orig_r, sizeof(orig_r) };
    if (ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &io) < 0) {
        ptrace(PTRACE_DETACH, tid, NULL, NULL); return 1;
    }

    // Безпечне вирівнювання стеку
    unsigned long sp = orig_r.sp & ~0xF; 
    unsigned long path_addr = sp - 0x200;
    
    if (wmem(tid, path_addr, so, strlen(so) + 1) < 0) { printf("[!] write path\n"); goto detach; }

    struct arm64_regs r = orig_r;
    r.regs[0] = path_addr;
    r.regs[1] = 2; // RTLD_NOW
    r.pc = target_dlopen;
    r.regs[30] = orig_r.pc;
    r.sp = sp - 0x400; // Виділяємо місце під стек
    
    if (ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &io) < 0) { printf("[!] setregs\n"); goto detach; }

    printf("[*] Calling dlopen...\n");
    ptrace(PTRACE_CONT, tid, NULL, NULL);
    waitpid(tid, &st, 0);
    
    if (WIFSTOPPED(st)) {
        ptrace(PTRACE_GETREGSET, tid, (void*)NT_PRSTATUS, &io);
        printf("[+] Stopped: sig %d, ret: %lx\n", WSTOPSIG(st), r.regs[0]);
    } else {
        printf("[!] Process terminated or crashed\n");
    }

detach:
    memcpy(&r, &orig_r, sizeof(orig_r));
    ptrace(PTRACE_SETREGSET, tid, (void*)NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, tid, NULL, NULL);
    printf("[+] Detached\n");
    return 0;
}
