#include <jni.h>
#include <android/log.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <mutex>
#include <atomic>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

std::atomic<bool> g_menuOpen{true}, g_espBoxesEnabled{true};
float g_uiScale = 1.0f, g_menuAnimation = 0.0f, g_pageAnimation = 1.0f;
int g_section = 0, g_subsection = 0;

struct EspScreenBox {
    float left, top, right, bottom;
    bool statsValid = false, weaponValid = false, enemy = true;
    unsigned int hp = 0, maxHp = 100, armor = 0, maxArmor = 100;
    unsigned int weaponId = 0, magazineAmmo = 0, reserveAmmo = 0;
};
std::mutex g_espMutex;
std::vector<EspScreenBox> g_espScreenBoxes;

struct EspAppearance {
    bool box=true,health=true,armor=true,weapon=true,ammo=true,fill=false,rounded=true;
    int boxStyle=0;
    float lineWidth=1.5f,fillIntensity=0.08f,cornerRadius=6.0f;
    ImVec4 boxColor{0.69f,0.77f,1.0f,1.0f};
    ImVec4 healthColor{0.36f,0.87f,0.65f,1.0f},armorColor{0.43f,0.68f,1.0f,1.0f};
    ImVec4 textColor{0.91f,0.94f,1.0f,1.0f},accent{0.59f,0.68f,1.0f,1.0f};
};
EspAppearance g_appearance;

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;
int32_t (*orig_AInputQueue_preDispatchEvent)(AInputQueue* queue, AInputEvent* event) = nullptr;

