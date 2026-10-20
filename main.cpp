#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_ShowMenu = true;
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;

void RenderMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        ImGui::Begin("HantoO Internal", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);
        
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "HantoO Private Build");
        ImGui::Separator();
        ImGui::Text("Welcome to your own cheat.");
        
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
        // Используем GLES2 шейдер (100), он совместим везде
        ImGui_ImplOpenGL3_Init("#version 100");
        init = true;
    }

    // Берем реальный размер экрана из игры
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewport[2], (float)viewport[3]);

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
            // Ставим хук
            A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
            LOGI("Hook installed!");
        }
    }
}
