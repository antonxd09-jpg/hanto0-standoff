#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_ShowMenu = true;
bool g_Aimbot = false, g_Visuals = false, g_Misc = false;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
EGLBoolean (*orig_eglSwapBuffersWithDamageKHR)(EGLDisplay dpy, EGLSurface surf, EGLint* rects, EGLint n_rects) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;
int32_t (*orig_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** event) = nullptr;

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(10, 10);
    style.ItemSpacing = ImVec2(10, 12);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.08f, 0.09f, 0.98f); // Темный фон
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.25f, 0.50f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.25f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.90f, 0.30f, 0.30f, 1.00f); // Красный
    colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.20f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.90f, 0.30f, 0.30f, 1.00f); // Красный
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
}

void RenderMenu() {
    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    
    // Убрали NoMove, чтобы можно было таскать
    ImGui::Begin("HantoO Internal", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);

    ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "HantoO");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "| Private Build");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("TopTabs")) {
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Spacing();
            ImGui::Checkbox("Enable Aimbot", &g_Aimbot);
            ImGui::Spacing();
            if (g_Aimbot) {
                ImGui::Text("Aimbot Settings Here...");
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            ImGui::Spacing();
            ImGui::Checkbox("Enable ESP", &g_Visuals);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Spacing();
            ImGui::Checkbox("No Recoil", &g_Misc);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void SetupImGui(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = 1.3f;
        ImGui_ImplOpenGL3_Init("#version 100");
        SetupStyle();
        init = true;
    }
    
    EGLint width = 1920, height = 1080;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &height);
    ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    RenderMenu();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    SetupImGui(dpy, surf);
    return orig_eglSwapBuffers(dpy, surf);
}

EGLBoolean hooked_eglSwapBuffersWithDamageKHR(EGLDisplay dpy, EGLSurface surf, EGLint* rects, EGLint n_rects) {
    SetupImGui(dpy, surf);
    return orig_eglSwapBuffersWithDamageKHR(dpy, surf, rects, n_rects);
}

void ProcessEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t maskedAction = action & AMOTION_EVENT_ACTION_MASK;
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(x, y);
        
        if (maskedAction == AMOTION_EVENT_ACTION_DOWN || maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.AddMouseButtonEvent(0, true);
        } else if (maskedAction == AMOTION_EVENT_ACTION_UP || maskedAction == AMOTION_EVENT_ACTION_POINTER_UP || maskedAction == AMOTION_EVENT_ACTION_CANCEL) {
            io.AddMouseButtonEvent(0, false);
        }
    }
}

int32_t hooked_AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    ProcessEvent(event);
    if (ImGui::GetIO().WantCaptureMouse) return 1; 
    return orig_AInputQueue_preDispatchEvent(queue, event);
}

int32_t hooked_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** event) {
    int32_t ret = orig_AInputQueue_getEvent(queue, event);
    if (ret >= 0 && event && *event) {
        ProcessEvent(*event);
    }
    return ret;
}

__attribute__((constructor))
void init_cheat() {
    LOGI("HantoO loading...");
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        
        void* swapdmg = dlsym(egl, "eglSwapBuffersWithDamageKHR");
        if (swapdmg) A64HookFunction(swapdmg, (void*)hooked_eglSwapBuffersWithDamageKHR, (void**)&orig_eglSwapBuffersWithDamageKHR);
    }
    void* android_lib = dlopen("libandroid.so", RTLD_LAZY);
    if (android_lib) {
        void* pre_dispatch = dlsym(android_lib, "AInputQueue_preDispatchEvent");
        if (pre_dispatch) A64HookFunction(pre_dispatch, (void*)hooked_AInputQueue_preDispatchEvent, (void**)&orig_AInputQueue_preDispatchEvent);
        
        void* get_event = dlsym(android_lib, "AInputQueue_getEvent");
        if (get_event) A64HookFunction(get_event, (void*)hooked_AInputQueue_getEvent, (void**)&orig_AInputQueue_getEvent);
    }
    LOGI("Hooks installed!");
}
