#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

std::atomic<bool> g_menuOpen{true};
float g_uiScale = 1.0f, g_menuAnimation = 0.0f;
int g_section = 0;

struct EspAppearance {
    bool box=true, health=true;
    ImVec4 accent{0.59f,0.68f,1.0f,1.0f};
};
EspAppearance g_appearance;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
EGLBoolean (*orig_eglSwapBuffersWithDamageKHR)(EGLDisplay dpy, EGLSurface surf, EGLint* rects, EGLint n_rects) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;
int32_t (*orig_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** event) = nullptr;

ImU32 uiColor(ImVec4 c,float alpha=1.0f){c.w*=alpha*ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
ImVec2 uiAdd(ImVec2 a,float x,float y){return ImVec2(a.x+x,a.y+y);}

void uiTextLeft(ImDrawList* d,ImVec2 p,const char* text,ImU32 color,float size=0){
    ImFont* font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;
    d->AddText(font,size,p,color,text);
}

void uiPanel(ImDrawList* d,ImVec2 a,ImVec2 b,float rounding=14){
    d->AddRectFilled(a,b,uiColor(ImVec4(0.047f,0.055f,0.075f,0.985f)),rounding*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(0.19f,0.22f,0.29f,0.65f)),rounding*g_uiScale,0,1.0f);
}

bool uiRailButton(const char* label,int icon,bool selected,float width){
    const float h=68*g_uiScale;const ImVec2 a=ImGui::GetCursorScreenPos();
    const bool pressed=ImGui::InvisibleButton(label,ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
    if(selected)d->AddRectFilled(a,uiAdd(a,width,h),uiColor(ImVec4(0.10f,0.13f,0.21f,0.88f)),11*g_uiScale);
    uiTextLeft(d,uiAdd(a, 15*g_uiScale, 25*g_uiScale),label,uiColor(selected?ImVec4(0.93f,0.95f,1,1):ImVec4(0.54f,0.60f,0.71f,1)),18*g_uiScale);
    return pressed;
}

bool uiToggle(const char* label,bool& value){
    ImGui::PushID(label);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=48*g_uiScale;
    if(w < 10) { ImGui::PopID(); return false; }
    const bool hit=ImGui::InvisibleButton("##switch",ImVec2(w,h));if(hit)value=!value;
    auto* d=ImGui::GetWindowDrawList();const float sh=26*g_uiScale,sw=48*g_uiScale;
    const ImVec2 p=uiAdd(a,w-sw,0.5f*(h-sh));ImVec4 off(0.15f,0.18f,0.24f,1),on=g_appearance.accent;
    ImVec4 track = value ? on : off;
    d->AddRectFilled(p,uiAdd(p,sw,sh),uiColor(track),sh*0.5f);
    d->AddCircleFilled(uiAdd(p, (value ? sw-sh : 0) + sh*0.5f, sh*0.5f),9.4f*g_uiScale,uiColor(ImVec4(0.97f,0.98f,1,1)),20);
    uiTextLeft(d,a,label,uiColor(ImVec4(0.85f,0.89f,0.96f,1)),19*g_uiScale);
    ImGui::PopID();return hit;
}

void renderMinimalMenu(){
    auto& io=ImGui::GetIO();g_uiScale=std::min({io.DisplaySize.y/1080.0f,io.DisplaySize.x/1440.0f,1.25f});
    g_uiScale=std::max(0.25f,g_uiScale);io.FontGlobalScale=g_uiScale;
    
    const bool open=g_menuOpen.load();if(!open) return;
    const float s=g_uiScale,w=std::round(820*s),h=std::round(650*s);
    
    // РАЗРЕШАЕМ ДВИГАТЬ ОКНО (убрали NoMove и NoDecoration)
    auto flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,6*s));
    
    const char* sectionNames[]={"Aimbot","Visuals","Misc","Settings"};
    const int sectionCount=4;const float primaryH=(24+sectionCount*74)*s;
    
    // ЛЕВОЕ МЕНЮ (Вкладки) - жестко привязано к главному окну
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("HantoO", nullptr, flags);
    uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),w,h));
    
    // Отрисовка сайдбара вручную внутри главного окна
    ImGui::BeginChild("##sidebar", ImVec2(150*s, h - 24*s), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPosY(20*s);
    for(int i=0;i<sectionCount;++i) {
        if(uiRailButton(sectionNames[i], i, g_section==i, 130*s)) g_section=i;
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    // ПРАВАЯ ЧАСТЬ
    ImGui::BeginChild("##content", ImVec2(0, h - 24*s), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetCursorPos(ImVec2(20*s, 20*s));
    
    uiTextLeft(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), "HantoO Internal", uiColor(g_appearance.accent), 24*s);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50*s);

    if(g_section == 0) { // Aimbot
        uiToggle("Enable Aimbot", g_appearance.box);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10*s);
        uiToggle("Silent Aim", g_appearance.health);
    } else if(g_section == 1) { // Visuals
        uiToggle("Enable ESP", g_appearance.box);
    }
    
    ImGui::EndChild();
    
    ImGui::End();
    ImGui::PopStyleVar(2);
}

void SetupImGui(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.Fonts->AddFontDefault();
        ImGui_ImplOpenGL3_Init("#version 100");
        init = true;
    }
    EGLint width = 1920, height = 1080;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &width);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &height);
    ImGui::GetIO().DisplaySize = ImVec2((float)width, (float)height);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    renderMinimalMenu();
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
    // Если кликнули по меню - блокируем передачу в игру
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
}
