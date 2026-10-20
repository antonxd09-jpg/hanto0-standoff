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
bool g_Aimbot = false, g_Silent = false, g_VisibleCheck = false, g_AutoWall = false;
int g_BoneGroup = 0, g_MultipointsScale = 70, g_MinDamage = 0;
bool g_PerWeapon = false, g_BackCamera = false, g_ShotBuffer = false, g_Interpolation = false;
int g_WeaponGroup = 0, g_BufferTicks = 0;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;

void SetupStyle() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(10, 8);
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_Header]                    = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_HeaderHovered]             = ImVec4(0.90f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive]              = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
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

    ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("HantoO", &g_ShowMenu, ImGuiWindowFlags_NoCollapse);

    if (ImGui::BeginTabBar("TopTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Columns(2, "aim_cols", false);
            
            // ЛЕВАЯ КОЛОНКА
            ImGui::Text("Aimbot"); 
            ToggleButton("Enable Aimbot", &g_Aimbot);
            ImGui::Spacing();
            ImGui::Text("Silent");
            ToggleButton("Enable Silent", &g_Silent);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            
            ImGui::Text("Target");
            const char* bones[] = { "Head", "Neck", "Chest" };
            ImGui::Combo("Bones Group", &g_BoneGroup, bones, IM_ARRAYSIZE(bones));
            ImGui::Text("Multipoints Scale");
            ImGui::SliderInt("##multipoints", &g_MultipointsScale, 0, 100, "%d%%");
            ImGui::Spacing();
            ImGui::Text("Visible Check");
            ToggleButton("##visible", &g_VisibleCheck);
            ImGui::Text("Auto Wall");
            ToggleButton("##autowall", &g_AutoWall);
            ImGui::Text("Min. Damage");
            ImGui::SliderInt("##mindmg", &g_MinDamage, 0, 100);

            ImGui::NextColumn();

            // ПРАВАЯ КОЛОНКА
            ImGui::Text("Per Weapon");
            ToggleButton("##perweapon", &g_PerWeapon);
            ImGui::Text("Weapon Group");
            const char* weapons[] = { "Pistols", "Rifles", "Snipers" };
            ImGui::Combo("##weapongroup", &g_WeaponGroup, weapons, IM_ARRAYSIZE(weapons));
            
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            ImGui::Text("Other");
            ImGui::Text("Back Camera");
            ToggleButton("##backcam", &g_BackCamera);
            ImGui::Text("Shot Buffer");
            ToggleButton("##shotbuffer", &g_ShotBuffer);
            ImGui::Text("Interpolation");
            ToggleButton("##interp", &g_Interpolation);
            ImGui::Text("Buffer");
            ImGui::SliderInt("##buffer", &g_BufferTicks, 0, 10, "%d ticks");

            ImGui::Columns(1);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            ImGui::Text("Visual settings go here...");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Text("Misc settings go here...");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skins")) {
            ImGui::Text("Skin changer here...");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

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
        io.FontGlobalScale = 1.5f; // Увеличенный шрифт
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

int32_t hooked_AInputQueue_preDispatchEvent(AInputQueue* queue, AInputEvent* event) {
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
        
        // Если ImGui перехватил клик (например, нажал на кнопку) - не передаем в игру
        if (io.WantCaptureMouse) {
            return 1; 
        }
    }
    return orig_AInputQueue_preDispatchEvent(queue, event);
}

__attribute__((constructor))
void init_cheat() {
    LOGI("HantoO Zenin loading...");
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    }
    void* android_lib = dlopen("libandroid.so", RTLD_LAZY);
    if (android_lib) {
        void* pre_dispatch = dlsym(android_lib, "AInputQueue_preDispatchEvent");
        if (pre_dispatch) A64HookFunction(pre_dispatch, (void*)hooked_AInputQueue_preDispatchEvent, (void**)&orig_AInputQueue_preDispatchEvent);
    }
    LOGI("Hooks installed!");
}
