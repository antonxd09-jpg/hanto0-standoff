#include <jni.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <android/log.h>

#if __has_include("offsets.hpp")
#include "offsets.hpp"
#define HAS_ARCHIVE_OFFSETS 1
#else
#define HAS_ARCHIVE_OFFSETS 0
#endif

namespace {
    std::mutex g_mutex;
    std::unordered_map<std::string, bool> g_toggles = {
        {"aimbot", false},
        {"silentaim", false},
        {"triggerbot", false},
        {"prediction", false},
        {"esp", false},
        {"skeleton", false},
        {"healthbar", false},
        {"nametag", false},
        {"distance", false},
        {"radar", false},
        {"bunny", false},
        {"speed", false},
        {"norecoil", false},
        {"nospread", false},
        {"rapidfire", false},
        {"nofog", false},
        {"antiss", false},
        {"streamproof", false}
    };

    std::unordered_map<std::string, float> g_sliders = {
        {"fov", 45.0f},
        {"smooth", 5.0f},
        {"bone", 0.0f},
        {"delay", 0.0f},
        {"radar", 160.0f},
        {"speed", 1.0f}
    };
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_hanto0_loader_NativeBridge_setFeatureToggle(JNIEnv* env, jobject /*thiz*/, jstring key, jboolean enabled) {
    const char* k = env->GetStringUTFChars(key, nullptr);
    std::lock_guard<std::mutex> lock(g_mutex);
    g_toggles[k] = static_cast<bool>(enabled);
    __android_log_print(ANDROID_LOG_INFO, "HantoO", "Toggle %s -> %d", k, enabled ? 1 : 0);
    env->ReleaseStringUTFChars(key, k);
    return enabled;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_hanto0_loader_NativeBridge_getFeatureToggle(JNIEnv* env, jobject /*thiz*/, jstring key) {
    const char* k = env->GetStringUTFChars(key, nullptr);
    std::lock_guard<std::mutex> lock(g_mutex);
    bool state = g_toggles[k];
    env->ReleaseStringUTFChars(key, k);
    return static_cast<jboolean>(state);
}

extern "C" JNIEXPORT void JNICALL
Java_com_hanto0_loader_NativeBridge_setSliderValue(JNIEnv* env, jobject /*thiz*/, jstring key, jfloat value) {
    const char* k = env->GetStringUTFChars(key, nullptr);
    std::lock_guard<std::mutex> lock(g_mutex);
    g_sliders[k] = static_cast<float>(value);
    __android_log_print(ANDROID_LOG_INFO, "HantoO", "Slider %s -> %.2f", k, value);
    env->ReleaseStringUTFChars(key, k);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_hanto0_loader_NativeBridge_getArchiveMetadata(JNIEnv* env, jobject /*thiz*/) {
#if HAS_ARCHIVE_OFFSETS
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"offsetsLoaded\":true,\"clientWorldTypeInfo\":\"0x%lX\",\"playerTypeInfo\":\"0x%lX\"}",
             static_cast<unsigned long>(offsets::manager::ClientWorld_TypeInfo),
             static_cast<unsigned long>(offsets::players::odq_TypeInfo));
    return env->NewStringUTF(buf);
#else
    return env->NewStringUTF("{\"offsetsLoaded\":false}");
#endif
}
