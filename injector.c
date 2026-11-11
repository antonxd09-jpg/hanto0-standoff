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

static pid_t target_tid = -1;
static struct arm64_regs orig_regs;
static struct iovec io = { &orig_regs, sizeof(orig_regs) };
static int attached = 0;

void alarm_handler(int sig) {
    printf("[!] Timeout reached, detaching...\n");
    if (attached) {
        ptrace(PTRACE_SETREGSET, target_tid, (void*)NT_PRSTATUS, &io);
        ptrace(PTRACE_DETACH, target_tid, NULL, NULL);
    }
    exit(1);
}

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

    target_tid = pid; // Беремо головний потік

    void* local_dlopen = dlsym(RTLD_DEFAULT, "dlopen");
    if (!local_dlopen) { printf("[!] dlsym failed\n"); return 1; }

    unsigned long local_libdl = get_base(getpid(), "libdl.so");
    unsigned long local_libc  = get_base(getpid(), "libc.so");
    unsigned long target_dlopen = 0;

    if (local_libdl) target_dlopen = get_base(pid, "libdl.so") + ((unsigned long)local_dlopen - local_libdl);
    else if (local_libc) target_dlopen = get_base(pid, "libc.so") + ((unsigned long)local_dlopen - local_libc);

    if (!target_dlopen) { printf("[!] dlopen not found\n"); return 1; }

    printf("[+] Target TID: %d, dlopen at %lx\n", target_tid, target_dlopen);

    if (ptrace(PTRACE_ATTACH, target_tid, NULL, NULL) < 0) {
        printf("[!] ptrace attach: %s\n", strerror(errno)); return 1;
    }
    int st; waitpid(target_tid, &st, 0); 
    attached = 1;
    printf("[+] Attached\n");

    if (ptrace(PTRACE_GETREGSET, target_tid, (void*)NT_PRSTATUS, &io) < 0) {
        ptrace(PTRACE_DETACH, target_tid, NULL, NULL); return 1;
    }
    memcpy(&orig_regs, &orig_regs, sizeof(orig_regs));

    unsigned long path = (orig_regs.sp & ~0xF) - 0x1000;
    if (wmem(target_tid, path, so, strlen(so) + 1) < 0) { printf("[!] write path\n"); goto detach; }

    unsigned long ret_addr = orig_regs.pc;
    struct arm64_regs r;
    memset(&r, 0, sizeof(r));
    r.regs[0] = path; r.regs[1] = 1; r.pc = target_dlopen; r.regs[30] = ret_addr; r.sp = orig_regs.sp;
    
    if (ptrace(PTRACE_SETREGSET, target_tid, (void*)NT_PRSTATUS, &io) < 0) { printf("[!] setregs\n"); goto detach; }

    signal(SIGALRM, alarm_handler);
    alarm(5); // Таймаут 5 секунд

    printf("[*] Calling dlopen...\n");
    while (1) {
        ptrace(PTRACE_CONT, target_tid, NULL, NULL);
        waitpid(target_tid, &st, 0);
        if (!WIFSTOPPED(st)) break;
        ptrace(PTRACE_GETREGSET, target_tid, (void*)NT_PRSTATUS, &io);
        if (r.pc == ret_addr) {
            printf("[+] dlopen returned handle: %lx\n", r.regs[0]);
            break;
        }
    }
    alarm(0);

detach:
    memcpy(&r, &orig_regs, sizeof(orig_regs));
    ptrace(PTRACE_SETREGSET, target_tid, (void*)NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, target_tid, NULL, NULL);
    printf("[+] Detached\n");
    return 0;
}
