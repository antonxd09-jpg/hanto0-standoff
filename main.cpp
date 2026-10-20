#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <mutex>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Переменные меню
bool g_ShowMenu = true;
bool g_FloatingButton = true;
bool g_ESP = false;
bool g_Aimbot = false;
float g_ESPColor[4] = { 0.8f, 0.4f, 1.0f, 1.0f }; // Фиолетовый по умолчанию

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;

// Настройка стиля ImGui (под стиль Zenin)
void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.08f, 0.95f); // Темный фон
    colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.12f, 0.10f, 0.20f, 1.00f); // Фиолетовый оттенок
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.15f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.20f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.50f, 0.25f, 0.80f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.15f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.35f, 0.20f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.8f, 0.4f, 1.0f, 1.00f);
}

void RenderMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Плавающая кнопка "H" (когда меню закрыто)
    if (g_FloatingButton && !g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(50, 200), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(60, 60), ImGuiCond_Once);
        ImGui::Begin("##FloatingBtn", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);
        if (ImGui::Button("H", ImVec2(60, 60))) {
            g_ShowMenu = true;
        }
        ImGui::End();
    }

    // Само меню
    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(650, 450), ImGuiCond_FirstUseEver);
        ImGui::Begin("HantoO Internal", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);

        // Вкладки
        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Visuals")) {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "ESP Settings");
                ImGui::Separator();
                ImGui::Checkbox("Enable ESP", &g_ESP);
                if (g_ESP) {
                    ImGui::ColorEdit4("ESP Color", g_ESPColor);
                    ImGui::SliderInt("Distance", &g_ESP, 0, 100); // Заглушка для слайдера
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Aimbot")) {
                ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "Aimbot Settings");
                ImGui::Separator();
                ImGui::Checkbox("Enable Aimbot", &g_Aimbot);
                ImGui::SliderFloat("Smooth", &g_Aimbot, 0.0f, 10.0f);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Config")) {
                ImGui::Text("Save and load your configs here.");
                if (ImGui::Button("Save Config")) {
                    // Логика сохранения
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Хук отрисовки
EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        ImGui_ImplOpenGL3_Init("#version 100");
        SetupStyle();
        init = true;
    }

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewport[2], (float)viewport[3]);

    RenderMenu();
    return orig_eglSwapBuffers(dpy, surf);
}

// Хук тач-событий (чтобы можно было кликать и перетаскивать)
int32_t hooked_AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t action = AMotionEvent_getAction(event);
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);

        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(x, y);

        if (action == AMOTION_EVENT_ACTION_DOWN) {
            io.MouseDown[0] = true;
        } else if (action == AMOTION_EVENT_ACTION_UP) {
            io.MouseDown[0] = false;
        }

        // Если ImGui хочет захватить клик (например, нажать на кнопку),
        // мы блокируем передачу тача в игру
        if (io.WantCaptureMouse) {
            return 1; // Событие поглощено
        }
    }
    return orig_AInputQueue_preDispatchEvent(queue, event);
}

__attribute__((constructor))
void init_cheat() {
    LOGI("HantoO Internal loading...");
    
    // Хук на отрисовку
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    }

    // Хук на тачи (находится в libandroid.so)
    void* android_lib = dlopen("libandroid.so", RTLD_LAZY);
    if (android_lib) {
        void* pre_dispatch = dlsym(android_lib, "AInputQueue_preDispatchEvent");
        if (pre_dispatch) A64HookFunction(pre_dispatch, (void*)hooked_AInputQueue_preDispatchEvent, (void**)&orig_AInputQueue_preDispatchEvent);
    }
    
    LOGI("Hooks installed!");
}
