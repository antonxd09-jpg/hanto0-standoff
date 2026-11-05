// Included inside astra::aim::runtime, after field/native/Actor helpers.
inline bool bytesMatch(std::uintptr_t,std::initializer_list<std::uint32_t>);
using RaycastFn=bool(*)(const std::uint64_t*,const Ray*,assists::RaycastHit*,float,int,int);
using DefaultSceneFn=std::uint64_t(*)(const void*);
using EntityIdFn=std::int32_t(*)(void*,const void*);
inline RaycastFn g_raycast=nullptr;inline DefaultSceneFn g_defaultScene=nullptr;inline EntityIdFn g_entityId=nullptr;
inline assists::RecoilTracker g_recoilTracker{};inline unsigned g_visibilityBudget=0;
inline Clock::time_point g_lastAssistBind{};
inline void prepareAssists(bool rcs,bool visibility){
 if(!rcs&&!visibility)return;auto now=Clock::now();if(now-g_lastAssistBind<std::chrono::seconds(1))return;g_lastAssistBind=now;auto b=ed::g_runtime.unityBase;
 if(rcs&&!g_rcsReady.load()){
  bool ok=bytesMatch(b+aim_offsets::kRecoilScale,{0xd10103ff,0x6d0123e9,0xa90257fe,0xa9034ff4})&&bytesMatch(b+aim_offsets::kRecoilPointLoad,{0x2d4f26a8})&&bytesMatch(b+aim_offsets::kRecoilMultiplierLoad,{0xbd404261});
  g_rcsReady.store(ok);if(ok)debug::log("RCS profile validated: ActualPoint/Multiplier and native ray-builder axes");
 }
 if(visibility&&!g_visibilityReady.load()){
  bool ok=bytesMatch(b+silent_offsets::kNativeRaycast,{0xfc1c0fe8,0xf90007fe,0xa9015ff8,0xa90257f6})&&bytesMatch(b+silent_offsets::kDefaultPhysicsScene,{0xaa1f03e0,0xd65f03c0})&&bytesMatch(b+silent_offsets::kObjectEntityId,{0xf81e0ffe,0xa9014ff4,0xb0033b54,0xaa0003f3});
  if(ok){g_raycast=reinterpret_cast<RaycastFn>(b+silent_offsets::kNativeRaycast);g_defaultScene=reinterpret_cast<DefaultSceneFn>(b+silent_offsets::kDefaultPhysicsScene);g_entityId=reinterpret_cast<EntityIdFn>(b+silent_offsets::kObjectEntityId);g_visibilityReady.store(true);debug::log("visibility profile validated: 8-byte PhysicsScene, 44-byte RaycastHit, first collider ownership");}
 }
}
inline bool g_noRecoilPatch=false;inline std::uint32_t g_noRecoilOriginal=0;
inline bool patchWord(std::uintptr_t addr,std::uint32_t value,std::uint32_t& old){
 if(!ed::readValue(reinterpret_cast<void*>(addr),old))return false;
 auto page=addr&~std::uintptr_t(4095);
 if(mprotect(reinterpret_cast<void*>(page),4096,PROT_READ|PROT_WRITE|PROT_EXEC))return false;
 __atomic_store_n(reinterpret_cast<std::uint32_t*>(addr),value,__ATOMIC_RELEASE);
 __builtin___clear_cache(reinterpret_cast<char*>(addr),reinterpret_cast<char*>(addr+4));
 mprotect(reinterpret_cast<void*>(page),4096,PROT_READ|PROT_EXEC);
 return true;
}
// The ray builder calls 0x972976C, which returns mult * ActualPoint and adds it
// to the shot direction. Loading the multiplier as zero removes recoil AND
// spread from the bullet while leaving the state fields intact for diagnostics.
inline void setNoRecoilPatch(bool on){
 auto base=ed::g_runtime.unityBase;if(!base)return;
 std::uintptr_t addr=base+aim_offsets::kRecoilMultiplierLoad;
 if(on&&!g_noRecoilPatch){
  std::uint32_t word=0;if(!ed::readValue(reinterpret_cast<void*>(addr),word)||word!=0xbd404261u)return;
  std::uint32_t patched=0x1e270000u|(31u<<5)|(word&0x1Fu);
  if(patchWord(addr,patched,g_noRecoilOriginal)){g_noRecoilPatch=true;debug::noRecoilPatch.store(1);debug::log("no-recoil spread patch installed rva=0x%llx",(unsigned long long)aim_offsets::kRecoilMultiplierLoad);}
 }else if(!on&&g_noRecoilPatch){
  std::uint32_t cur=0;std::uint32_t expect=0x1e270000u|(31u<<5)|(g_noRecoilOriginal&0x1Fu);
  if(ed::readValue(reinterpret_cast<void*>(addr),cur)&&cur!=expect){g_noRecoilPatch=false;debug::noRecoilPatch.store(0);return;}
  std::uint32_t old=0;if(patchWord(addr,g_noRecoilOriginal,old)){g_noRecoilPatch=false;debug::noRecoilPatch.store(0);debug::log("no-recoil spread patch removed");}
 }
}
inline void applyWeaponNoRecoil(bool on,void* weaponPart){
 if(!weaponPart)return;
 float mult=0;assists::Point2 point{};
 bool haveMult=field(weaponPart,aim_offsets::kRecoilMultiplier,mult);
 bool havePoint=field(weaponPart,aim_offsets::kRecoilActualPoint,point);
 if(haveMult)debug::weaponMult.store(mult);
 if(havePoint){debug::weaponPointX.store(point.x);debug::weaponPointY.store(point.y);}
 if(!on)return;
 std::uintptr_t base=reinterpret_cast<std::uintptr_t>(weaponPart);
 // Only the point is zeroed: the multiplier stays for the game's own maths, and
 // the code patch already removes its effect on the bullet.
 if(havePoint){assists::Point2 zero{};auto* a=reinterpret_cast<void*>(base+aim_offsets::kRecoilActualPoint);if(ed::writable(a,sizeof(zero)))std::memcpy(a,&zero,sizeof(zero));}
 debug::weaponNoRecoilWrites.fetch_add(1);
}
inline bool readRecoil(void* weaponPart,Angles& out){
 debug::rcsReads.fetch_add(1);assists::Point2 point{};float mult=0;
 bool ok=g_rcsReady.load()&&named(weaponPart,"nrk")&&field(weaponPart,aim_offsets::kRecoilActualPoint,point)&&field(weaponPart,aim_offsets::kRecoilMultiplier,mult)&&assists::recoilAngles(point,mult,out);
 if(!ok){debug::rcsRejected.fetch_add(1);return false;}debug::recoilPitch.store(out.pitch);debug::recoilYaw.store(out.yaw);return true;
}
inline bool visibilityAccept(void* context,std::uintptr_t player,Vec3 origin,Vec3 point){
 auto* actor=static_cast<const Actor*>(context);
 if(!actor||reinterpret_cast<std::uintptr_t>(actor->player)!=player||!g_visibilityReady.load()||!g_raycast||!g_defaultScene||!ed::isUnityMainCallback()||!g_visibilityBudget){debug::visibilityUnknown.fetch_add(1);return false;}
 bool owned=false;for(unsigned i=0;i<actor->colliderCount&&i<actor->colliderIds.size();++i)owned|=actor->colliderIds[i]!=0;if(!owned){debug::visibilityUnknown.fetch_add(1);return false;}
 --g_visibilityBudget;auto delta=sub(point,origin);float distance=length(delta);
 if(!finite(origin)||!finite(point)||!std::isfinite(distance)||distance<.2f||distance>501){debug::visibilityUnknown.fetch_add(1);return false;}
 Ray ray{origin,normalized(delta)};assists::RaycastHit hit{};auto scene=g_defaultScene(nullptr);debug::visibilityRays.fetch_add(1);
 // All standard raycast layers except IgnoreRaycast; include hitbox triggers.
 bool collision=g_raycast(&scene,&ray,&hit,distance+.05f,-5,2);
 if(!collision){debug::visibilityClear.fetch_add(1);return true;}
 if(!hit.collider||!std::isfinite(hit.distance)||hit.distance<0||hit.distance>distance+.06f){debug::visibilityUnknown.fetch_add(1);return false;}
 for(unsigned i=0;i<actor->colliderCount&&i<actor->colliderIds.size();++i)if(actor->colliderIds[i]&&actor->colliderIds[i]==hit.collider){debug::visibilityTarget.fetch_add(1);return true;}
 debug::visibilityBlocked.fetch_add(1);return false;
}
