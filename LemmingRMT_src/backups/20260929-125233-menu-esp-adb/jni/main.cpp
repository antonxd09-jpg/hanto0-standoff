#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "game_offsets.hpp"
#include "esp_runtime.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "fonts/mozilla_text.hpp"
#include <android/input.h>
#include <android/log.h>
#include <jni.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <csignal>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#define LOG_TAG "LemmingRMT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#if __SIZEOF_POINTER__ == 4
#define LR_R_TYPE(info) ELF32_R_TYPE(info)
#define LR_R_SYM(info) ELF32_R_SYM(info)
#define LR_JUMP_SLOT R_ARM_JUMP_SLOT
#define LR_GLOB_DAT R_ARM_GLOB_DAT
#elif __SIZEOF_POINTER__ == 8
#define LR_R_TYPE(info) ELF64_R_TYPE(info)
#define LR_R_SYM(info) ELF64_R_SYM(info)
#define LR_JUMP_SLOT R_AARCH64_JUMP_SLOT
#define LR_GLOB_DAT R_AARCH64_GLOB_DAT
#else
#error Unsupported pointer size.
#endif

namespace {

using SwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
using InputGetEventFn = int32_t (*)(AInputQueue*, AInputEvent**);
using NativeInjectEventFn = jboolean (*)(JNIEnv*, jobject, jobject, jint);
using GetCreatedJavaVMsFn = jint (*)(JavaVM**, jsize, jsize*);

SwapBuffersFn g_realSwapBuffers = nullptr;
InputGetEventFn g_realInputGetEvent = nullptr;
NativeInjectEventFn g_realNativeInjectEvent = nullptr;
JavaVM* g_javaVm = nullptr;
jclass g_motionEventClass = nullptr;
jmethodID g_motionGetActionMasked = nullptr;
jmethodID g_motionGetActionIndex = nullptr;
jmethodID g_motionGetX = nullptr;
jmethodID g_motionGetY = nullptr;
std::atomic<bool> g_started{false};
std::atomic<bool> g_menuOpen{true};
std::atomic<int> g_screenWidth{0};
std::atomic<int> g_screenHeight{0};
std::atomic<int> g_swapSlots{0};
std::atomic<int> g_inputSlots{0};
std::atomic<bool> g_javaInputHookReady{false};
std::atomic<uint64_t> g_javaInputEvents{0};
std::atomic<uintptr_t> g_unityBase{0};
std::atomic<bool> g_offsetProfileReady{false};
volatile sig_atomic_t g_toggleMenuSignal = 0;

EGLContext g_imguiContext = EGL_NO_CONTEXT;
bool g_imguiBackendReady = false;
std::chrono::steady_clock::time_point g_lastImGuiFrame{};
std::atomic<bool> g_espBoxesEnabled{false};
std::atomic<bool> g_espResolverReady{false};

struct PointerEvent { float x; float y; int kind; };
std::mutex g_pointerMutex;
std::vector<PointerEvent> g_pointerEvents;

using EspScreenBox = lemming::esp::ScreenBox;
std::mutex g_espMutex;
std::vector<EspScreenBox> g_espScreenBoxes;

#if defined(__aarch64__)
constexpr const char* kArchitectureLabel = "ARM64 / GLES3";
#else
constexpr const char* kArchitectureLabel = "ARMV7 / GLES3";
#endif

void queuePointerEvent(float x, float y, int kind) {
    std::lock_guard<std::mutex> lock(g_pointerMutex);
    if (g_pointerEvents.size() >= 64) g_pointerEvents.erase(g_pointerEvents.begin());
    g_pointerEvents.push_back({x, y, kind});
}

void configureImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 15.0f;
    style.ChildRounding = 11.0f;
    style.FrameRounding = 9.0f;
    style.PopupRounding = 11.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 9.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.93f, 0.95f, 0.98f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.46f, 0.51f, 0.60f, 1.00f);
    c[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.043f, 0.059f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.16f, 0.19f, 0.25f, 0.85f);
    c[ImGuiCol_FrameBg] = ImVec4(0.075f, 0.090f, 0.122f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.105f, 0.130f, 0.180f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.135f, 0.170f, 0.230f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.075f, 0.090f, 0.122f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.17f, 0.23f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.23f, 0.31f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.68f, 1.00f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.12f, 0.16f, 0.22f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.14f, 0.17f, 0.23f, 1.00f);
}

