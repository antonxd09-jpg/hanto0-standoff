#pragma once
// Render-thread configuration. Uses existing fresh player/camera snapshots.
namespace astra_arrows {
struct Settings {
 bool enabled=false,glow=true;
 float size=12.f,inset=65.f;
 ImVec4 color{.94f,.96f,1.f,1.f};
};
static Settings settings;
static void reset(){settings=Settings{};}
static ImU32 ink(ImVec4 c,float alpha=1){c.w*=alpha;return ImGui::ColorConvertFloat4ToU32(c);}
static void arrow(ImDrawList* d,ImVec2 c,ImVec2 dir,float size,ImVec4 col,bool glow){
 const ImVec2 side(-dir.y,dir.x);
 auto pt=[&](float forward,float lateral){return ImVec2(c.x+dir.x*forward+side.x*lateral,c.y+dir.y*forward+side.y*lateral);};
 const ImVec2 tip=pt(size,0),left=pt(-size*.72f,size*.65f),right=pt(-size*.72f,-size*.65f),notch=pt(-size*.28f,0);
 if(glow){
  // Subtle local halo, not a flashing/pulsing animation.
  for(int i=3;i>=1;--i)d->AddCircleFilled(c,size*(.8f+.3f*i),ink(col,.018f),20);
 }
 d->AddTriangleFilled(tip,left,notch,ink(col));
 d->AddTriangleFilled(tip,notch,right,ink(col,.78f));
 const auto outline=IM_COL32(2,5,9,225);
 d->AddLine(tip,left,outline,1.25f);d->AddLine(left,notch,outline,1.25f);
 d->AddLine(notch,right,outline,1.25f);d->AddLine(right,tip,outline,1.25f);
 d->AddLine(pt(size*.70f,0),pt(-size*.22f,size*.33f),IM_COL32(255,255,255,105),1.f);
}
static void render(){
 if(!settings.enabled||!g_b.esp_enabled||!g_espBoxesEnabled.load())return;
 namespace ed=astra::esp::detail;
 using Vec3=astra::esp::Vec3;
 const auto screen=ImGui::GetIO().DisplaySize;
 if(screen.x<160||screen.y<120||!ed::g_runtime.ready)return;
 void* local=nullptr;
 {std::lock_guard<std::mutex> lock(ed::g_hookPlayerMutex);local=ed::g_hookLocalPlayer;}
 if(!local||ed::objectClass(local)!=ed::g_runtime.playerClass)return;
 ed::PlayerSnapshot ls{};
 if(!ed::readPlayerSnapshot(local,ls)||!ls.localOwned||!ls.statsValid||!ls.hp||!ed::validTeam(ls.team))return;
 auto world=ed::readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(local)+astra::offsets::field::mfc::kWorld));
 Vec3 localPos{};
 if(!world||!ed::readPlayerPoseSnapshot(local,localPos))return;
 astra::esp::CameraMatrixCache matrices{};std::uintptr_t camera=0;
 if(!ed::readCameraForPresent(matrices,camera))return;
 std::array<ed::PlayerPoseCache,128> poses{};
 {std::lock_guard<std::mutex> lock(ed::g_playerPoseMutex);poses=ed::g_playerPoses;}
 const auto now=std::chrono::steady_clock::now();
 const float scale=std::clamp(screen.y/1080.f,.85f,1.4f);
 const float size=std::clamp(settings.size,7.f,24.f)*scale;
 const float margin=std::max(size*2.f,std::clamp(settings.inset,30.f,180.f)*scale);
 const float rx=screen.x*.5f-margin,ry=screen.y*.5f-margin;
 if(rx<size*2||ry<size*2)return;
 const ImVec2 center(screen.x*.5f,screen.y*.5f);
 struct Marker {float angle;};std::array<Marker,64> markers{};unsigned count=0;
 for(const auto& pose:poses){
  if(!pose.player||pose.player==local||pose.world!=world||now<pose.captured||now-pose.captured>std::chrono::milliseconds(150))continue;
  if(ed::objectClass(pose.player)!=ed::g_runtime.playerClass)continue;
  auto currentWorld=ed::readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(pose.player)+astra::offsets::field::mfc::kWorld));
  if(currentWorld!=world)continue;
  ed::PlayerSnapshot ps{};
  if(!ed::readPlayerSnapshot(pose.player,ps)||ps.localOwned||!ps.statsValid||!ps.hp||!ed::validTeam(ps.team)||ps.team==ls.team)continue;
  const auto& feet=pose.position;
  if(!std::isfinite(feet.x)||!std::isfinite(feet.y)||!std::isfinite(feet.z))continue;
  const Vec3 head{feet.x,feet.y+astra::offsets::managed::kFallbackPlayerHeight,feet.z};
  Vec3 fs{},hs{};
  const bool fp=ed::project(matrices,feet,int(screen.x),int(screen.y),fs);
  const bool hp=ed::project(matrices,head,int(screen.x),int(screen.y),hs);
  if(fp&&hp){
   const float height=std::fabs(fs.y-hs.y),cx=(fs.x+hs.x)*.5f,half=height*.24f;
   if(cx+half>=0&&cx-half<=screen.x&&std::max(fs.y,hs.y)>=0&&std::min(fs.y,hs.y)<=screen.y)continue;
  }
  const auto view=ed::multiply(matrices.worldToCamera,{feet.x,feet.y+.89f,feet.z,1});
  if(!std::isfinite(view.x)||!std::isfinite(view.y)||!std::isfinite(view.z))continue;
  float dx=0,dy=0;
  if(-view.z>.05f){
   const auto clip=ed::multiply(matrices.projection,view);
   if(!std::isfinite(clip.x)||!std::isfinite(clip.y)||!std::isfinite(clip.w)||std::fabs(clip.w)<.0001f)continue;
   dx=clip.x/clip.w*screen.x*.5f;dy=-clip.y/clip.w*screen.y*.5f;
  }else{
   // Behind camera: a bearing indicator; directly behind points down.
   dx=view.x;dy=std::max(.01f,view.z);
  }
  if(!std::isfinite(dx)||!std::isfinite(dy)||std::hypot(dx,dy)<.001f)continue;
  if(count<markers.size())markers[count++]={std::atan2(dy,dx)};
 }
 auto* d=ImGui::GetBackgroundDrawList();d->PushClipRect(ImVec2(0,0),screen,true);
 for(unsigned i=0;i<count;++i){
  const float a=markers[i].angle;const ImVec2 dir(std::cos(a),std::sin(a));
  // Nearby bearings use concentric lanes; direction is never distorted.
  unsigned lane=0;
  for(unsigned j=0;j<i;++j)if(std::fabs(std::remainder(a-markers[j].angle,6.2831853f))<.11f)++lane;
  lane=std::min(lane,5u);
  const float shift=float(lane)*(size*2.2f);
  const float ax=std::max(size*2,rx-shift),ay=std::max(size*2,ry-shift);
  const float distance=1.f/std::sqrt(dir.x*dir.x/(ax*ax)+dir.y*dir.y/(ay*ay));
  arrow(d,ImVec2(center.x+dir.x*distance,center.y+dir.y*distance),dir,size,settings.color,settings.glow);
 }
 d->PopClipRect();
}
}
