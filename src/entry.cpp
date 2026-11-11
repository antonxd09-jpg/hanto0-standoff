#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h" // ВИПРАВЛЕНО: додано шлях backends/
#include "menu/menu.h"
#include "dobby.h"

#define LOG_TAG "Hanto0"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surf);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;

nl::Menu* g_menu = nullptr;
bool g_initialized = false;

EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    if (!g_initialized) {
        // Ініціалізуємо ImGui в потоці рендеру гри, коли контекст OpenGL вже активний
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        
        g_menu = new nl::Menu();
        g_menu->init(ImVec2(100, 100), 1.0f);
        g_menu->set_visible(true); // Меню з'явиться одразу
        
        g_initialized = true;
        LOGI("ImGui initialized on GL thread");
    }

    if (g_initialized) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
        g_menu->render(); // Виклик твого меню
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    return orig_eglSwapBuffers(dpy, surf);
}

__attribute__((constructor))
void on_load() {
    LOGI("Menu loaded!");
    FILE* f = fopen("/data/local/tmp/menu_loaded.txt", "w");
    if (f) { fputs("Menu loaded!", f); fclose(f); }

    // Ставимо хук
    void* target = DobbySymbolResolver("libEGL.so", "eglSwapBuffers");
    if (target) {
        DobbyHook(target, (void*)hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
        LOGI("Hooked eglSwapBuffers");
    } else {
        LOGI("Failed to find eglSwapBuffers");
    }
}