bool ensureImGui() {
    const EGLContext current = eglGetCurrentContext();
    if (current == EGL_NO_CONTEXT) return false;
    if (g_imguiBackendReady && current == g_imguiContext && ImGui::GetCurrentContext()) return true;
    if (ImGui::GetCurrentContext()) {
        if (g_imguiBackendReady) ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendPlatformName = "LemmingRMT_UnityJNI";
    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
        static_cast<int>(lemming::fonts::kMozillaTextSize), 21.0f, &cfg,
        io.Fonts->GetGlyphRangesCyrillic());
    if (!font) io.Fonts->AddFontDefault();
    configureImGuiStyle();
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        LOGE("ImGui OpenGL3 initialization failed");
        ImGui::DestroyContext();
        g_imguiBackendReady = false;
        return false;
    }
    g_imguiContext = current;
    g_imguiBackendReady = true;
    g_lastImGuiFrame = std::chrono::steady_clock::now();
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOGI("ImGui ready: Mozilla Text, version=%s renderer=%s",
         version ? version : "unknown", renderer ? renderer : "unknown");
    return true;
}

bool toggleSwitch(const char* label, bool* value) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const float h = 24.0f, w = 44.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImGuiID id = window->GetID(label);
    const ImRect box(p, ImVec2(p.x + w, p.y + h));
    ImGui::ItemSize(ImVec2(ImGui::GetContentRegionAvail().x, h));
    if (!ImGui::ItemAdd(box, id)) return false;
    const bool pressed = ImGui::ButtonBehavior(box, id, nullptr, nullptr);
    if (pressed) *value = !*value;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(box.Min, box.Max, *value ? IM_COL32(72,158,255,255) : IM_COL32(45,52,68,255), h*0.5f);
    const float x = *value ? box.Max.x-h*0.5f : box.Min.x+h*0.5f;
    draw->AddCircleFilled(ImVec2(x, box.Min.y+h*0.5f), 9.0f, IM_COL32(244,247,252,255));
    draw->AddText(ImVec2(box.Max.x+12.0f, box.Min.y+1.0f), ImGui::GetColorU32(ImGuiCol_Text), label);
    return pressed;
}

