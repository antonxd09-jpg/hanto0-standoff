#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <elf.h>

struct arm64_regs { unsigned long regs[31]; unsigned long sp, pc, pstate; };

static int rmem(pid_t pid, unsigned long addr, void* buf, size_t len) {
    struct iovec l = { buf, len }, r = { (void*)addr, len };
    return process_vm_readv(pid, &l, 1, &r, 1, 0) == (ssize_t)len ? 0 : -1;
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

static unsigned long find_sym(pid_t pid, unsigned long base, const char* name) {
    Elf64_Ehdr eh; if (rmem(pid, base, &eh, sizeof(eh)) < 0) return 0;
    Elf64_Phdr* ph = calloc(eh.e_phnum, sizeof(Elf64_Phdr));
    rmem(pid, base + eh.e_phoff, ph, eh.e_phnum * sizeof(Elf64_Phdr));
    unsigned long dyn = 0, dsz = 0;
    for (int i=0; i<eh.e_phnum; i++) if (ph[i].p_type == PT_DYNAMIC) { dyn = base + ph[i].p_vaddr; dsz = ph[i].p_memsz; break; }
    free(ph); if (!dyn) return 0;
    Elf64_Dyn* d = calloc(1, dsz); rmem(pid, dyn, d, dsz);
    unsigned long str = 0, sym = 0;
    for (size_t i=0; i<dsz/sizeof(Elf64_Dyn); i++) {
        if (d[i].d_tag == DT_STRTAB) str = d[i].d_un.d_ptr;
        else if (d[i].d_tag == DT_SYMTAB) sym = d[i].d_un.d_ptr;
        else if (d[i].d_tag == DT_NULL) break;
    }
    free(d); if (!str || !sym) return 0;
    char n[256]; Elf64_Sym s;
    for (int i=1; i<10000; i++) {
        if (rmem(pid, sym + i * sizeof(Elf64_Sym), &s, sizeof(s)) < 0) break;
        if (s.st_name == 0) continue;
        memset(n, 0, sizeof(n)); if (rmem(pid, str + s.st_name, n, 255) < 0) continue;
        if (strcmp(n, name) == 0) return base + s.st_value;
    }
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) { printf("Usage: %s <pid> <so_path>\n", argv[0]); return 1; }
    pid_t pid = atoi(argv[1]); const char* so = argv[2];
    printf("[*] PID: %d, SO: %s\n", pid, so);
    
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        printf("[!] ptrace attach: %s\n", strerror(errno)); return 1;
    }
    int st; waitpid(pid, &st, 0); printf("[+] Attached\n");
    
    struct arm64_regs r, orig; struct iovec io = { &r, sizeof(r) };
    if (ptrace(PTRACE_GETREGSET, pid, (void*)NT_PRSTATUS, &io) < 0) {
        printf("[!] getregs: %s\n", strerror(errno)); ptrace(PTRACE_DETACH, pid, NULL, NULL); return 1;
    }
    memcpy(&orig, &r, sizeof(r));
    
    unsigned long libc = get_base(pid, "libc.so");
    unsigned long dlopen = find_sym(pid, libc, "dlopen");
    printf("[+] libc: %lx, dlopen: %lx\n", libc, dlopen);
    if (!dlopen) goto detach;
    
    unsigned long path = r.sp - 0x400;
    if (wmem(pid, path, so, strlen(so) + 1) < 0) { printf("[!] write path\n"); goto detach; }
    
    memset(&r, 0, sizeof(r));
    r.regs[0] = path; r.regs[1] = 2; r.pc = dlopen; r.regs[30] = orig.pc; r.sp = orig.sp;
    if (ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io) < 0) { printf("[!] setregs\n"); goto detach; }
    
    printf("[*] Calling dlopen...\n");
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    waitpid(pid, &st, 0);
    if (WIFSTOPPED(st)) printf("[+] Stopped: sig %d\n", WSTOPSIG(st));

detach:
    memcpy(&r, &orig, sizeof(orig));
    ptrace(PTRACE_SETREGSET, pid, (void*)NT_PRSTATUS, &io);
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    printf("[+] Detached\n");
    return 0;
}
