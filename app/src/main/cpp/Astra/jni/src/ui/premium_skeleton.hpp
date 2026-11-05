#pragma once
// Included inside premium.hpp after its local settings declarations.
namespace astra_skeleton_ui {
static void controls(){
 auto s=astra_skeleton::settings();bool changed=false;
 ImGui::Separator();ImGui::TextUnformatted("Skeleton ESP");
 changed|=ImGui::Checkbox("Skeleton",&s.enabled);
 changed|=ImGui::Checkbox("Skeleton teammates",&s.teammates);
 changed|=ImGui::SliderFloat("Bone thickness",&s.thickness,.5f,4.f,"%.1f");
 changed|=ImGui::ColorEdit4("Enemy bones",s.enemy,ImGuiColorEditFlags_NoInputs);
 changed|=ImGui::ColorEdit4("Team bones",s.team,ImGuiColorEditFlags_NoInputs);
 changed|=ImGui::Combo("Bone dictionary layout",&s.layout,"Unity standard\0Donor alternate\0");
 if(changed)astra_skeleton::publish(s);
 ImGui::TextDisabled("Resolved bones: %d / 19",astra_skeleton::lastBones.load());
 ImGui::TextWrapped("If joints are incorrect, switch dictionary layout. No occlusion bypass.");
}
static void render(){
 auto s=astra_skeleton::settings();if(!s.enabled||!g_b.esp_enabled)return;
 auto frame=astra_skeleton::frame();auto now=astra_skeleton::Clock::now();
 if(!frame.world||now-frame.captured>std::chrono::milliseconds(250))return;
 astra::esp::CameraMatrixCache matrices{};std::uintptr_t camera=0;
 if(!astra::esp::detail::readCameraForPresent(matrices,camera))return;
 auto display=ImGui::GetIO().DisplaySize;if(display.x<1||display.y<1)return;
 auto* d=ImGui::GetBackgroundDrawList();d->PushClipRect(ImVec2(0,0),display,true);
 for(const auto& pose:frame.poses){
  if(!pose.player||pose.world!=frame.world||pose.layout!=s.layout||now-pose.captured>std::chrono::milliseconds(150))continue;
  bool enemy=pose.team!=frame.team;if(!enemy&&!s.teammates)continue;
  std::array<astra::esp::Vec3,astra_skeleton::count> projected{};
  std::array<bool,astra_skeleton::count> valid{};
  for(int i=0;i<astra_skeleton::count;++i)if(pose.valid[i])valid[i]=astra::esp::detail::project(matrices,pose.positions[i],int(display.x),int(display.y),projected[i]);
  const float* c=enemy?s.enemy:s.team;
  auto color=ImGui::ColorConvertFloat4ToU32(ImVec4(c[0],c[1],c[2],c[3]));
  float th=s.thickness*g_S;
  auto line=[&](int a,int b){
   if(!valid[a]||!valid[b])return;
   const auto& p=pose.positions[a];const auto& q=pose.positions[b];
   float dx=p.x-q.x,dy=p.y-q.y,dz=p.z-q.z;
   if(dx*dx+dy*dy+dz*dz>2.25f)return;
   ImVec2 x(projected[a].x,projected[a].y),y(projected[b].x,projected[b].y);
   d->AddLine(x,y,IM_COL32(0,0,0,180),th+2);d->AddLine(x,y,color,th);
  };
  for(const auto& edge:astra_skeleton::links)line(edge[0],edge[1]);
  if(!pose.valid[3])line(2,4);
 }
 d->PopClipRect();
}
}
