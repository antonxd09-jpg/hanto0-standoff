#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "menu/menu.h"
#include "menu/fonts.h"
#include "dobby.h"

#define LOG_TAG "Hanto0"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surf);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

nl::Menu* g_menu = nullptr;
bool g_initialized = false;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_initialized) {
        LOGI("First eglSwapBuffers call. Initializing ImGui...");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        nl::load_fonts();
        ImGui_ImplOpenGL3_Init(nullptr); // Авто-визначення версії OpenGL
        
        g_menu = new nl::Menu();
        g_menu->init(ImVec2(100, 100), 1.0f);
        g_menu->set_visible(true);
        
        g_initialized = true;
        LOGI("ImGui initialized");
    }

    if (g_initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        g_menu->render();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    return orig_eglSwapBuffers(dpy, surf);
}

__attribute__((constructor))
void on_load() {
    LOGI("Menu loaded!");
    void* target = dlsym(RTLD_DEFAULT, "eglSwapBuffers");
    if (target) {
        DobbyHook(target, (void*)hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers");
    } else {
        LOGI("Failed to find eglSwapBuffers");
    }
}
