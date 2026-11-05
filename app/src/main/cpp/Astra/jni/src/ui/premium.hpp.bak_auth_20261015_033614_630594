// ASTRA_SECTIONS_FIX_V2
// ASTRA_BLUE_NIGHT_V2
// ASTRA_SCREEN_NIGHT_V1
// ASTRA_ARROWS_V6
// ASTRA_ESP_INFO_V5
// ASTRA_NOTIFICATIONS_V4
// ASTRA_POLISH_V3
// ASTRA_GUI_MOTION_V2
// ASTRA_GUI_ANIMATIONS_V1
#pragma once
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>
#include "imgui.h"
#include "features/aim/core.hpp"
#include "features/aim/assists.hpp"
#include "features/visuals/esp_runtime.hpp"

// Include at the same location as the original premium.hpp in entry.cpp.
// Existing premium_esp_extra.hpp and premium_world_hud.hpp remain required.
extern std::vector<EspScreenBox> g_espScreenBoxes;
static ImVec4 g_accent4{.96f,.97f,1.f,1.f};
static ImVec4 g_espBoxCol{.94f,.95f,.97f,1},g_espHpCol{.36f,.87f,.65f,1};
static ImVec4 g_espWepCol{.91f,.92f,.95f,1},g_espAmmoCol{.68f,.72f,.79f,1};
static float g_S=1,g_openT=0;
static bool g_open=false;
static int g_tab=0;
struct KwB {
 bool esp_enabled=false,esp_box=true,esp_hp=true,esp_armor=true;
 bool esp_wep=true,esp_ammo=true,esp_night=false;
};
static KwB g_b;
// ASTRA_ANIMATED_APPEARANCE_V1
namespace astra_esp_extra {
static bool animatedCheckbox(const char*,bool*);
namespace AnimatedImGui {
using namespace ::ImGui;
inline bool Checkbox(const char* label,bool* value){return animatedCheckbox(label,value);}
}
}
#define ImGui AnimatedImGui
#include "premium_esp_extra.hpp"
#undef ImGui
#include "premium_arrows.hpp"

namespace astra_premium {
struct Projection {float width=0,height=0,p00=0,p11=0,shiftX=0,shiftY=0;double captured=0;};
inline std::mutex projectionMutex;
inline Projection projection;
inline double monotonicSeconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline void setAimProjection(float w,float h,float p00,float p11,float sx=0,float sy=0){
 std::lock_guard<std::mutex> lock(projectionMutex);projection={w,h,p00,p11,sx,sy,monotonicSeconds()};
}
inline void clearAimProjection(){std::lock_guard<std::mutex> lock(projectionMutex);projection={};}
}


// Appearance preferences are render-thread-only and session-local.
static float g_uiScaleV3=1.f,g_motionSpeedV3=1.f;
static bool g_snowEnabledV3=true;
static float g_snowDensityV3=1.f,g_snowSpeedV3=1.f;

struct GuiNoticeV4 {char text[160]{};double began=0;};
static std::vector<GuiNoticeV4> g_noticesV4;
static void guiNotifyV3(const char* text){
 if(!text||!*text)return;
 const double now=ImGui::GetTime();
 if(!g_noticesV4.empty() && std::strcmp(g_noticesV4.back().text,text)==0 && now-g_noticesV4.back().began<.4)return;
 if(g_noticesV4.size()>=4)g_noticesV4.erase(g_noticesV4.begin());
 GuiNoticeV4 n{};std::snprintf(n.text,sizeof(n.text),"%s",text);n.began=now;g_noticesV4.push_back(n);
}

static void guiToastV3(){
 const double now=ImGui::GetTime();
 constexpr double duration=3.2;
 g_noticesV4.erase(std::remove_if(g_noticesV4.begin(),g_noticesV4.end(),[&](const GuiNoticeV4& n){return now-n.began>=duration;}),g_noticesV4.end());
 if(g_noticesV4.empty())return;
 const auto screen=ImGui::GetIO().DisplaySize;if(screen.x<60||screen.y<60)return;
 auto* font=ImGui::GetFont();
 const float scale=std::clamp(g_S,.85f,1.4f),fs=font->FontSize*scale;
 const float margin=16.f*scale;
 const float w=std::min(screen.x-2*margin,340.f*scale);
 const float h=fs+32.f*scale,gap=9.f*scale;
 float y=margin;
 auto* d=ImGui::GetForegroundDrawList();
 d->PushClipRect(ImVec2(0,0),screen,true);
 for(auto it=g_noticesV4.rbegin();it!=g_noticesV4.rend();++it){
  const float age=float(now-it->began),left=float(duration)-age;
  const float entering=std::clamp(age/.22f,0.f,1.f);
  const float ease=1.f-std::pow(1.f-entering,3.f);
  const float alpha=ease*std::clamp(left/.35f,0.f,1.f);
  if(y+h>screen.y-margin)break;
  const float x=screen.x-margin-w+(1-ease)*45.f*scale;
  const ImVec2 p(x,y),z(x+w,y+h);
  d->AddRectFilled(ImVec2(x+2,y+4),ImVec2(z.x+2,z.y+4),IM_COL32(0,0,0,int(75*alpha)),7*scale);
  d->AddRectFilled(p,z,IM_COL32(18,20,26,int(248*alpha)),7*scale);
  d->AddRect(p,z,IM_COL32(120,128,148,int(90*alpha)),7*scale);
  d->AddRectFilled(ImVec2(x+1,y+10*scale),ImVec2(x+3*scale,z.y-10*scale),IM_COL32(245,247,255,int(255*alpha)),1);
  d->PushClipRect(ImVec2(x+14*scale,y),ImVec2(z.x-12*scale,z.y),true);
  d->AddText(font,fs,ImVec2(x+17*scale,y+13*scale),IM_COL32(238,241,249,int(255*alpha)),it->text);
  d->PopClipRect();
  const float progress=std::clamp(left/float(duration),0.f,1.f);
  d->AddLine(ImVec2(x+10*scale,z.y-3*scale),ImVec2(x+10*scale+(w-20*scale)*progress,z.y-3*scale),IM_COL32(215,222,240,int(150*alpha)),2*scale);
  y+=(h+gap)*ease;
 }
 d->PopClipRect();
}