void renderWatermark() {
    ImGui::SetNextWindowPos(ImVec2(16,16), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(188,42), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::Begin("##lemming_watermark", nullptr, ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
    ImGui::InvisibleButton("##watermark_button", ImVec2(188,42));
    const bool clicked = ImGui::IsItemClicked();
    const ImVec2 a=ImGui::GetItemRectMin(), b=ImGui::GetItemRectMax();
    ImDrawList* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(ImVec2(a.x+3,a.y+4),ImVec2(b.x+3,b.y+4),IM_COL32(0,0,0,90),12);
    d->AddRectFilled(a,b,IM_COL32(13,16,23,242),12);
    d->AddRect(a,b,IM_COL32(49,59,76,230),12,0,1);
    d->AddCircleFilled(ImVec2(a.x+20,a.y+21),5,IM_COL32(74,164,255,255));
    d->AddText(ImVec2(a.x+34,a.y+10),IM_COL32(239,244,251,255),"LemmingRMT");
    if (clicked) g_menuOpen.store(true,std::memory_order_relaxed);
    ImGui::End(); ImGui::PopStyleColor(); ImGui::PopStyleVar();
}

void renderEspBoxes() {
    if (!g_espBoxesEnabled.load(std::memory_order_relaxed) || !g_espResolverReady.load(std::memory_order_relaxed)) return;
    std::vector<EspScreenBox> boxes;
    { std::lock_guard<std::mutex> lock(g_espMutex); boxes=g_espScreenBoxes; }
    ImDrawList* d=ImGui::GetBackgroundDrawList();
    for (const auto& b: boxes) {
        const ImU32 col=b.enemy?IM_COL32(255,84,102,245):IM_COL32(82,182,255,235);
        d->AddRect(ImVec2(b.left,b.top),ImVec2(b.right,b.bottom),IM_COL32(0,0,0,190),3,0,4);
        d->AddRect(ImVec2(b.left,b.top),ImVec2(b.right,b.bottom),col,3,0,1.8f);
    }
}

void renderMinimalMenu() {
    renderWatermark(); renderEspBoxes();
    if (!g_menuOpen.load(std::memory_order_relaxed)) return;
    ImGui::SetNextWindowPos(ImVec2(24,72),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430,264),ImGuiCond_Always);
    ImGui::Begin("##lemming_menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    ImDrawList* d=ImGui::GetWindowDrawList();
    const ImVec2 wp=ImGui::GetWindowPos(), ws=ImGui::GetWindowSize();
    d->AddRectFilled(wp,ImVec2(wp.x+4,wp.y+ws.y),IM_COL32(74,164,255,255),15,ImDrawFlags_RoundCornersLeft);
    ImGui::SetCursorPos(ImVec2(22,18)); ImGui::TextUnformatted("LemmingRMT");
    ImGui::SameLine(); ImGui::SetCursorPosY(21); ImGui::TextColored(ImVec4(.43f,.49f,.59f,1),"internal overlay");
    ImGui::SetCursorPos(ImVec2(ws.x-50,12));
    if(ImGui::Button("x",ImVec2(32,32))) g_menuOpen.store(false,std::memory_order_relaxed);
    ImGui::SetCursorPos(ImVec2(22,62));
    const ImVec2 sm=ImGui::GetCursorScreenPos(), sx(sm.x+ws.x-44,sm.y+48);
    d->AddRectFilled(sm,sx,IM_COL32(20,27,38,255),11);
    d->AddCircleFilled(ImVec2(sm.x+18,sm.y+24),5,IM_COL32(77,221,142,255));
    d->AddText(ImVec2(sm.x+32,sm.y+8),IM_COL32(232,238,247,255),"Overlay online");
    d->AddText(ImVec2(sm.x+32,sm.y+27),IM_COL32(119,132,153,255),"Unity JNI input  •  OpenGL ES 3");
    ImGui::SetCursorPos(ImVec2(22,132));
    bool esp=g_espBoxesEnabled.load(std::memory_order_relaxed);
    const bool ready=g_espResolverReady.load(std::memory_order_relaxed);
    ImGui::BeginDisabled(!ready);
    if(toggleSwitch("ESP-боксы",&esp)) g_espBoxesEnabled.store(esp,std::memory_order_relaxed);
    ImGui::EndDisabled();
    ImGui::SetCursorPos(ImVec2(78,164));
    const char* espStatus=lemming::esp::statusText();
    if(ready) ImGui::TextColored(ImVec4(.42f,.84f,.61f,1),"%s  •  игроков: %d",espStatus,lemming::esp::playerCount());
    else ImGui::TextColored(ImVec4(.95f,.55f,.34f,1),"%s",espStatus);
    ImGui::SetCursorPos(ImVec2(22,ws.y-39)); ImGui::Separator(); ImGui::SetCursorPosY(ws.y-27);
    ImGui::TextColored(ImVec4(.42f,.47f,.56f,1),"Mozilla Text  •  Dear ImGui  •  %s",kArchitectureLabel);
    ImGui::End();
}

void drawOverlay(int width,int height) {
    if(width<160||height<120||!ensureImGui()) return;
    std::vector<EspScreenBox> nextEspBoxes;
    {
        std::lock_guard<std::mutex> lock(g_espMutex);
        nextEspBoxes=g_espScreenBoxes;
    }
    const bool espReady=lemming::esp::update(g_unityBase.load(std::memory_order_acquire),width,height,nextEspBoxes);
    g_espResolverReady.store(espReady&&lemming::esp::ready(),std::memory_order_release);
    if(espReady){
        std::lock_guard<std::mutex> lock(g_espMutex);
        g_espScreenBoxes.swap(nextEspBoxes);
    }
    ImGuiIO& io=ImGui::GetIO(); io.DisplaySize=ImVec2((float)width,(float)height);
    const auto now=std::chrono::steady_clock::now();
    const float dt=std::chrono::duration<float>(now-g_lastImGuiFrame).count();
    io.DeltaTime=(dt>0&&dt<.25f)?dt:(1.0f/60.0f); g_lastImGuiFrame=now;
    std::vector<PointerEvent> events;
    { std::lock_guard<std::mutex> lock(g_pointerMutex); events.swap(g_pointerEvents); }
    ImGui_ImplOpenGL3_NewFrame();
    for(const auto& e:events){ io.AddMousePosEvent(e.x,e.y); if(e.kind==1)io.AddMouseButtonEvent(0,true); else if(e.kind==2)io.AddMouseButtonEvent(0,false); }
    ImGui::NewFrame(); renderMinimalMenu(); ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool inside(float x, float y, float left, float top, float width, float height) {
    return x >= left && x <= left + width && y >= top && y <= top + height;
}

void handleTouch(float x, float y) {
    const int width = g_screenWidth.load(std::memory_order_relaxed);
    const int height = g_screenHeight.load(std::memory_order_relaxed);
    if (width <= 0 || height <= 0) return;

    if (!g_menuOpen.load(std::memory_order_relaxed)) {
        if (inside(x, y, 16.0f, 16.0f, 196.0f, 36.0f)) {
            g_menuOpen.store(true, std::memory_order_relaxed);
            LOGI("menu opened by Java touch");
        }
        return;
    }

}

bool clearJniException(JNIEnv* env, const char* stage) {
    if (!env || !env->ExceptionCheck()) return false;
    env->ExceptionClear();
    LOGE("JNI exception during %s", stage ? stage : "unknown stage");
    return true;
}

[[maybe_unused]] jboolean hookedNativeInjectEvent(JNIEnv* env, jobject instance, jobject inputEvent, jint displayId) {
    if (env && inputEvent && g_motionEventClass &&
        env->IsInstanceOf(inputEvent, g_motionEventClass) == JNI_TRUE) {
        const jint action = env->CallIntMethod(inputEvent, g_motionGetActionMasked);
        const jint index = env->CallIntMethod(inputEvent, g_motionGetActionIndex);
        if (!clearJniException(env, "reading MotionEvent action") &&
            (action == 0 || action == 1 || action == 2 || action == 3)) {
            const jint pointerIndex = action == 2 ? 0 : index;
            const jfloat x = env->CallFloatMethod(inputEvent, g_motionGetX, pointerIndex);
            const jfloat y = env->CallFloatMethod(inputEvent, g_motionGetY, pointerIndex);
            if (!clearJniException(env, "reading MotionEvent coordinates")) {
                const uint64_t eventNumber = g_javaInputEvents.fetch_add(1, std::memory_order_relaxed) + 1;
                const int kind = action == 0 ? 1 : ((action == 1 || action == 3) ? 2 : 0);
                queuePointerEvent(x, y, kind);
                if (action == 0) {
                    if (eventNumber <= 16) LOGI("Java touch down: x=%.1f y=%.1f display=%d", x, y, displayId);
                    handleTouch(x, y);
                }
            }
        }
    }
    return g_realNativeInjectEvent
        ? g_realNativeInjectEvent(env, instance, inputEvent, displayId)
        : JNI_FALSE;
}

bool installJavaInputHook() {
    if (g_javaInputHookReady.load(std::memory_order_acquire)) return true;
#if !defined(__aarch64__)
    return false;
#else
    constexpr uintptr_t kNativeInjectEventRva = 0x5668AD8;
    const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
    if (!unityBase) return false;

    JavaVM* vm = g_javaVm;
    if (!vm) {
        auto getCreatedVms = reinterpret_cast<GetCreatedJavaVMsFn>(
            dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs"));
        if (!getCreatedVms) {
            const char* candidates[] = {"libart.so", "libnativehelper.so"};
            for (const char* library : candidates) {
                void* handle = dlopen(library, RTLD_NOW | RTLD_LOCAL);
                if (!handle) continue;
                getCreatedVms = reinterpret_cast<GetCreatedJavaVMsFn>(
                    dlsym(handle, "JNI_GetCreatedJavaVMs"));
                if (getCreatedVms) break;
            }
        }
        if (!getCreatedVms) return false;
        jsize count = 0;
        if (getCreatedVms(&vm, 1, &count) != JNI_OK || count < 1 || !vm) return false;
        g_javaVm = vm;
    }

    JNIEnv* env = nullptr;
    bool attachedHere = false;
    const jint envStatus = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) return false;
        attachedHere = true;
    } else if (envStatus != JNI_OK || !env) {
        return false;
    }

    bool installed = false;
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    if (!activityThread || clearJniException(env, "finding ActivityThread")) goto cleanup;
    {
        jmethodID currentApplication = env->GetStaticMethodID(
            activityThread, "currentApplication", "()Landroid/app/Application;");
        if (!currentApplication || clearJniException(env, "finding currentApplication")) goto cleanup;
        jobject application = env->CallStaticObjectMethod(activityThread, currentApplication);
        if (!application || clearJniException(env, "getting current Application")) goto cleanup;

        jclass applicationClass = env->GetObjectClass(application);
        jmethodID getClassLoader = applicationClass
            ? env->GetMethodID(applicationClass, "getClassLoader", "()Ljava/lang/ClassLoader;")
            : nullptr;
        if (!getClassLoader || clearJniException(env, "finding getClassLoader")) {
            if (applicationClass) env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }
        jobject classLoader = env->CallObjectMethod(application, getClassLoader);
        if (!classLoader || clearJniException(env, "getting app ClassLoader")) {
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
        jmethodID loadClass = classLoaderClass
            ? env->GetMethodID(classLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;")
            : nullptr;
        jstring unityName = env->NewStringUTF("com.unity3d.player.UnityPlayer");
        jobject unityClassObject = (loadClass && unityName)
            ? env->CallObjectMethod(classLoader, loadClass, unityName)
            : nullptr;
        if (!unityClassObject || clearJniException(env, "loading UnityPlayer class")) {
            if (unityName) env->DeleteLocalRef(unityName);
            if (classLoaderClass) env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jclass motionEventLocal = env->FindClass("android/view/MotionEvent");
        jclass motionGlobal = motionEventLocal
            ? reinterpret_cast<jclass>(env->NewGlobalRef(motionEventLocal))
            : nullptr;
        if (!motionGlobal || clearJniException(env, "preparing MotionEvent")) {
            if (motionEventLocal) env->DeleteLocalRef(motionEventLocal);
            env->DeleteLocalRef(unityClassObject);
            env->DeleteLocalRef(unityName);
            env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        jmethodID getActionMasked = env->GetMethodID(motionGlobal, "getActionMasked", "()I");
        jmethodID getActionIndex = env->GetMethodID(motionGlobal, "getActionIndex", "()I");
        jmethodID getX = env->GetMethodID(motionGlobal, "getX", "(I)F");
        jmethodID getY = env->GetMethodID(motionGlobal, "getY", "(I)F");
        if (!getActionMasked || !getActionIndex || !getX || !getY ||
            clearJniException(env, "resolving MotionEvent methods")) {
            env->DeleteGlobalRef(motionGlobal);
            env->DeleteLocalRef(motionEventLocal);
            env->DeleteLocalRef(unityClassObject);
            env->DeleteLocalRef(unityName);
            env->DeleteLocalRef(classLoaderClass);
            env->DeleteLocalRef(classLoader);
            env->DeleteLocalRef(applicationClass);
            env->DeleteLocalRef(application);
            goto cleanup;
        }

        g_realNativeInjectEvent = reinterpret_cast<NativeInjectEventFn>(
            unityBase + kNativeInjectEventRva);
        JNINativeMethod method{
            const_cast<char*>("nativeInjectEvent"),
            const_cast<char*>("(Landroid/view/InputEvent;I)Z"),
            reinterpret_cast<void*>(hookedNativeInjectEvent)
        };
        const jint result = env->RegisterNatives(
            reinterpret_cast<jclass>(unityClassObject), &method, 1);
        if (result == JNI_OK && !clearJniException(env, "registering nativeInjectEvent")) {
            g_motionEventClass = motionGlobal;
            g_motionGetActionMasked = getActionMasked;
            g_motionGetActionIndex = getActionIndex;
            g_motionGetX = getX;
            g_motionGetY = getY;
            g_javaInputHookReady.store(true, std::memory_order_release);
            installed = true;
            LOGI("Java input hook ready: nativeInjectEvent RVA=0x%llx",
                 static_cast<unsigned long long>(kNativeInjectEventRva));
        } else {
            env->DeleteGlobalRef(motionGlobal);
            clearJniException(env, "RegisterNatives failure");
        }

        env->DeleteLocalRef(motionEventLocal);
        env->DeleteLocalRef(unityClassObject);
        env->DeleteLocalRef(unityName);
        env->DeleteLocalRef(classLoaderClass);
        env->DeleteLocalRef(classLoader);
        env->DeleteLocalRef(applicationClass);
        env->DeleteLocalRef(application);
    }
cleanup:
    if (activityThread) env->DeleteLocalRef(activityThread);
    if (attachedHere) vm->DetachCurrentThread();
    return installed;
#endif
}

EGLBoolean hookedSwapBuffers(EGLDisplay display, EGLSurface surface) {
    EGLint width = 0;
    EGLint height = 0;
    if (surface != EGL_NO_SURFACE &&
        eglQuerySurface(display, surface, EGL_WIDTH, &width) == EGL_TRUE &&
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) == EGL_TRUE) {
        g_screenWidth.store(width, std::memory_order_relaxed);
        g_screenHeight.store(height, std::memory_order_relaxed);
        drawOverlay(width, height);
    }
    return g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE;
}

int32_t hookedInputGetEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!g_realInputGetEvent) return -1;
    const int32_t result = g_realInputGetEvent(queue, outEvent);
    if (result >= 0 && outEvent && *outEvent && AInputEvent_getType(*outEvent) == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t action = AMotionEvent_getAction(*outEvent);
        const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
        const size_t index = static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                                  AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            const size_t count = AMotionEvent_getPointerCount(*outEvent);
            if (index < count) {
                handleTouch(AMotionEvent_getX(*outEvent, index), AMotionEvent_getY(*outEvent, index));
            }
        }
    }
    return result;
}

uintptr_t makeAbsolute(uintptr_t base, uintptr_t value, uintptr_t minAddress, uintptr_t maxAddress) {
    if (value >= minAddress && value < maxAddress) return value;
    return base + value;
}

bool patchSlot(void** slot, void* replacement, void** original) {
    if (!slot || !replacement) return false;
    void* current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (current == replacement) return false;
    if (original && !*original && current) *original = current;

    const long pageSize = sysconf(_SC_PAGESIZE);
    const uintptr_t page = reinterpret_cast<uintptr_t>(slot) & ~(static_cast<uintptr_t>(pageSize) - 1u);
    if (mprotect(reinterpret_cast<void*>(page), static_cast<size_t>(pageSize), PROT_READ | PROT_WRITE) != 0) {
        return false;
    }
    __atomic_store_n(slot, replacement, __ATOMIC_RELEASE);
    __builtin___clear_cache(reinterpret_cast<char*>(slot), reinterpret_cast<char*>(slot) + sizeof(void*));
    mprotect(reinterpret_cast<void*>(page), static_cast<size_t>(pageSize), PROT_READ);
    return true;
}

struct HookTargets {
    const char* name;
    void* replacement;
    void** original;
    std::atomic<int>* slotCount;
};

template <typename Relocation>
bool patchRelocations(uintptr_t base,
                      const Relocation* relocs,
                      size_t count,
                      const ElfW(Sym)* symbols,
                      const char* strings,
                      const HookTargets* targets,
                      size_t targetCount) {
    bool changed = false;
    for (size_t i = 0; i < count; ++i) {
        const Relocation& rel = relocs[i];
        const unsigned type = LR_R_TYPE(rel.r_info);
        if (type != LR_JUMP_SLOT && type != LR_GLOB_DAT) continue;
        const unsigned symIndex = LR_R_SYM(rel.r_info);
        const char* symbolName = strings + symbols[symIndex].st_name;
        for (size_t t = 0; t < targetCount; ++t) {
            if (std::strcmp(symbolName, targets[t].name) != 0) continue;
            auto** slot = reinterpret_cast<void**>(base + static_cast<uintptr_t>(rel.r_offset));
            if (patchSlot(slot, targets[t].replacement, targets[t].original)) {
                targets[t].slotCount->fetch_add(1, std::memory_order_relaxed);
                changed = true;
            }
        }
    }
    return changed;
}

int hookModule(dl_phdr_info* info, size_t, void*) {
    const char* moduleName = info->dlpi_name ? info->dlpi_name : "";
    if (!std::strstr(moduleName, "libunity.so") &&
        !std::strstr(moduleName, "libil2cpp.so") &&
        !std::strstr(moduleName, "libmain.so")) {
        return 0;
    }

    const uintptr_t base = static_cast<uintptr_t>(info->dlpi_addr);
    uintptr_t minAddress = UINTPTR_MAX;
    uintptr_t maxAddress = 0;
    const ElfW(Dyn)* dynamic = nullptr;

    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& ph = info->dlpi_phdr[i];
        if (ph.p_type == PT_LOAD) {
            minAddress = std::min(minAddress, base + static_cast<uintptr_t>(ph.p_vaddr));
            maxAddress = std::max(maxAddress, base + static_cast<uintptr_t>(ph.p_vaddr + ph.p_memsz));
        } else if (ph.p_type == PT_DYNAMIC) {
            dynamic = reinterpret_cast<const ElfW(Dyn)*>(base + ph.p_vaddr);
        }
    }
    if (!dynamic || minAddress == UINTPTR_MAX) return 0;

    if (std::strstr(moduleName, "libunity.so")) {
        g_unityBase.store(base, std::memory_order_release);
#if defined(__aarch64__)
        const uintptr_t probe = base + lemming::offsets::method_rva::kWeaponryOnPreInitialize;
        g_offsetProfileReady.store(probe >= minAddress && probe < maxAddress, std::memory_order_release);
#else
        g_offsetProfileReady.store(false, std::memory_order_release);
#endif
    }

    const ElfW(Sym)* symbols = nullptr;
    const char* strings = nullptr;
    const void* pltRelocations = nullptr;
    const ElfW(Rel)* regularRel = nullptr;
    const ElfW(Rela)* regularRela = nullptr;
    uintptr_t pltRelType = 0;
    size_t pltRelSize = 0;
    size_t regularRelSize = 0;
    size_t regularRelaSize = 0;

    for (const ElfW(Dyn)* d = dynamic; d->d_tag != DT_NULL; ++d) {
        const uintptr_t value = static_cast<uintptr_t>(d->d_un.d_ptr);
        switch (d->d_tag) {
            case DT_SYMTAB:
                symbols = reinterpret_cast<const ElfW(Sym)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_STRTAB:
                strings = reinterpret_cast<const char*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_JMPREL:
                pltRelocations = reinterpret_cast<const void*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_PLTRELSZ:
                pltRelSize = static_cast<size_t>(d->d_un.d_val);
                break;
            case DT_PLTREL:
                pltRelType = static_cast<uintptr_t>(d->d_un.d_val);
                break;
            case DT_REL:
                regularRel = reinterpret_cast<const ElfW(Rel)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_RELSZ:
                regularRelSize = static_cast<size_t>(d->d_un.d_val);
                break;
            case DT_RELA:
                regularRela = reinterpret_cast<const ElfW(Rela)*>(makeAbsolute(base, value, minAddress, maxAddress));
                break;
            case DT_RELASZ:
                regularRelaSize = static_cast<size_t>(d->d_un.d_val);
                break;
            default:
                break;
        }
    }
    if (!symbols || !strings) return 0;

    HookTargets targets[] = {
        {"eglSwapBuffers", reinterpret_cast<void*>(hookedSwapBuffers), reinterpret_cast<void**>(&g_realSwapBuffers), &g_swapSlots},
        {"AInputQueue_getEvent", reinterpret_cast<void*>(hookedInputGetEvent), reinterpret_cast<void**>(&g_realInputGetEvent), &g_inputSlots},
    };

    if (pltRelocations && pltRelSize) {
        if (pltRelType == DT_REL) {
            const auto* relocs = reinterpret_cast<const ElfW(Rel)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, 2);
        } else if (pltRelType == DT_RELA) {
            const auto* relocs = reinterpret_cast<const ElfW(Rela)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rela)), symbols, strings, targets, 2);
        }
    }
    if (regularRel && regularRelSize) {
        patchRelocations(base, regularRel, regularRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, 2);
    }
    if (regularRela && regularRelaSize) {
        patchRelocations(base, regularRela, regularRelaSize / sizeof(ElfW(Rela)), symbols, strings, targets, 2);
    }
    return 0;
}

