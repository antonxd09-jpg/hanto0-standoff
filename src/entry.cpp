#include <stdio.h>
#include <android/log.h>
__attribute__((constructor))
void on_load() {
    FILE* f = fopen("/data/local/tmp/menu_loaded.txt", "w");
    if (f) { fputs("Menu loaded!", f); fclose(f); }
    __android_log_print(ANDROID_LOG_INFO, "Hanto0", "Menu loaded!");
}
