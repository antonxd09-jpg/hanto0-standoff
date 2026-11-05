#pragma once
#include "features/aim/runtime.hpp"
#include "features/visuals/worldcolor_runtime.hpp"
#include "features/visuals/hudcolor_runtime.hpp"
#include "features/visuals/astra_night.hpp"
#include <mutex>
#include <cmath>

namespace astra_world_hud {
struct Settings {
 astra::worldcolor::Settings world;
 astra::hudcolor::Settings hud;
};
inline std::mutex mutex;
inline Settings requested;
inline unsigned revision=0;
inline std::atomic<bool> wanted{false},cleanupPending{false};
inline std::atomic<int> worldState{0},hudState{0};
inline Settings configuration(){std::lock_guard<std::mutex> l(mutex);return requested;}
inline float clean(float x,float fallback,float lo,float hi){return std::isfinite(x)?std::max(lo,std::min(x,hi)):fallback;}
inline void color(astra::worldcolor::Color4& c){
 c.r=clean(c.r,1,0,1);c.g=clean(c.g,1,0,1);c.b=clean(c.b,1,0,1);c.a=clean(c.a,1,0,1);
}
inline void publish(Settings s){
 color(s.world.color);color(s.world.fogColor);color(s.hud.tint);
 s.world.fogStart=clean(s.world.fogStart,0,0,4999);
 s.world.fogEnd=clean(s.world.fogEnd,150,s.world.fogStart+1,5000);
 {std::lock_guard<std::mutex> l(mutex);requested=s;++revision;}
 cleanupPending.store(true,std::memory_order_release);
 wanted.store(s.world.enabled||s.world.fog||s.hud.enabled,std::memory_order_release);
}
inline bool needsRuntime(){return wanted.load(std::memory_order_acquire)||cleanupPending.load(std::memory_order_acquire)||astra_night::needsRuntime();}
// All donor Settings accesses happen on UnityMain only. The render thread
// publishes a separate draft through the mutex above.
inline void onPlayer(void* player){
 astra::aim::runtime::onPlayer(player);
 namespace ed=astra::esp::detail;
 if(!ed::isUnityMainCallback()||!ed::g_runtime.ready||!player)return;
 void* local=nullptr;{std::lock_guard<std::mutex> l(ed::g_hookPlayerMutex);local=ed::g_hookLocalPlayer;}
 if(player!=local)return;
 astra_night::tick(player);
 Settings next;unsigned version;
 {std::lock_guard<std::mutex> l(mutex);next=requested;version=revision;}
 static unsigned appliedVersion=~0u;
 static std::uintptr_t world=0;
 auto* current=ed::readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player)+astra::offsets::field::mfc::kWorld));
 if(!current)return;
 auto id=reinterpret_cast<std::uintptr_t>(current);
 if(world!=id){
  // Never restore objects from a previous scene into the new one.
  astra::worldcolor::detail::g_savedCount=0;
  astra::worldcolor::detail::g_fogSaved.valid=false;
  astra::worldcolor::g_appliedFlag=false;
  astra::hudcolor::detail::g_savedCount=0;
  astra::worldcolor::detail::g_lastScan={};
  astra::hudcolor::detail::g_lastScan={};
  world=id;appliedVersion=~0u;
 }
 if(version!=appliedVersion){
  // Undo texture removal before applying a different solid/color setting.
  if(astra::worldcolor::g_appliedFlag){
   astra::worldcolor::detail::restoreMaterials();
   astra::worldcolor::g_appliedFlag=false;
  }
  // Restoring first also makes Include text -> off restore already tinted text.
  if(astra::hudcolor::detail::g_savedCount&&astra::hudcolor::detail::g_fn.setColor)
   astra::hudcolor::detail::restoreAll();
  astra::worldcolor::g_settings=next.world;
  astra::hudcolor::g_settings=next.hud;
  astra::worldcolor::detail::g_lastScan={};
  astra::hudcolor::detail::g_lastScan={};
  appliedVersion=version;
 }
 if(needsRuntime()||next.world.enabled||next.world.fog||next.hud.enabled){
  astra::worldcolor::tick(player);
  astra::hudcolor::onPlayerUpdate(player);
 }
 worldState.store(astra::worldcolor::g_status.load());
 hudState.store(astra::hudcolor::g_status.load());
 // Do not lose a publication that arrived during the native calls.
 {std::lock_guard<std::mutex> l(mutex);if(revision==version)cleanupPending.store(false,std::memory_order_release);}
}
}