void resolveOriginals() {
    void* egl = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (egl) g_realSwapBuffers = reinterpret_cast<SwapBuffersFn>(dlsym(egl, "eglSwapBuffers"));
    void* android = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (android) g_realInputGetEvent = reinterpret_cast<InputGetEventFn>(dlsym(android, "AInputQueue_getEvent"));
}

void handleToggleSignal(int) {
    g_toggleMenuSignal = 1;
}

void hookWorker() {
    resolveOriginals();
    int lastSwap = -1;
    int lastInput = -1;
    uintptr_t lastUnityBase = 0;
    bool lastProfileReady = false;
    for (;;) {
        if (g_toggleMenuSignal) {
            g_toggleMenuSignal = 0;
            g_menuOpen.store(!g_menuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
            LOGI("menu toggled by SIGUSR1: open=%d", g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0);
        }
        dl_iterate_phdr(hookModule, nullptr);
        if (!g_javaInputHookReady.load(std::memory_order_acquire)) {
            installJavaInputHook();
        }
        const int swaps = g_swapSlots.load(std::memory_order_relaxed);
        const int inputs = g_inputSlots.load(std::memory_order_relaxed);
        if (swaps != lastSwap || inputs != lastInput) {
            LOGI("hook status: swap=%d nativeInput=%d javaInput=%d signalToggle=ready", swaps, inputs,
                 g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0);
            lastSwap = swaps;
            lastInput = inputs;
        }
        const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
        const bool profileReady = g_offsetProfileReady.load(std::memory_order_acquire);
        if (unityBase != lastUnityBase || profileReady != lastProfileReady) {
            LOGI("dump64 profile: base=%p ready=%d source=%s", reinterpret_cast<void*>(unityBase),
                 profileReady ? 1 : 0, lemming::offsets::kProfileName);
            lastUnityBase = unityBase;
            lastProfileReady = profileReady;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void start() {
    bool expected = false;
    if (!g_started.compare_exchange_strong(expected, true)) return;
    struct sigaction action{};
    action.sa_handler = handleToggleSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &action, nullptr);
    std::thread(hookWorker).detach();
    LOGI("bootstrap started");
}

} // namespace

extern "C" __attribute__((visibility("default"))) void LemmingRMT_ToggleMenu() {
    g_menuOpen.store(!g_menuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetMenuOpen(int open) {
    g_menuOpen.store(open != 0, std::memory_order_relaxed);
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsMenuOpen() {
    return g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0;
}

extern "C" __attribute__((visibility("default"))) uintptr_t LemmingRMT_GetUnityBase() {
    return g_unityBase.load(std::memory_order_acquire);
}

extern "C" __attribute__((visibility("default"))) uintptr_t LemmingRMT_ResolveUnityRva(uintptr_t rva) {
    const uintptr_t base = g_unityBase.load(std::memory_order_acquire);
    return base ? base + rva : 0;
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsOffsetProfileReady() {
    return g_offsetProfileReady.load(std::memory_order_acquire) ? 1 : 0;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_javaVm = vm;
    start();
    return JNI_VERSION_1_6;
}

__attribute__((constructor)) static void LemmingRMT_OnLoad() {
    start();
}
