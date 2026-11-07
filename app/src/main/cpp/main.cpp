#include <jni.h>
#include <android/log.h>
#include <atomic>
#include <vector>
#include <cmath>
#include "mem.h"
#include "offsets.hpp"
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"hanto",__VA_ARGS__)
static uintptr_t g_base=0;
static std::atomic<bool> v_box{true},v_skel{true},v_line{true},v_chams{false},v_hp{true},v_ammo{true};
static std::atomic<bool> a_silent{true},a_vis{true},a_trig{false};
static std::atomic<int> a_bone{0};

extern "C" JNIEXPORT jboolean JNICALL Java_com_hanto_standoff_Native_attach(JNIEnv*, jobject, jint pid){
    if(!Mem::attach(pid)) return JNI_FALSE;
    g_base=Mem::base("libunity.so");
    LOG("attach pid=%d base=0x%lx",pid,(unsigned long)g_base);
    return JNI_TRUE;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_detach(JNIEnv*, jobject){ Mem::detach(); }
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_hanto_standoff_Native_getEntities(JNIEnv* env, jobject){
    std::vector<float> out;
    if(Mem::ok() && g_base){
        // заготовка: чтение по off::LOCAL_PLAYER, ENTITY_LIST
    }
    jfloatArray a=env->NewFloatArray(out.size());
    env->SetFloatArrayRegion(a,0,out.size(),out.data());
    return a;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setVisual(JNIEnv*, jobject, jboolean b,jboolean s,jboolean l,jboolean c,jboolean h,jboolean am){
    v_box=b;v_skel=s;v_line=l;v_chams=c;v_hp=h;v_ammo=am;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setAim(JNIEnv*, jobject, jboolean en,jboolean vis,jint bone,jfloat fov){
    a_silent=en;a_vis=vis;a_bone=bone;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setTriggerbot(JNIEnv*, jobject, jboolean en){ a_trig=en; }
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_applySkin(JNIEnv*, jobject, jint def){ LOG("skin %d",def); }