namespace mixui_final {




namespace aim=astra::aim;
static float width=0;
static int dim=155;
static bool previewEnabled=true;
static aim::Configuration edit;
static bool dirty=false,resetRequested=false;
static const char* tabs[]={"Aim","Anti-aim","Visuals","Misc","Skins","Players","Config"};
static const char* titles[7][2]={
 {"Angle aimbot","Bone selection"},{"Angles","Movement"},
 {"Player ESP","Preview"},{"Movement","Other"},{"Models","Weapons"},
 {"Players","Adjustments"},{"Interface","Presets"}
};
static float u(float x){return x*g_S;}
static float clamp(float x,float a,float b){return std::isfinite(x)?std::max(a,std::min(x,b)):a;}
static ImVec4 gray(float x,float a=1){return ImVec4(x,x,x,a);}
static ImU32 color(ImVec4 c){return ImGui::GetColorU32(c);}
static float row(){return std::max(u(38),ImGui::GetFontSize()+u(16));}
static void gap(float h=7){ImGui::Dummy(ImVec2(0,u(h)));}
static void theme(){
 auto& s=ImGui::GetStyle();
 s.WindowPadding=ImVec2(u(16),u(16));s.FramePadding=ImVec2(u(11),u(9));
 s.ItemSpacing=ImVec2(u(10),u(8));s.ItemInnerSpacing=ImVec2(u(8),u(5));
 s.WindowRounding=u(9);s.ChildRounding=u(6);s.FrameRounding=u(4);s.PopupRounding=u(7);
 s.WindowBorderSize=1;s.ChildBorderSize=1;s.FrameBorderSize=1;s.PopupBorderSize=1;
 s.ScrollbarSize=std::max(26.f,u(27));s.ScrollbarRounding=u(6);
 s.GrabMinSize=std::max(34.f,u(38));s.GrabRounding=u(3);
 auto* c=s.Colors;
 c[ImGuiCol_Text]=ImVec4(.90f,.91f,.94f,1);c[ImGuiCol_TextDisabled]=ImVec4(.44f,.46f,.51f,1);
 c[ImGuiCol_WindowBg]=ImVec4(.035f,.038f,.045f,1);c[ImGuiCol_ChildBg]=gray(0,0);
 c[ImGuiCol_PopupBg]=ImVec4(.062f,.068f,.080f,1);
 c[ImGuiCol_Border]=ImVec4(.19f,.20f,.23f,.8f);c[ImGuiCol_BorderShadow]=gray(0,0);
 c[ImGuiCol_FrameBg]=ImVec4(.09f,.098f,.115f,1);
 c[ImGuiCol_FrameBgHovered]=ImVec4(.13f,.14f,.16f,1);c[ImGuiCol_FrameBgActive]=ImVec4(.17f,.18f,.21f,1);
 c[ImGuiCol_Button]=ImVec4(.10f,.11f,.13f,1);c[ImGuiCol_ButtonHovered]=ImVec4(.17f,.18f,.21f,1);c[ImGuiCol_ButtonActive]=ImVec4(.23f,.24f,.27f,1);
 c[ImGuiCol_Header]=ImVec4(.15f,.16f,.19f,1);c[ImGuiCol_HeaderHovered]=ImVec4(.20f,.21f,.24f,1);c[ImGuiCol_HeaderActive]=ImVec4(.25f,.26f,.29f,1);
 c[ImGuiCol_CheckMark]=g_accent4;c[ImGuiCol_SliderGrab]=g_accent4;c[ImGuiCol_SliderGrabActive]=g_accent4;
 c[ImGuiCol_Separator]=ImVec4(.18f,.19f,.22f,.8f);
 c[ImGuiCol_ScrollbarBg]=ImVec4(.025f,.028f,.035f,.8f);c[ImGuiCol_ScrollbarGrab]=ImVec4(.22f,.24f,.28f,1);
 c[ImGuiCol_ScrollbarGrabHovered]=ImVec4(.36f,.38f,.43f,1);c[ImGuiCol_ScrollbarGrabActive]=g_accent4;
 c[ImGuiCol_TextSelectedBg]=ImVec4(g_accent4.x,g_accent4.y,g_accent4.z,.18f);
}

// Unlike ImGui::Separator, this respects the current card width.
static void cardSeparatorV4(){
 const auto p=ImGui::GetCursorScreenPos();
 const float w=std::max(1.f,width);
 ImGui::GetWindowDrawList()->AddLine(p,ImVec2(p.x+w,p.y),ImGui::GetColorU32(ImGuiCol_Separator),1.f);
 ImGui::Dummy(ImVec2(w,1.f));
}
static void section(const char* label){
 gap(10);cardSeparatorV4();gap(2);
 ImGui::TextColored(ImVec4(.61f,.64f,.70f,1),"%s",label);
}
static void note(const char* label){
 ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.45f,.48f,.54f,1));
 ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+width);ImGui::TextWrapped("%s",label);
 ImGui::PopTextWrapPos();ImGui::PopStyleColor();
}
static bool check(const char* label,bool& value,float w=0){
 if(w<=0)w=width;w=std::max(1.f,w);
 ImGui::PushID(label);
 auto* storage=ImGui::GetStateStorage();
 const auto p=ImGui::GetCursorScreenPos();
 // Logical 27-unit box, at least 23 actual pixels on small screens.
 const float box=std::max(23.f,u(27));
 const float h=std::max(row(),box+u(14));
 const float y=p.y+(h-box)*.5f;
 const bool before=value;
 const bool changed=ImGui::InvisibleButton("##check",ImVec2(w,h));
 if(changed){
 value=!value;
 char message[160];std::snprintf(message,sizeof(message),"%s: %s",label,value?"ON":"OFF");
 guiNotifyV3(message);
 }
 const bool hovered=ImGui::IsItemHovered();
 const bool held=ImGui::IsItemActive();
 const float now=float(ImGui::GetTime());
 const float dt=std::clamp(ImGui::GetIO().DeltaTime,0.f,.05f);
 const auto tid=ImGui::GetID("##motion-fill"),hid=ImGui::GetID("##motion-hover");
 const auto pid=ImGui::GetID("##motion-press"),rid=ImGui::GetID("##motion-ripple");
 const auto timeid=ImGui::GetID("##motion-time");
 const bool stale=now-storage->GetFloat(timeid,now)>.20f;
 float t=storage->GetFloat(tid,before?1.f:0.f);
 float hv=storage->GetFloat(hid,0),press=storage->GetFloat(pid,0);
 float ripple=storage->GetFloat(rid,1.f);
 if(stale){t=before?1.f:0.f;hv=press=0;ripple=1;}
 t+=(float(value)-t)*(1-std::exp(-15.f*g_motionSpeedV3*dt));
 hv+=(float(hovered)-hv)*(1-std::exp(-13.f*g_motionSpeedV3*dt));
 press+=(float(held)-press)*(1-std::exp(-23.f*g_motionSpeedV3*dt));
 ripple=changed?0.f:std::min(1.f,ripple+dt*2.9f*g_motionSpeedV3);
 if(std::fabs(t-float(value))<.001f)t=float(value);
 storage->SetFloat(tid,t);storage->SetFloat(hid,hv);storage->SetFloat(pid,press);
 storage->SetFloat(rid,ripple);storage->SetFloat(timeid,now);
 auto* d=ImGui::GetWindowDrawList();
 d->PushClipRect(p,ImVec2(p.x+w,p.y+h),true);
 d->AddRectFilled(p,ImVec2(p.x+w,p.y+h),color(gray(1,.032f*hv)),u(5));
 const float bounce=std::sin(ripple*3.14159265f)*std::exp(-ripple*3.f);
 const float inset=box*(.065f*press-.045f*bounce);
 const ImVec2 a(p.x+inset,y+inset),b(p.x+box-inset,y+box-inset);
 ImVec4 fill(.08f,.09f,.11f,1);
 fill.x+=(g_accent4.x-fill.x)*t;fill.y+=(g_accent4.y-fill.y)*t;fill.z+=(g_accent4.z-fill.z)*t;
 ImVec4 glow=g_accent4;glow.w=.07f*t+.055f*hv;
 d->AddRectFilled(ImVec2(a.x-2,a.y-2),ImVec2(b.x+2,b.y+2),color(glow),u(6));
 d->AddRectFilled(a,b,color(fill),u(4));
 ImVec4 edge=gray(.29f+.16f*hv);edge.x+=(g_accent4.x-edge.x)*t;edge.y+=(g_accent4.y-edge.y)*t;edge.z+=(g_accent4.z-edge.z)*t;
 d->AddRect(a,b,color(edge),u(4));
 // Normalized geometry: tick scales correctly with the entire checkbox.
 const float side=b.x-a.x;
 ImVec2 q0(a.x+side*.21f,a.y+side*.51f),q1(a.x+side*.42f,a.y+side*.70f),q2(a.x+side*.80f,a.y+side*.29f);
 const float reveal=std::clamp((t-.08f)/.84f,0.f,1.f);
 auto lerp=[](ImVec2 x,ImVec2 z,float k){return ImVec2(x.x+(z.x-x.x)*k,x.y+(z.y-x.y)*k);};
 const auto ink=color(gray(.045f,std::min(1.f,t*2)));
 const float thick=std::max(1.8f,side*.085f);
 if(reveal>0)d->AddLine(q0,lerp(q0,q1,std::min(1.f,reveal/.34f)),ink,thick);
 if(reveal>.34f)d->AddLine(q1,lerp(q1,q2,(reveal-.34f)/.66f),ink,thick);
 if(ripple<1){
  ImVec4 ring=g_accent4;ring.w=(1-ripple)*.28f;
  d->AddCircle(ImVec2(p.x+box*.5f,y+box*.5f),box*(.53f+.25f*ripple),color(ring),24,std::max(1.f,u(1)));
 }
 d->AddText(ImVec2(p.x+box+u(12)+u(1.5f)*hv,p.y+(h-ImGui::GetFontSize())*.5f),color(gray(.72f+.22f*t+.035f*hv)),label);
 d->PopClipRect();ImGui::PopID();return changed;
}
static bool slider(const char* label,float& value,float min,float max,const char* fmt){
 ImGui::PushID(label);
 const auto p=ImGui::GetCursorScreenPos();
 char text[64];std::snprintf(text,sizeof(text),fmt,value);
 const auto ext=ImGui::CalcTextSize(text);
 const float bh=ImGui::GetFontSize()+u(8),badge=std::min(width,ext.x+u(18));
 auto* d=ImGui::GetWindowDrawList();
 d->PushClipRect(p,ImVec2(p.x+std::max(1.f,width-badge-u(8)),p.y+bh),true);
 d->AddText(ImVec2(p.x,p.y+u(4)),color(gray(.85f)),label);d->PopClipRect();
 const ImVec2 a(p.x+width-badge,p.y),b(p.x+width,p.y+bh);
 d->AddRectFilled(a,b,color(gray(1,.045f)),u(4));
 d->AddRect(a,b,color(gray(1,.07f)),u(4));
 d->PushClipRect(a,b,true);d->AddText(ImVec2(a.x+u(9),a.y+u(4)),color(g_accent4),text);d->PopClipRect();
 ImGui::Dummy(ImVec2(width,bh));
 // Full-width, large native hit area with keyboard/navigation preserved.
 const float height=std::max(42.f,u(44));
 ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(u(12),std::max(1.f,(height-ImGui::GetFontSize())*.5f)));
 ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,std::max(32.f,u(35)));
 ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,u(6));
 ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(.09f,.10f,.12f,1));
 ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,ImVec4(.13f,.14f,.17f,1));
 ImGui::PushStyleColor(ImGuiCol_SliderGrab,g_accent4);
 ImGui::SetNextItemWidth(width);
 const bool changed=ImGui::SliderFloat("##value",&value,min,max,fmt);
 if(changed)value=clamp(value,min,max);
 if(ImGui::IsItemDeactivatedAfterEdit()){
  char number[64],message[160];std::snprintf(number,sizeof(number),fmt,value);
  std::snprintf(message,sizeof(message),"%s: %s",label,number);guiNotifyV3(message);
 }

 ImGui::PopStyleColor(3);ImGui::PopStyleVar(3);ImGui::PopID();return changed;
}
static void colorCheck(const char* label,bool& value,ImVec4& c){
 ImGui::PushID(label);
 const float side=std::max(row(),std::max(23.f,u(27))+u(14));
 const float buttonW=std::max(36.f,u(38));
 check(label,value,std::max(1.f,width-buttonW-u(8)));ImGui::SameLine(0,u(8));
 const auto p=ImGui::GetCursorScreenPos();
 if(ImGui::InvisibleButton("##color",ImVec2(buttonW,side)))ImGui::OpenPopup("##picker");
 auto* d=ImGui::GetWindowDrawList();const bool hover=ImGui::IsItemHovered();
 const float size=std::max(18.f,u(21));
 const ImVec2 a(p.x+(buttonW-size)*.5f,p.y+(side-size)*.5f),b(a.x+size,a.y+size);
 d->AddRectFilled(ImVec2(p.x,p.y+u(3)),ImVec2(p.x+buttonW,p.y+side-u(3)),color(gray(1,hover?.07f:.025f)),u(5));
 d->AddRectFilled(a,b,color(c),u(4));d->AddRect(a,b,color(gray(1,.3f)),u(4));
 if(ImGui::BeginPopup("##picker")){
  ImGui::TextUnformatted(label);ImGui::Separator();
  const float pw=std::max(100.f,std::min(u(255),ImGui::GetIO().DisplaySize.x-40));
  ImGui::SetNextItemWidth(pw);
  ImGui::ColorPicker4("##rgb",&c.x,ImGuiColorEditFlags_NoAlpha|ImGuiColorEditFlags_NoSidePreview);
  const ImVec4 swatches[]={ImVec4(1,1,1,1),ImVec4(.68f,.77f,1,1),ImVec4(.38f,.88f,.65f,1),ImVec4(1,.48f,.48f,1),ImVec4(1,.75f,.35f,1)};
  const float sw=std::max(12.f,(pw-u(24))/5);
  for(int i=0;i<5;++i){ImGui::PushID(i);if(i)ImGui::SameLine(0,u(6));if(ImGui::ColorButton("##swatch",swatches[i],ImGuiColorEditFlags_NoTooltip,ImVec2(sw,std::max(30.f,u(32)))))c=swatches[i];ImGui::PopID();}
  ImGui::EndPopup();
 }ImGui::PopID();
}
static void stub(const char* label){
 const auto p=ImGui::GetCursorScreenPos();const float h=row();
 ImGui::Dummy(ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
 d->PushClipRect(p,ImVec2(p.x+width,p.y+h),true);
 const char* badge="soon";auto ext=ImGui::CalcTextSize(badge);float bw=ext.x+u(16);
 const float right=p.x+width,bx=right-bw,bh=ext.y+u(6),by=p.y+(h-bh)*.5f;
 d->PushClipRect(p,ImVec2(std::max(p.x,bx-u(8)),p.y+h),true);
 d->AddText(ImVec2(p.x,p.y+(h-ImGui::GetFontSize())*.5f),color(gray(.57f)),label);d->PopClipRect();
 d->AddRectFilled(ImVec2(bx,by),ImVec2(right,by+bh),color(gray(1,.045f)),u(3));
 d->AddRect(ImVec2(bx,by),ImVec2(right,by+bh),color(gray(1,.08f)),u(3));
 d->AddText(ImVec2(bx+u(8),by+u(3)),color(gray(.46f)),badge);d->PopClipRect();
}
static void updateAimProjection(){
 if(!edit.normal.showFov){astra_premium::clearAimProjection();return;}
 astra::esp::CameraMatrixCache m{};std::uintptr_t camera=0;
 if(!astra::esp::detail::readCameraForPresent(m,camera)){astra_premium::clearAimProjection();return;}
 const auto& p=m.projection.value;float scale=-p[11];
 if(!std::isfinite(scale)||scale<=.0001f){astra_premium::clearAimProjection();return;}
 for(int index:{1,3,4,7,12,13,15})if(std::fabs(p[index]/scale)>.001f){astra_premium::clearAimProjection();return;}
 auto size=ImGui::GetIO().DisplaySize;float p00=std::fabs(p[0]/scale),p11=std::fabs(p[5]/scale),sx=p[8]/scale,sy=p[9]/scale;
 auto g=aim::assists::fovGeometry(edit.normal.fov,size.x,size.y,p00,p11,sx,sy);
 if(!g.valid){astra_premium::clearAimProjection();return;}
 astra_premium::setAimProjection(size.x,size.y,p00,p11,sx,sy);
}
static aim::assists::FovGeometry currentFovGeometry(float degrees){
 astra_premium::Projection p;{std::lock_guard<std::mutex> lock(astra_premium::projectionMutex);p=astra_premium::projection;}
 double age=astra_premium::monotonicSeconds()-p.captured;auto display=ImGui::GetIO().DisplaySize;
 if(age<0||age>=.25||std::fabs(p.width-display.x)>1||std::fabs(p.height-display.y)>1)return {};
 return aim::assists::fovGeometry(degrees,p.width,p.height,p.p00,p.p11,p.shiftX,p.shiftY);
}
static bool projectionReady(){return currentFovGeometry(edit.normal.fov).valid;}
static void renderFov(const aim::Settings& settings){
 if(!settings.showFov)return;auto g=currentFovGeometry(settings.fov);if(!g.valid||g.coversScreen)return;
 auto* d=ImGui::GetBackgroundDrawList();d->PushClipRect(ImVec2(0,0),ImGui::GetIO().DisplaySize,true);d->PathClear();
 for(int i=0;i<=256;++i){float a=float(i)*6.28318530718f/256;d->PathLineTo(ImVec2(g.centerX+std::cos(a)*g.radiusX,g.centerY+std::sin(a)*g.radiusY));}
 d->PathStroke(ImGui::ColorConvertFloat4ToU32(g_accent4),0,std::max(1.f,g_S));d->PopClipRect();
}
static void preview(){
 const auto p=ImGui::GetCursorScreenPos();const float h=u(280),cx=p.x+width*.5f;
 ImGui::Dummy(ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
 d->PushClipRect(p,ImVec2(p.x+width,p.y+h),true);
 d->AddRectFilled(p,ImVec2(p.x+width,p.y+h),color(ImVec4(.035f,.04f,.05f,1)),u(5));
 for(float x=p.x+u(20);x<p.x+width;x+=u(24))d->AddLine(ImVec2(x,p.y),ImVec2(x,p.y+h),color(gray(1,.02f)));
 for(float y=p.y+u(20);y<p.y+h;y+=u(24))d->AddLine(ImVec2(p.x,y),ImVec2(p.x+width,y),color(gray(1,.02f)));
 const float top=p.y+u(25),bottom=p.y+h-u(75),bw=std::min(width*.42f,u(80));
 const float bodyHeight=bottom-top;
 auto body=color(ImVec4(.24f,.26f,.30f,1));
 d->AddCircleFilled(ImVec2(cx,top+bodyHeight*.09f),bodyHeight*.065f,body,24);
 d->AddRectFilled(ImVec2(cx-bw*.17f,top+bodyHeight*.18f),ImVec2(cx+bw*.17f,top+bodyHeight*.59f),body,u(5));
 for(int side:{-1,1}){
  d->AddLine(ImVec2(cx+side*bw*.22f,top+bodyHeight*.22f),ImVec2(cx+side*bw*.33f,top+bodyHeight*.62f),body,u(8));
  d->AddLine(ImVec2(cx+side*bw*.10f,top+bodyHeight*.56f),ImVec2(cx+side*bw*.20f,bottom-u(4)),body,u(9));
 }
 if(g_b.esp_enabled){
  EspScreenBox b{};b.left=cx-bw*.5f;b.right=cx+bw*.5f;b.top=top;b.bottom=bottom;
  b.enemy=true;b.statsValid=true;b.weaponValid=true;b.hp=74;b.maxHp=100;b.armor=60;b.maxArmor=100;
  // Preview examples only; live values are never fabricated.
  if(astra_esp_extra::settings.lowHealth)b.hp=20;
  b.weaponId=7;b.magazineAmmo=astra_esp_extra::settings.emptyMagazine?0:24;b.reserveAmmo=90;
  b.distanceUnits=24.f;b.distanceValid=true;
  astra_esp_extra::draw(d,b);
 }
 d->AddRect(p,ImVec2(p.x+width,p.y+h),color(gray(1,.07f)),u(5));d->PopClipRect();
}
static void reset(){
 astra_arrows::reset();
 g_uiScaleV3=1;g_motionSpeedV3=1;g_snowEnabledV3=true;g_snowDensityV3=1;g_snowSpeedV3=1;
 astra_esp_extra::reset();astra_night::publish(astra_night::Settings{});
 edit=aim::Configuration{};dirty=true;resetRequested=true;g_b=KwB{};dim=155;previewEnabled=true;
 g_accent4=ImVec4(.96f,.97f,1,1);g_espBoxCol=ImVec4(.94f,.95f,.97f,1);g_espHpCol=ImVec4(.36f,.87f,.65f,1);g_espWepCol=ImVec4(.91f,.92f,.95f,1);g_espAmmoCol=ImVec4(.68f,.72f,.79f,1);
}
#include "premium_world_hud.hpp"
static void leftContent(){
 // Chams GUI parked: implementation and runtime retained.
 switch(g_tab){
 case 0:
  dirty|=check("Aimbot",edit.normal.enabled);dirty|=check("Visible check",edit.normal.visibleCheck);
  dirty|=slider("Fov",edit.normal.fov,.1f,180.f,"%.1f deg");dirty|=check("Show fov",edit.normal.showFov);
  dirty|=slider("Smoothen",edit.normal.smoothing,0.f,1.f,"%.3f s");note("0 = instant; higher = smoother.");
  section("Recoil control");dirty|=check("RCS",edit.normal.rcs);dirty|=slider("RCS strength",edit.normal.rcsStrength,0.f,100.f,"%.0f%%");break;
 case 1:stub("Anti-aim");break;
 case 2: {
 // ASTRA_COMPACT_VISUALS_V1
 check("Enabled",g_b.esp_enabled);
 colorCheck("Bounding box",g_b.esp_box,g_espBoxCol);
 ImGui::TextUnformatted("Frame style");
 ImGui::SetNextItemWidth(width);
 ImGui::Combo("##compact-frame-style",&astra_esp_extra::frameStyle,"Classic\0Corners\0Rounded\0Dashed\0Side brackets\0");
 auto& a=astra_esp_extra::settings;
 check("Snaplines",a.snaplines);
 check("Fill",a.fill);
 colorCheck("Arrows",astra_arrows::settings.enabled,astra_arrows::settings.color);
 if(astra_sections::header("Advanced ESP",width)) {
  ImGui::PushID("compact-advanced");
section("Players");
  colorCheck("Bounding box",g_b.esp_box,g_espBoxCol);colorCheck("Health bar",g_b.esp_hp,g_espHpCol);
  colorCheck("Armor bar",g_b.esp_armor,astra_esp_extra::settings.armorColor);
  colorCheck("Weapon info",g_b.esp_wep,g_espWepCol);colorCheck("Ammo",g_b.esp_ammo,g_espAmmoCol);

  section("Information visuals");
  {auto& info=astra_esp_extra::settings;
   check("Distance",info.distance);
   check("HP number",info.hpNumber);
   check("Armor number",info.armorNumber);
   check("Low health marker",info.lowHealth);
   check("Empty magazine marker",info.emptyMagazine);
   slider("Low HP threshold",info.lowHealthPercent,5,50,"%.0f%%");
   slider("Meters per world unit",info.metersPerUnit,.01f,10.f,"%.2f");
   note("Distance requires fresh local and enemy positions. Default: 1 world unit = 1 m.");
  }



  section("Extra appearance");
  check("Box head marker",a.headMarker);
  check("Text shadow",a.shadow);
  check("Text outline",a.outline);
  check("Arrow halo",astra_arrows::settings.glow);
  check("Soft frame halo",astra_esp_extra::softHalo);
  check("HP / armor divisions",astra_esp_extra::barDivisions);
  slider("Halo opacity",astra_esp_extra::haloOpacity,0,.25f,"%.2f");
  ImGui::PopID();
 }
 if(astra_sections::header("Sizes and opacity",width)) {
  slider("Line width",a.thickness,1,4,"%.1f");
  slider("Fill opacity",a.fillAlpha,0,.35f,"%.2f");
  slider("ESP text scale",a.textScale,.5f,2,"%.2f");
  slider("Arrow size",astra_arrows::settings.size,7,24,"%.0f px");
  slider("Arrow inset",astra_arrows::settings.inset,30,180,"%.0f px");
 }
 if(astra_sections::header("Hitlog / Hitsound",width)) {
  ImGui::PushID("compact-hitlog");
// ASTRA_HITLOG_VISUALS_V1
section("Hitlog");
{
 bool enabled=astra_hitlog::enabled.load();
 if(check("Hitlog",enabled)){
  astra_hitlog::enabled.store(enabled);
  if(!enabled){
   std::lock_guard<std::mutex> lock(astra_hitlog::mutex);
   astra_hitlog::lines.clear();
   astra_hitlog::localWorld=nullptr;
  }
 }
 const int state=astra_hitlog::state.load();
 note(state==2?"Hook installed":
      state==-1?"Code mismatch":
      state==-2?"Hook installation failed":
      state==1?"Installing...":"Waiting for enabled / local player");
 ImGui::Text("Events: %llu | Local hits: %llu",
             astra_hitlog::received.load(),
             astra_hitlog::accepted.load());
 astra_hitlog::deathDiagnostics();
}

  ImGui::PopID();
 }
 break;
 }
 case 3:stub("Auto Strafe");stub("Bunny Hop");stub("Fast Run");stub("Long Jump");stub("Fly Mode");break;
 case 4:stub("Player model");stub("Arms model");stub("Gloves");break;
 case 5:stub("Player list");stub("Search players");stub("Team filter");break;
 case 6:
  ImGui::TextUnformatted("Accent");ImGui::ColorEdit3("##accent",&g_accent4.x,ImGuiColorEditFlags_NoInputs);
  {ImVec4 presets[]={ImVec4(.96f,.97f,1,1),gray(.70f),ImVec4(.63f,.79f,.39f,1),ImVec4(.49f,.67f,.88f,1),ImVec4(.72f,.59f,.86f,1)};
   float side=std::max(1.f,std::min(u(38),(width-u(24))/5));
   for(int i=0;i<5;++i){ImGui::PushID(i);if(i)ImGui::SameLine(0,u(6));if(ImGui::ColorButton("##preset",presets[i],0,ImVec2(side,side)))g_accent4=presets[i];ImGui::PopID();}}
  ImGui::TextUnformatted("Background dimming");ImGui::SetNextItemWidth(width);ImGui::SliderInt("##dim",&dim,0,220);dim=std::max(0,std::min(dim,220));
  check("Show ESP preview",previewEnabled);
  section("Appearance");
  slider("Interface scale",g_uiScaleV3,.8f,1.25f,"%.2fx");
  slider("Animation speed",g_motionSpeedV3,.5f,2.f,"%.2fx");
  check("Snow",g_snowEnabledV3);
  slider("Snow density",g_snowDensityV3,.3f,2.f,"%.2fx");
  slider("Snow speed",g_snowSpeedV3,.3f,2.f,"%.2fx");
  if(ImGui::Button("Test notification",ImVec2(width,row())))guiNotifyV3("Interface updated");
  note("Appearance settings are kept for this session.");

  section("Defaults");
  if(ImGui::Button("Reset settings",ImVec2(width,row())))ImGui::OpenPopup("Reset settings?");
  if(ImGui::BeginPopupModal("Reset settings?",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
   ImGui::TextUnformatted("Restore controls and colors?");
   if(ImGui::Button("Reset",ImVec2(u(100),row()))){reset();guiNotifyV3("Settings reset");ImGui::CloseCurrentPopup();}
   ImGui::SameLine();if(ImGui::Button("Cancel",ImVec2(u(100),row())))ImGui::CloseCurrentPopup();ImGui::EndPopup();
  }break;
 }
}
static void rightContent(){
 switch(g_tab){
 case 0:
  for(unsigned i=0;i<unsigned(aim::Body::Count);++i){bool value=(edit.normal.bodies&(1u<<i))!=0;if(check(aim::kBodyNames[i],value)){if(value)edit.normal.bodies|=1u<<i;else edit.normal.bodies&=~(1u<<i);dirty=true;}}
  if(!edit.normal.bodies)note("Select at least one body region.");section("Status");
  if(!edit.normal.enabled)note("Disabled");else if(edit.pauseInMenu&&g_open)note("Paused while menu is open");else if(!aim::g_normalHookReady.load())note("Waiting for angle runtime hook");else note(aim::stateText(aim::g_normalState.load()));
  if(edit.normal.visibleCheck&&!aim::g_visibilityReady.load())note("Visibility profile not ready; targeting is blocked.");
  if(edit.normal.rcs&&!aim::g_rcsReady.load())note("RCS profile not ready");
  // ASTRA_RCS_DIAGNOSTICS_V1
  {
   namespace dbg=astra::aim::debug;
   ImGui::Text("RCS: enabled=%d ready=%d strength=%.0f",
       int(edit.normal.rcs),int(aim::g_rcsReady.load()),
       double(edit.normal.rcsStrength));
   ImGui::Text("Reads: %llu | Rejected: %llu",
       (unsigned long long)dbg::rcsReads.load(),
       (unsigned long long)dbg::rcsRejected.load());
   ImGui::Text("Applied: %llu",
       (unsigned long long)dbg::rcsApplied.load());
   ImGui::Text("Last recoil: pitch=%.4f yaw=%.4f",
       double(dbg::recoilPitch.load()),double(dbg::recoilYaw.load()));
   ImGui::Text("Attack raw: %d",dbg::attackRaw.load());
  }

  if(edit.normal.showFov){auto g=currentFovGeometry(edit.normal.fov);if(!g.valid)note("FOV overlay: waiting for fresh camera projection");else if(g.coversScreen)note("FOV covers the screen; outline is outside the view.");}break;
 case 1:stub("Movement");break;
 case 2:if(previewEnabled)preview();worldHudControls();break;
 case 3:stub("No Spread");stub("Custom keybinds");break;
 case 4:stub("Weapon skin");stub("Knife selection");stub("Wear");break;
 case 5:stub("Player overrides");stub("Priority target");stub("Ignore player");break;
 case 6:stub("Save preset");stub("Load preset");stub("Import / export");note("Settings are kept for this session.");break;
 }
}
static float panel(int index,ImVec2 p,float w){
 ImGui::PushID(index);ImGui::SetCursorScreenPos(ImVec2(p.x+u(16),p.y+u(15)));ImGui::BeginGroup();
 width=std::max(1.f,w-u(32));ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+width);
 ImGui::TextColored(ImVec4(.96f,.97f,1,1),"%s",titles[g_tab][index]);gap(7);cardSeparatorV4();gap(5);
 ImGui::PushItemWidth(width);if(index==0)leftContent();else rightContent();ImGui::PopItemWidth();
 ImGui::PopTextWrapPos();ImGui::EndGroup();float bottom=std::max(p.y+u(110),ImGui::GetItemRectMax().y+u(16));
 auto* d=ImGui::GetWindowDrawList();d->ChannelsSetCurrent(0);
 d->AddRectFilled(ImVec2(p.x,p.y+u(3)),ImVec2(p.x+w,bottom+u(3)),color(gray(0,.22f)),u(6));
 d->AddRectFilled(p,ImVec2(p.x+w,bottom),color(ImVec4(.060f,.066f,.079f,1)),u(6));
 d->AddRect(p,ImVec2(p.x+w,bottom),color(ImVec4(.16f,.18f,.21f,.9f)),u(6));
 d->AddLine(ImVec2(p.x+u(12),p.y+1),ImVec2(p.x+w-u(12),p.y+1),color(gray(1,.10f)));
 d->ChannelsSetCurrent(1);ImGui::SetCursorScreenPos(p);ImGui::Dummy(ImVec2(w,bottom-p.y));ImGui::PopID();return bottom;
}
static void panelsV3Content(){
 float available=ImGui::GetContentRegionAvail().x;if(available<=u(35))return;
 auto p=ImGui::GetCursorScreenPos();float bottom;auto* d=ImGui::GetWindowDrawList();d->ChannelsSplit(2);d->ChannelsSetCurrent(1);
 if(available>=u(590)){float w=(available-u(14))*.5f;float a=panel(0,p,w),b=panel(1,ImVec2(p.x+w+u(14),p.y),w);bottom=std::max(a,b);}
 else{float a=panel(0,p,available);bottom=panel(1,ImVec2(p.x,a+u(14)),available);}
 d->ChannelsMerge();ImGui::SetCursorScreenPos(ImVec2(p.x,bottom));ImGui::Dummy(ImVec2(available,u(8)));
}


static void panels(){
 static int previous=-1;static bool wasOpen=false;static float progress=1;static int lastFrame=-2;
 const int frame=ImGui::GetFrameCount();
 if(previous!=g_tab||(!wasOpen&&g_open)||frame>lastFrame+1){progress=0;previous=g_tab;}
 wasOpen=g_open;lastFrame=frame;
 progress=std::min(1.f,progress+std::clamp(ImGui::GetIO().DeltaTime,0.f,.05f)*5.f*g_motionSpeedV3);
 const float ease=progress*progress*(3-2*progress);
 const float alpha=ImGui::GetStyle().Alpha;
 ImGui::PushStyleVar(ImGuiStyleVar_Alpha,alpha*(.22f+.78f*ease));
 panelsV3Content();ImGui::PopStyleVar();
}

static void tabIconV3(ImDrawList* d,int kind,ImVec2 c,float size,ImU32 ink){
 const float r=size*.40f,t=std::max(1.f,size*.08f);
 auto line=[&](float x,float y,float a,float b){d->AddLine(ImVec2(c.x+x*r,c.y+y*r),ImVec2(c.x+a*r,c.y+b*r),ink,t);};
 if(kind==0){d->AddCircle(c,r*.7f,ink,20,t);line(-1,0,1,0);line(0,-1,0,1);}
 else if(kind==1){line(-.9f,-.55f,.8f,-.55f);line(.8f,-.55f,.4f,-.95f);line(.8f,-.55f,.4f,-.15f);line(.9f,.55f,-.8f,.55f);line(-.8f,.55f,-.4f,.15f);}
 else if(kind==2){line(-1,0,0,-.65f);line(0,-.65f,1,0);line(1,0,0,.65f);line(0,.65f,-1,0);d->AddCircle(c,r*.25f,ink,12,t);}
 else if(kind==3){line(.2f,-1,-.65f,.15f);line(-.65f,.15f,.25f,.15f);line(.25f,.15f,-.2f,1);line(-.2f,1,.65f,-.15f);}
 else if(kind==4){line(-1,-.55f,-.45f,-1);line(-.45f,-1,0,-.65f);line(0,-.65f,.45f,-1);line(.45f,-1,1,-.55f);line(-1,-.55f,-.55f,-.1f);line(1,-.55f,.55f,-.1f);line(-.55f,-.1f,-.55f,1);line(.55f,-.1f,.55f,1);line(-.55f,1,.55f,1);}
 else if(kind==5){d->AddCircle(ImVec2(c.x,c.y-r*.45f),r*.38f,ink,16,t);line(-.75f,1,-.65f,.45f);line(-.65f,.45f,0,.2f);line(0,.2f,.65f,.45f);line(.65f,.45f,.75f,1);}
 else{for(int i=0;i<3;++i){float y=float(i-1)*.7f;line(-1,y,1,y);float x=i==1?.4f:-.35f;d->AddCircleFilled(ImVec2(c.x+x*r,c.y+y*r),t*1.8f,ink,10);}}
}

static void navigation(){
 const auto start=ImGui::GetCursorScreenPos();
 const float w=ImGui::GetContentRegionAvail().x,available=ImGui::GetContentRegionAvail().y;
 const float step=std::max(u(18),std::min(u(49),available/7.f)),h=std::max(1.f,step-u(5));
 const float dt=std::clamp(ImGui::GetIO().DeltaTime,0.f,.05f),k=1-std::exp(-15.f*g_motionSpeedV3*dt);
 auto* store=ImGui::GetStateStorage();auto* d=ImGui::GetWindowDrawList();
 const auto posid=ImGui::GetID("##v2-slide"),timeid=ImGui::GetID("##v2-time");
 const float now=float(ImGui::GetTime());
 const bool stale=now-store->GetFloat(timeid,now)>.2f;
 float slide=store->GetFloat(posid,float(g_tab));if(stale)slide=float(g_tab);
 d->ChannelsSplit(2);d->ChannelsSetCurrent(1);
 for(int i=0;i<7;++i){
  ImGui::PushID(i);ImGui::SetCursorScreenPos(ImVec2(start.x,start.y+step*i));auto p=ImGui::GetCursorScreenPos();
  if(ImGui::InvisibleButton("##tab",ImVec2(w,h)))g_tab=i;
  const bool selected=g_tab==i,hover=ImGui::IsItemHovered();
  auto sid=ImGui::GetID("##v2-select"),hid=ImGui::GetID("##v2-hover");
  float s=store->GetFloat(sid,selected?1.f:0.f),hv=store->GetFloat(hid,0);
  if(stale){s=selected?1.f:0.f;hv=0;}
  s+=(float(selected)-s)*k;hv+=(float(hover)-hv)*k;
  store->SetFloat(sid,s);store->SetFloat(hid,hv);
  d->AddRectFilled(p,ImVec2(p.x+w,p.y+h),color(gray(1,.035f*hv)),u(5));
  ImVec4 ink(.51f,.53f,.57f,1);
  ink.x+=(g_accent4.x-ink.x)*s;ink.y+=(g_accent4.y-ink.y)*s;ink.z+=(g_accent4.z-ink.z)*s;
  ink.x+=.13f*hv*(1-s);ink.y+=.13f*hv*(1-s);ink.z+=.13f*hv*(1-s);
  d->PushClipRect(p,ImVec2(p.x+w,p.y+h),true);
  tabIconV3(d,i,ImVec2(p.x+u(19),p.y+h*.5f),u(18),color(ink));
  d->AddText(ImVec2(p.x+u(37)+u(3)*s+u(1)*hv,p.y+(h-ImGui::GetFontSize())*.5f),color(ink),tabs[i]);
  d->PopClipRect();ImGui::PopID();
 }
 slide+=(float(g_tab)-slide)*k;if(std::fabs(slide-g_tab)<.001f)slide=float(g_tab);
 store->SetFloat(posid,slide);store->SetFloat(timeid,now);
 d->ChannelsSetCurrent(0);
 const float y=start.y+slide*step;
 d->AddRectFilled(ImVec2(start.x,y),ImVec2(start.x+w,y+h),color(ImVec4(.14f,.15f,.18f,1)),u(5));
 d->AddRect(ImVec2(start.x,y),ImVec2(start.x+w,y+h),color(gray(1,.12f)),u(5));
 if(h>u(20)){
  ImVec4 glow=g_accent4;glow.w=.09f;
  d->AddRectFilled(ImVec2(start.x,y+u(7)),ImVec2(start.x+u(6),y+h-u(7)),color(glow),u(3));
  d->AddRectFilled(ImVec2(start.x+u(1),y+u(10)),ImVec2(start.x+u(3),y+h-u(10)),color(g_accent4),u(1));
 }
 d->ChannelsMerge();ImGui::SetCursorScreenPos(ImVec2(start.x,start.y+7*step));
}
static void commit(){
 if(!dirty)return;auto c=resetRequested?aim::Configuration{}:aim::configuration();
 c.normal.enabled=edit.normal.enabled;c.normal.visibleCheck=edit.normal.visibleCheck;c.normal.fov=edit.normal.fov;
 c.normal.showFov=edit.normal.showFov;c.normal.bodies=edit.normal.bodies;c.normal.smoothing=edit.normal.smoothing;
 c.normal.rcs=edit.normal.rcs;c.normal.rcsStrength=edit.normal.rcsStrength;c.silent.enabled=false;c.autoFire.enabled=false;
 aim::publish(c);dirty=false;resetRequested=false;
}
}


// ASTRA_ANIMATED_APPEARANCE_IMPL_V1
namespace astra_esp_extra {
static bool animatedCheckbox(const char* label,bool* value){
 return value ? mixui_final::check(label,*value) : false;
}
}

static void renderEsp(){
 if(!g_b.esp_enabled||!g_espBoxesEnabled.load())return;std::vector<EspScreenBox> boxes;
 {std::lock_guard<std::mutex> lock(g_espMutex);boxes=g_espScreenBoxes;}
 auto* d=ImGui::GetBackgroundDrawList();d->PushClipRect(ImVec2(0,0),ImGui::GetIO().DisplaySize,true);
 for(const auto& b:boxes)astra_esp_extra::draw(d,b);d->PopClipRect();
}


// Render-thread-only particles; no window, no input interception.
// ASTRA_SNOW_MENU_EXCLUSION_V1
static ImVec2 g_snowMenuMinV2{},g_snowMenuMaxV2{};
static bool g_snowMenuVisibleV2=false;
static void astraRenderSnowV2(){
 auto& io=ImGui::GetIO();const ImVec2 screen=io.DisplaySize;
 if(screen.x<1||screen.y<1)return;
 static float opacity=0;
 const float dt=std::clamp(io.DeltaTime,0.f,.05f);
 opacity+=(float(g_open&&g_snowEnabledV3)-opacity)*(1-std::exp(-5.5f*dt));
 if(opacity<.003f&&(!g_open||!g_snowEnabledV3)){opacity=0;return;}
 struct Flake{float x,y,r,speed,phase;};
 static std::vector<Flake> flakes;
 static unsigned seed=0xa932c647u;
 auto rnd=[]()->float{seed=1664525u*seed+1013904223u;return float(seed>>8)*(1.f/16777216.f);};
 const int count=std::clamp(int(double(screen.x)*screen.y/8500.*g_snowDensityV3),30,420);
 if(int(flakes.size())!=count){
  flakes.clear();flakes.reserve(count);
  for(int i=0;i<count;++i){float layer=rnd();flakes.push_back({rnd(),rnd(),1.0f+layer*1.7f,22.f+layer*42.f,rnd()*6.2831853f});}
 }
 // Foreground draws above the dimming layer and above the GUI itself.
 auto* d=ImGui::GetForegroundDrawList();d->PushClipRect(ImVec2(0,0),screen,true);
 const float scale=std::clamp(std::min(screen.x/1040.f,screen.y/750.f),.8f,1.4f);
 const float time=float(std::fmod(ImGui::GetTime(),10000.));
 for(auto& f:flakes){
  f.y+=dt*f.speed*scale*g_snowSpeedV3/screen.y;
  f.x+=dt*(7.f+std::sin(time*.7f+f.phase)*11.f)/screen.x;
  if(f.y>1.015f){f.y=-.015f;f.x=rnd();}
  if(f.x>1.015f)f.x=-.015f;if(f.x<-.015f)f.x=1.015f;
  const float a=opacity*(.40f+.33f*(f.r/2.7f));
  const ImVec2 p(f.x*screen.x,f.y*screen.y);
  // Keep simulation running; skip particles touching the menu or its shadow.
  const float margin=f.r*scale*1.8f+12.f;
  if(g_snowMenuVisibleV2 &&
     p.x>=g_snowMenuMinV2.x-margin && p.x<=g_snowMenuMaxV2.x+margin &&
     p.y>=g_snowMenuMinV2.y-margin && p.y<=g_snowMenuMaxV2.y+margin)continue;

  if(f.r>2.1f)d->AddCircleFilled(p,f.r*scale*1.8f,IM_COL32(255,255,255,int(a*22)),10);
  d->AddCircleFilled(p,f.r*scale,IM_COL32(255,255,255,int(a*255)),10);
 }
 d->PopClipRect();
}


// Screen-space effect: covers the game image, but precedes ESP/menu drawing.
// Night uses native lighting only.

static void renderMinimalMenu(){
 g_snowMenuVisibleV2=false;

 struct SnowAtFrameEndV2 {~SnowAtFrameEndV2(){astraRenderSnowV2();guiToastV3();}} snowAtFrameEndV2;

 using namespace mixui_final;
 aim::g_menuOpen.store(g_open,std::memory_order_release);
 edit=aim::configuration();dirty=false;resetRequested=false;
 // Preserve the original GUI's feature policy.
 // Runtime access disabled for menu isolation.
 auto& io=ImGui::GetIO();if(io.DisplaySize.x<=0||io.DisplaySize.y<=0)return;
 g_tab=std::max(0,std::min(g_tab,6));g_S=clamp(std::min(io.DisplaySize.x/1040.f,io.DisplaySize.y/750.f)*g_uiScaleV3,.55f,2.f);
 updateAimProjection();renderEsp();astra_arrows::render();renderFov(edit.normal);
 const ImGuiStyle saved=ImGui::GetStyle();theme();
 ImGui::SetNextWindowPos(ImVec2(u(12),u(10)),ImGuiCond_Always);
 ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(u(6),u(5)));
 if(ImGui::Begin("##astra-launcher",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing)){
  ImGui::SetWindowFontScale(g_S);
  if(ImGui::Button("Astra",ImVec2(u(120),std::max(32.f,u(34))))){g_open=!g_open;
   aim::g_menuOpen.store(g_open,std::memory_order_release);}
 }ImGui::End();ImGui::PopStyleVar();
 g_openT+=(float(g_open)-g_openT)*(1-std::exp(-20*g_motionSpeedV3*clamp(io.DeltaTime,0,.1f)));
 if(!g_open&&g_openT<.012f){ImGui::GetStyle()=saved;return;}
 ImVec2 size(std::min(u(990),io.DisplaySize.x-u(16)),std::min(u(650),io.DisplaySize.y-u(78)));
 if(size.x<u(350)||size.y<u(210)){ImGui::GetStyle()=saved;return;}
 ImVec2 pos((io.DisplaySize.x-size.x)*.5f,(io.DisplaySize.y-size.y)*.5f+u(7)*(1-g_openT));
 auto* bg=ImGui::GetBackgroundDrawList();bg->AddRectFilled(ImVec2(0,0),io.DisplaySize,IM_COL32(0,0,0,int(dim*g_openT*saved.Alpha)));
 for(int i=3;i>=1;--i){float pad=u(float(i)*3);bg->AddRectFilled(ImVec2(pos.x-pad,pos.y-pad+u(4)),ImVec2(pos.x+size.x+pad,pos.y+size.y+pad+u(4)),IM_COL32(0,0,0,int(18*g_openT*saved.Alpha)),u(10)+pad);}
 const float motionScaleV2=.975f+.025f*std::clamp(g_openT,0.f,1.f);
 pos.x+=size.x*(1-motionScaleV2)*.5f;pos.y+=size.y*(1-motionScaleV2)*.5f;
 size.x*=motionScaleV2;size.y*=motionScaleV2;
 g_snowMenuMinV2=pos;
 g_snowMenuMaxV2=ImVec2(pos.x+size.x,pos.y+size.y);
 g_snowMenuVisibleV2=true;

 ImGui::SetNextWindowPos(pos,ImGuiCond_Always);ImGui::SetNextWindowSize(size,ImGuiCond_Always);
 ImGui::PushStyleVar(ImGuiStyleVar_Alpha,saved.Alpha*g_openT);
 ImGuiWindowFlags flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse;
 if(!g_open)flags|=ImGuiWindowFlags_NoInputs;
 if(ImGui::Begin("##astra-menu",nullptr,flags)){
  ImGui::SetWindowFontScale(g_S);auto p=ImGui::GetCursorScreenPos();float available=ImGui::GetContentRegionAvail().x;
  float head=std::max(u(42),ImGui::GetFontSize()+u(18));auto* d=ImGui::GetWindowDrawList();
  d->AddLine(ImVec2(pos.x+u(13),pos.y+1),ImVec2(pos.x+size.x-u(13),pos.y+1),color(gray(1,.24f)));
  d->AddRectFilled(ImVec2(p.x+u(2),p.y+u(12)),ImVec2(p.x+u(5),p.y+head-u(12)),color(g_accent4),u(1));
  const char* brand="Astra";d->AddText(ImVec2(p.x+u(15),p.y+(head-ImGui::GetFontSize())*.5f),color(g_accent4),brand);
  float brandEnd=p.x+u(15)+ImGui::CalcTextSize(brand).x;
  if(available>u(440))d->AddText(ImVec2(brandEnd+u(18),p.y+(head-ImGui::GetFontSize())*.5f),color(gray(.40f)),"/ INTERNAL");
  ImGui::SetCursorScreenPos(ImVec2(p.x+available-head,p.y));
  if(ImGui::InvisibleButton("##close",ImVec2(head,head))){g_open=false;
   aim::g_menuOpen.store(false,std::memory_order_release);}
  auto cp=ImGui::GetItemRectMin();bool hovered=ImGui::IsItemHovered();
  d->AddRectFilled(cp,ImVec2(cp.x+head,cp.y+head),color(gray(1,hovered?.10f:.035f)),u(4));
  float midx=cp.x+head*.5f,midy=cp.y+head*.5f,r=u(4.5f);auto ink=color(gray(hovered?1.f:.65f));
  d->AddLine(ImVec2(midx-r,midy-r),ImVec2(midx+r,midy+r),ink,u(1.5f));d->AddLine(ImVec2(midx+r,midy-r),ImVec2(midx-r,midy+r),ink,u(1.5f));
  ImGui::SetCursorScreenPos(ImVec2(p.x,p.y+head+u(9)));ImGui::Separator();gap(9);
  const float footerHeightV3=std::max(u(34),ImGui::GetFontSize()+u(14));
  const float footerYV3=ImGui::GetCursorScreenPos().y+ImGui::GetContentRegionAvail().y-footerHeightV3;
  float height=std::max(1.f,ImGui::GetContentRegionAvail().y-footerHeightV3-u(6)),sidebar=u(size.x<u(600)?128.f:155.f);
  if(ImGui::BeginChild("##navigation",ImVec2(sidebar,height),false,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse))navigation();
  ImGui::EndChild();ImGui::SameLine(0,u(14));ImGui::PushID(g_tab);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(u(3),u(3)));
  if(ImGui::BeginChild("##tab-scroll",ImVec2(0,height),false,ImGuiWindowFlags_AlwaysVerticalScrollbar|ImGuiWindowFlags_AlwaysUseWindowPadding))panels();
  ImGui::EndChild();ImGui::PopStyleVar();ImGui::PopID();

  // Dedicated footer; reserve space above so it never overlaps scrolling content.
  ImGui::SetCursorScreenPos(ImVec2(p.x,footerYV3));
  ImGui::Separator();
  static float smoothFpsV3=60;
  const float measured=1.f/std::max(io.DeltaTime,.001f);
  smoothFpsV3+=(measured-smoothFpsV3)*(1-std::exp(-2.f*io.DeltaTime));
  char perfV3[80];std::snprintf(perfV3,sizeof(perfV3),"%.0f FPS / %.1f ms",smoothFpsV3,1000.f/std::max(1.f,smoothFpsV3));
  const float tyV3=footerYV3+u(8);
  d->AddText(ImVec2(p.x,tyV3),color(gray(.44f)),"astra / v1");
  const float perfWidthV3=ImGui::CalcTextSize(perfV3).x;
  if(available>perfWidthV3+u(145))d->AddText(ImVec2(p.x+available-perfWidthV3,tyV3),color(gray(.54f)),perfV3);

 }ImGui::End();ImGui::PopStyleVar();ImGui::GetStyle()=saved;
 

 commit();
 aim::g_menuOpen.store(g_open,std::memory_order_release);
}