float uiEase(float value,float target,float speed=18.0f){return value+(target-value)*(1.0f-std::exp(-speed*std::min(ImGui::GetIO().DeltaTime,0.05f)));}
ImU32 uiColor(ImVec4 c,float alpha=1.0f){c.w*=alpha*ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
ImVec2 uiAdd(ImVec2 a,float x,float y){return ImVec2(a.x+x,a.y+y);}

void uiText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;
    const ImVec2 t=font->CalcTextSizeA(size,FLT_MAX,0,text);
    d->AddText(font,size,ImVec2(center.x-t.x*0.5f,center.y-t.y*0.5f),color,text);
}
void uiTextLeft(ImDrawList* d,ImVec2 p,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;d->AddText(font,size,p,color,text);
}
void uiPanel(ImDrawList* d,ImVec2 a,ImVec2 b,float rounding=14){
    d->AddRectFilled(uiAdd(a,0,6*g_uiScale),uiAdd(b,0,6*g_uiScale),uiColor(ImVec4(0,0,0,0.23f)),rounding*g_uiScale);
    d->AddRectFilled(a,b,uiColor(ImVec4(0.047f,0.055f,0.075f,0.985f)),rounding*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(0.19f,0.22f,0.29f,0.65f)),rounding*g_uiScale,0,1.0f);
}
void uiCard(ImDrawList* d,ImVec2 a,ImVec2 b){
    d->AddRectFilled(a,b,uiColor(ImVec4(.066f,.078f,.104f,1)),12*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(.18f,.21f,.28f,.58f)),12*g_uiScale);
}
bool uiRailButton(const char* label,int icon,bool selected,float width){
    const float h=68*g_uiScale;const ImVec2 a=ImGui::GetCursorScreenPos();
    const bool pressed=ImGui::InvisibleButton(label,ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
    if(selected)d->AddRectFilled(a,uiAdd(a,width,h),uiColor(ImVec4(0.10f,0.13f,0.21f,0.88f)),11*g_uiScale);
    uiText(d,uiAdd(a,width*0.5f,52*g_uiScale),label,uiColor(selected?ImVec4(0.93f,0.95f,1,1):ImVec4(0.54f,0.60f,0.71f,1)),18*g_uiScale);
    return pressed;
}
bool uiToggle(const char* label,bool& value){
    ImGui::PushID(label);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=48*g_uiScale;
    const bool hit=ImGui::InvisibleButton("##switch",ImVec2(w,h));if(hit)value=!value;
    ImGuiStorage* store=ImGui::GetStateStorage();auto id=ImGui::GetItemID();
    float t=uiEase(store->GetFloat(id,value?1:0),value?1:0,20);store->SetFloat(id,t);
    auto* d=ImGui::GetWindowDrawList();const float sh=26*g_uiScale,sw=48*g_uiScale;
    const ImVec2 p=uiAdd(a,w-sw,0.5f*(h-sh));ImVec4 off(0.15f,0.18f,0.24f,1),on=g_appearance.accent;
    ImVec4 track(off.x+(on.x-off.x)*t,off.y+(on.y-off.y)*t,off.z+(on.z-off.z)*t,1);
    d->AddRectFilled(p,uiAdd(p,sw,sh),uiColor(track),sh*0.5f);
    d->AddCircleFilled(uiAdd(p,sh*0.5f+(sw-sh)*t,sh*0.5f),9.4f*g_uiScale,uiColor(ImVec4(0.97f,0.98f,1,1)),20);
    uiText(d,uiAdd(a,(w-sw-14*g_uiScale)*0.5f,h*0.5f),label,uiColor(ImVec4(0.85f,0.89f,0.96f,1)),19*g_uiScale);
    ImGui::PopID();return hit;
}
void uiSlider(const char* id,const char* label,float& value,float lo,float hi,const char* format){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();
    uiTextLeft(ImGui::GetWindowDrawList(),a,label,uiColor(ImVec4(.68f,.73f,.82f,1)),16*g_uiScale);
    ImGui::SetCursorScreenPos(uiAdd(a,0,23*g_uiScale));ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SliderFloat("##value",&value,lo,hi,format);
    ImGui::SetCursorScreenPos(uiAdd(a,0,64*g_uiScale));ImGui::PopID();
}
void uiColorRow(const char* label,ImVec4& c){
    ImGui::PushID(label);const ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
    uiText(ImGui::GetWindowDrawList(),uiAdd(p,(w-42*g_uiScale)*0.5f,19*g_uiScale),label,uiColor(ImVec4(0.76f,0.81f,0.90f,1)),18*g_uiScale);
    ImGui::SetCursorScreenPos(uiAdd(p,w-34*g_uiScale,2*g_uiScale));
    if(ImGui::ColorButton("##color",c,ImGuiColorEditFlags_NoTooltip,ImVec2(32*g_uiScale,32*g_uiScale)))ImGui::OpenPopup("##picker");
    if(ImGui::BeginPopup("##picker")){
        ImGui::SetNextItemWidth(250*g_uiScale);
        ImGui::ColorPicker4("##rgba",&c.x,ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(uiAdd(p,0,44*g_uiScale));ImGui::PopID();
}
void drawStyledEsp(ImDrawList* d,const EspScreenBox& b){
    const ImVec2 a(b.left,b.top),z(b.right,b.bottom);float h=z.y-a.y,w=z.x-a.x;if(h<8||w<2)return;
    const float center=(a.x+z.x)*0.5f,thick=g_appearance.lineWidth;
    const float rounding=g_appearance.rounded?g_appearance.cornerRadius*g_uiScale:0.0f;const auto boxColor=uiColor(g_appearance.boxColor);
    if(g_appearance.box){
        if(g_appearance.fill)d->AddRectFilled(a,z,uiColor(g_appearance.boxColor,std::clamp(g_appearance.fillIntensity,0.0f,0.35f)),rounding);
        d->AddRect(a,z,boxColor,rounding,0,thick);
    }
}

void renderMinimalMenu(){
    auto& io=ImGui::GetIO();g_uiScale=std::min({io.DisplaySize.y/1080.0f,io.DisplaySize.x/1440.0f,io.DisplaySize.y/760.0f,1.25f});
    g_uiScale=std::max(0.25f,g_uiScale);io.FontGlobalScale=g_uiScale;
    
    const bool open=g_menuOpen.load();g_menuAnimation=uiEase(g_menuAnimation,open?1:0);if(g_menuAnimation<0.005f&&!open)return;
    g_pageAnimation=uiEase(g_pageAnimation,1,15);const float s=g_uiScale,w=std::round(820*s),h=std::round(650*s);
    const float x=std::floor((io.DisplaySize.x-w)*.5f),y=std::floor((io.DisplaySize.y-h)*.5f)+(1-g_menuAnimation)*12*s;
    auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground;
    if(!open)flags|=ImGuiWindowFlags_NoInputs;
    
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,6*s));
    
    const char* sectionNames[]={"Aimbot","Visuals","Misc","Settings"};
    const int sectionCount=4;const float primaryH=(24+sectionCount*74)*s;
    
    // ЛЕВОЕ МЕНЮ
    ImGui::SetNextWindowPos(ImVec2(x-176*s,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(156*s,primaryH),ImGuiCond_Always);
    ImGui::Begin("##sections",nullptr,flags);
    uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),156*s,primaryH/s));
    for(int i=0;i<sectionCount;++i)if(uiRailButton(sectionNames[i],i,g_section==i,132*s)){g_section=i;g_subsection=0;}
    ImGui::End();
    
    // ГЛАВНОЕ ОКНО
    const float mainW = w - 156*s - 176*s;
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(mainW, h), ImGuiCond_Always);
    ImGui::Begin("##main", nullptr, flags);
    uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),mainW,h));
    
    ImGui::SetCursorScreenPos(uiAdd(ImGui::GetWindowPos(), 20*s, 20*s));
    if(g_section == 0) { // Aimbot
        uiToggle("Enable Aimbot", g_appearance.box);
        uiSlider("smooth", "Smoothness", g_appearance.lineWidth, 1.0f, 10.0f, "%.1f");
    } else if(g_section == 1) { // Visuals
        uiToggle("Enable ESP", g_appearance.health);
        uiColorRow("Box Color", g_appearance.boxColor);
    } else {
        ImGui::Text("Settings here");
    }
    
    ImGui::End();
    ImGui::PopStyleVar(3);
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.Fonts->AddFontDefault();
        ImGui_ImplOpenGL3_Init("#version 300 es");
        init = true;
    }
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewport[2], (float)viewport[3]);
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    renderMinimalMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
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
        
        if (io.WantCaptureMouse) return 1; 
    }
    return orig_AInputQueue_preDispatchEvent(queue, event);
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
    }
}
