#pragma once
#include "imgui.h"
#include <algorithm>
namespace astra_sections {
// Bounded header for manually drawn cards. No span across the parent window.
inline bool header(const char* label,float requestedWidth) {
 ImGui::PushID(label);
 auto* state=ImGui::GetStateStorage();
 const ImGuiID key=ImGui::GetID("##expanded");
 bool open=state->GetBool(key,false);
 const float w=std::max(1.f,requestedWidth);
 const float fs=ImGui::GetFontSize(),h=std::max(30.f,fs+18.f);
 const ImVec2 pos=ImGui::GetCursorScreenPos();
 if(ImGui::InvisibleButton("##section",ImVec2(w,h))) {
  open=!open;state->SetBool(key,open);
 }
 const bool hover=ImGui::IsItemHovered();
 auto* d=ImGui::GetWindowDrawList();
 const ImVec2 end(pos.x+w,pos.y+h);
 d->PushClipRect(pos,end,true);
 d->AddRectFilled(pos,end,ImGui::GetColorU32(hover?ImGuiCol_HeaderHovered:ImGuiCol_Header),4.f);
 const float cx=pos.x+13.f,cy=pos.y+h*.5f,r=4.f;
 const auto ink=ImGui::GetColorU32(ImGuiCol_Text);
 if(open)d->AddTriangleFilled(ImVec2(cx-r,cy-r*.5f),ImVec2(cx+r,cy-r*.5f),ImVec2(cx,cy+r),ink);
 else d->AddTriangleFilled(ImVec2(cx-r*.5f,cy-r),ImVec2(cx-r*.5f,cy+r),ImVec2(cx+r,cy),ink);
 d->AddText(ImVec2(pos.x+26.f,pos.y+(h-fs)*.5f),ink,label);
 d->PopClipRect();ImGui::PopID();return open;
}
}
