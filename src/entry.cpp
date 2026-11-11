#include <android/log.h>
#include <fstream>
__attribute__((constructor))
void on_load() {
    std::ofstream out("/data/data/com.axlebolt.standoff2/cache/loaded.txt");
    out << "Menu loaded!" << std::endl;
    __android_log_print(ANDROID_LOG_INFO, "Hanto0", "Menu loaded!");
}
