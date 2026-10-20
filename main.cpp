#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_ShowMenu = true;
bool g_ESP = false; // Пока выключено, чтобы не крашить

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;

void RenderMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("HantoO Internal", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
        
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "HantoO Private Build");
        ImGui::Separator();
        
        ImGui::Checkbox("Wallhack (ESP) - SOON", &g_ESP);
        
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        ImGui_ImplOpenGL3_Init("#version 300 es");
        init = true;
    }
    RenderMenu();
    return orig_eglSwapBuffers(dpy, surf);
}

__attribute__((constructor))
void init_cheat() {
    LOGI("HantoO Internal loading...");
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) {
            A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
            LOGI("Hook installed!");
        }
    }
}
