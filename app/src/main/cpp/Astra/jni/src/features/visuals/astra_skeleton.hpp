#pragma once
#include "features/aim/runtime.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace astra_skeleton {
namespace ed=astra::esp::detail;
using Vec3=astra::esp::Vec3;
using Clock=std::chrono::steady_clock;
constexpr int count=19;
// Slot order: head, neck, chest, spine, hips; left/right arms; left/right legs.
inline constexpr int links[][2]={
 {0,1},{1,2},{2,3},{3,4},{2,8},{8,7},{7,6},{6,5},
 {2,12},{12,11},{11,10},{10,9},{4,15},{15,14},{14,13},
 {4,18},{18,17},{17,16}
};
// Standard Unity HumanBodyBones vs the donor's alternate dictionary layout.
// These are dictionary keys, NOT aim HitBox enum values.
inline constexpr int keys[2][count]={
 {10,9,8,7,0,17,15,13,11,18,16,14,12,5,3,1,6,4,2},
 {11,10,8,7,0,18,16,14,12,19,17,15,13,5,3,1,6,4,2}
};
struct Settings {
 bool enabled=false,teammates=false;
 int layout=0;
 float thickness=1.4f;
 float enemy[4]={1,.36f,.30f,1};
 float team[4]={.36f,.82f,.60f,1};
};
inline std::mutex configMutex;
inline Settings config;
inline std::atomic<bool> active{false};
inline std::atomic<int> lastBones{0};
inline std::atomic<unsigned long long> published{0},rejected{0};
inline Settings settings(){std::lock_guard<std::mutex> l(configMutex);return config;}
inline float finiteClamp(float v,float fallback,float lo,float hi){return std::isfinite(v)?std::max(lo,std::min(v,hi)):fallback;}
inline void publish(Settings s){
 s.layout=std::max(0,std::min(s.layout,1));s.thickness=finiteClamp(s.thickness,1.4f,.5f,4.f);
 for(int i=0;i<4;++i){s.enemy[i]=finiteClamp(s.enemy[i],1,0,1);s.team[i]=finiteClamp(s.team[i],1,0,1);}
 {std::lock_guard<std::mutex> l(configMutex);config=s;}
 active.store(s.enabled,std::memory_order_release);
}
struct Pose {
 std::uintptr_t player=0,world=0;
 int team=0,layout=0;
 std::array<Vec3,count> positions{};
 std::array<bool,count> valid{};
 Clock::time_point captured{};
};
inline std::mutex poseMutex;
inline std::array<Pose,128> poses{};
inline std::uintptr_t currentWorld=0;
inline int currentTeam=0;
inline Clock::time_point contextTime{};
inline void clear(){
 std::lock_guard<std::mutex> l(poseMutex);poses={};currentWorld=0;currentTeam=0;contextTime={};lastBones.store(0);
}
template<class T> inline bool field(void* p,std::size_t offset,T& v){
 return p&&ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(p)+offset),v);
}
inline void* object(void* p,std::size_t offset){void* v=nullptr;return field(p,offset,v)?v:nullptr;}
// Read the class name into a bounded local buffer, rather than dereferencing
// a potentially stale C string with strcmp. Uses existing classGetName ABI.
inline bool named(void* p,const char* expected){
 if(!p)return false;void* klass=ed::objectClass(p);if(!klass)return false;
 const char* name=ed::className(klass);if(!name)return false;
 for(std::size_t i=0;i<64;++i){char c=0;if(!ed::readValue(name+i,c)||c!=expected[i])return false;if(!c)return true;}
 return false;
}
inline void* bindings(void* p){
 auto* parts=object(p,astra::offsets::field::mfc::kPartsContainer);
 auto* list=object(parts,0x10);int n=0;if(!field(list,0x18,n)||n<1||n>64)return nullptr;
 auto* items=object(list,0x10);std::uintptr_t capacity=0;
 if(!field(items,0x18,capacity)||capacity<unsigned(n)||capacity>256)return nullptr;
 for(int i=0;i<n;++i){
  auto* part=object(items,0x20+8*std::size_t(i));if(!named(part,"bis"))continue;
  auto* b=object(part,astra::offsets::field::jgi::kCharacterBindings);
  return named(b,"CharacterBindings")?b:nullptr;
 }
 return nullptr;
}
inline bool position(void* transform,Vec3 root,Vec3& p){
 if(!named(transform,"Transform"))return false;
 std::uintptr_t native=0,vt=0,fn=0;
 if(!field(transform,astra::offsets::managed::kUnityObjectCachedPtr,native)||!native)return false;
 if(!ed::readValue(reinterpret_cast<void*>(native),vt)||!vt||!ed::readValue(reinterpret_cast<void*>(vt),fn))return false;
 const auto base=ed::g_runtime.unityBase;
 if(fn<base+astra::offsets::elf::kTextBegin||fn>=base+astra::offsets::elf::kTextEnd)return false;
 if(!ed::readPosition(native,p))return false;
 float dx=p.x-root.x,dy=p.y-root.y,dz=p.z-root.z;
 return dx*dx+dy*dy+dz*dz<16.f;
}
inline void collect(void* player){
 if(!active.load(std::memory_order_acquire))return;
 if(!ed::isUnityMainCallback()||!ed::g_runtime.ready||!player)return;
 Settings s=settings();if(!s.enabled)return;
 void* local=nullptr;{std::lock_guard<std::mutex> l(ed::g_hookPlayerMutex);local=ed::g_hookLocalPlayer;}
 std::uint8_t owned=0;
 if(!local||!field(local,astra::offsets::field::mfc::kLocalOwned,owned)||owned!=1){clear();return;}
 ed::PlayerSnapshot localSnapshot{};
 void* world=object(local,astra::offsets::field::mfc::kWorld);
 if(!world||!ed::readPlayerSnapshot(local,localSnapshot)||!ed::validTeam(localSnapshot.team)){clear();return;}
 auto now=Clock::now();
 {
  std::lock_guard<std::mutex> l(poseMutex);
  if(currentWorld!=reinterpret_cast<std::uintptr_t>(world)||currentTeam!=localSnapshot.team)poses={};
  currentWorld=reinterpret_cast<std::uintptr_t>(world);currentTeam=localSnapshot.team;contextTime=now;
 }
 if(player==local)return;
 ed::PlayerSnapshot snapshot{};
 if(!ed::readPlayerSnapshot(player,snapshot)||!snapshot.statsValid||!snapshot.hp||snapshot.localOwned||!ed::validTeam(snapshot.team)||object(player,astra::offsets::field::mfc::kWorld)!=world)return;
 if(snapshot.team==localSnapshot.team&&!s.teammates)return;
 // Bound collection frequency per player; re-resolve pointers rather than
 // retaining Transform references across respawns or loadout changes.
 {
  std::lock_guard<std::mutex> l(poseMutex);
  for(const auto& p:poses)if(p.player==reinterpret_cast<std::uintptr_t>(player)&&p.layout==s.layout&&now-p.captured<std::chrono::milliseconds(16))return;
 }
 Vec3 root{};if(!ed::readPlayerPoseSnapshot(player,root))return;
 auto* b=bindings(player);auto* dict=object(b,astra::offsets::field::character_bindings::kBones);
 int n=0;auto* entries=object(dict,0x18);std::uintptr_t capacity=0;
 if(!field(dict,0x20,n)||n<1||n>256||!field(entries,0x18,capacity)||capacity<unsigned(n)||capacity>512){rejected.fetch_add(1);return;}
 Pose pose{};pose.player=reinterpret_cast<std::uintptr_t>(player);pose.world=reinterpret_cast<std::uintptr_t>(world);pose.team=snapshot.team;pose.layout=s.layout;pose.captured=now;
 for(int i=0;i<n;++i){
  std::size_t offset=0x20+std::size_t(i)*0x18;int hash=-1,key=-1;
  if(!field(entries,offset,hash)||hash<0||!field(entries,offset+8,key))continue;
  int slot=-1;for(int j=0;j<count;++j)if(keys[s.layout][j]==key){slot=j;break;}
  if(slot<0||pose.valid[slot])continue;
  pose.valid[slot]=position(object(entries,offset+0x10),root,pose.positions[slot]);
 }
 int valid=0;for(bool v:pose.valid)valid+=v;
 lastBones.store(valid);
 // Require a torso anchor. No strict world-Y ordering (crouch/lean changes it).
 if(valid<4||!pose.valid[4]||(!pose.valid[2]&&!pose.valid[3])){rejected.fetch_add(1);return;}
 std::lock_guard<std::mutex> l(poseMutex);Pose* dst=nullptr;
 for(auto& p:poses){if(p.player==pose.player){dst=&p;break;}if(!dst&&(!p.player||now-p.captured>std::chrono::milliseconds(250)))dst=&p;}
 if(dst){*dst=pose;published.fetch_add(1);}
}
// Replaces the existing single player-frame callback registration; forwards
// to the original aim handler exactly once, including when Skeleton is off.
inline void onPlayer(void* player){
 astra::aim::runtime::onPlayer(player);
 collect(player);
}
struct Frame {std::array<Pose,128> poses;std::uintptr_t world;int team;Clock::time_point captured;};
inline Frame frame(){std::lock_guard<std::mutex> l(poseMutex);return {poses,currentWorld,currentTeam,contextTime};}
}
