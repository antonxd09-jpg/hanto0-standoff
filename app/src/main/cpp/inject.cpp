#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <android/log.h>
#include <elf.h>
#include <link.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "HantoInject", __VA_ARGS__)
int inject_lib(pid_t pid, const char* lib_path) {
    LOGD("Attaching to %d...", pid);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) return -1;
    waitpid(pid, NULL, 0);
    LOGD("Attached. Injecting...");
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_client_MainActivity_nativeInject(JNIEnv *env, jobject thiz, jint pid, jstring libPath) {
    const char *path = env->GetStringUTFChars(libPath, 0);
    inject_lib(pid, path);
    env->ReleaseStringUTFChars(libPath, path);
}
