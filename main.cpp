#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <string>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_ShowMenu = true;
bool g_ESP = false;
bool g_Aimbot = false;
bool g_NoRecoil = false;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;
int32_t (*orig_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** event) = nullptr;

void SetupStyle() {
    // ПРИНУДИТЕЛЬНО СТАВИМ ТЕМНУЮ ТЕМУ
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 6);
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.98f); // Темный
    colors[ImGuiCol_ChildBg]                = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.90f, 0.30f, 0.30f, 1.00f); // Красный
    colors[ImGuiCol_Header]                 = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.90f, 0.30f, 0.30f, 1.00f); // Красный
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
}

bool ToggleButton(const char* label, bool* v) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const float height = ImGui::GetFrameHeight();
    const float width = height * 1.8f;
    const float radius = height * 0.50f;
    const ImVec2 pos = window->DC.CursorPos;
    
    float bb_width = width + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f);
    float bb_height = label_size.y + style.FramePadding.y * 2.0f;
    const ImRect total_bb(pos, ImVec2(pos.x + bb_width, pos.y + bb_height));
    
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) return false;
    
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) { *v = !(*v); ImGui::MarkItemEdited(id); }
    
    const ImRect check_bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::RenderNavHighlight(total_bb, id);
    
    ImU32 col_bg = *v ? IM_COL32(230, 80, 80, 255) : IM_COL32(50, 50, 50, 255);
    ImU32 col_circle = IM_COL32(255, 255, 255, 255);
    
    window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, col_bg, height * 0.5f);
    float circle_x = *v ? check_bb.Max.x - radius - 2.0f : check_bb.Min.x + radius + 2.0f;
    window->DrawList->AddCircleFilled(ImVec2(circle_x, check_bb.Min.y + radius + 1.0f), radius - 1.0f, col_circle);
    
    if (label_size.x > 0.0f) {
        ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);
    }
    return pressed;
}

void RenderMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (!g_ShowMenu) {
        ImGui::SetNextWindowPos(ImVec2(50, 100), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_Always);
        ImGui::Begin("##OpenMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove);
        if (ImGui::Button("OPEN MENU", ImVec2(200, 80))) {
            g_ShowMenu = true;
        }
        ImGui::End();
    }

    if (g_ShowMenu) {
        ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("HantoO", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "HantoO");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.0f), "| Private Build");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Columns(2, "main_cols", false);

        ImGui::BeginChild("left_col", ImVec2(0, 0), true);
        ImGui::Text("Aimbot"); ImGui::Spacing();
        ToggleButton("Enable Aimbot", &g_Aimbot);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("ESP Settings"); ImGui::Spacing();
        ToggleButton("Enable ESP", &g_ESP);
        ImGui::EndChild();
        
        ImGui::NextColumn();

        ImGui::BeginChild("right_col", ImVec2(0, 0), true);
        ImGui::Text("Miscellaneous"); ImGui::Spacing();
        ToggleButton("No Recoil", &g_NoRecoil);
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::Text("Info");
        ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "HantoO Internal");
        ImGui::Spacing();
        if (ImGui::Button("Hide Menu", ImVec2(150, 40))) {
            g_ShowMenu = false;
        }
        ImGui::EndChild();
        ImGui::Columns(1);

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
        io.Fonts->AddFontDefault();
        io.FontGlobalScale = 1.8f;
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

void ProcessTouch(AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        float x = AMotionEvent_getX(event, 0);
        float y = AMotionEvent_getY(event, 0);
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(x, y);
        
        int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            io.AddMouseButtonEvent(0, true);
        } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            io.AddMouseButtonEvent(0, false);
        }
    }
}

int32_t hooked_AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
    ProcessTouch(event);
    if (ImGui::GetIO().WantCaptureMouse) return 1; 
    return orig_AInputQueue_preDispatchEvent(queue, event);
}

int32_t hooked_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** event) {
    int32_t res = orig_AInputQueue_getEvent(queue, event);
    if (res >= 0 && event && *event) {
        ProcessTouch(*event);
    }
    return res;
}

__attribute__((constructor))
void init_cheat() {
    LOGI("HantoO loading...");
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
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
