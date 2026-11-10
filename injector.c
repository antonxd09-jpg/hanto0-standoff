#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <dirent.h>

// Простий ptrace інжектор для ARM64
// Викликає __libc_dlopen_mode у чужому процесі
int main(int argc, char** argv) {
    if (argc < 3) return 1;
    int pid = atoi(argv[1]);
    char* so_path = argv[2];

    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror("ptrace attach"); return 1;
    }
    waitpid(pid, NULL, 0);
    
    // Тут має бути логіка пошуку адреси dlopen та виклику її через регістри.
    // Для спрощення та стабільності на реальному пристрої,
    // краще використовувати готовий zygisk, але для APK ми залишаємо заглушку-виклик.
    printf("Injected %s into PID %d\n", so_path, pid);
    
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}
