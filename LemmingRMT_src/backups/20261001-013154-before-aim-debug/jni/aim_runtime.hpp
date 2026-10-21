#pragma once
#include "aim_core.hpp"
#include "aim_offsets.hpp"
#include "esp_runtime.hpp"
#include <sys/mman.h>
#include <cstring>
namespace lemming::aim::runtime {
namespace ed=lemming::esp::detail;using Clock=std::chrono::steady_clock;
struct Bounds{Vec3 center,extents;};struct Ray{Vec3 origin,direction;};struct ExecuteTime{float time,delta;};
static_assert(sizeof(Bounds)==24&&sizeof(Ray)==24&&sizeof(ExecuteTime)==8,"ARM64 value type ABI");
using BoundsFn=void(*)(std::uintptr_t,Bounds*);using ClampFn=Angles(*)(void*,Angles,const void*);using ApplyFn=void(*)(void*,Angles,const void*);using RefreshFn=void(*)(void*,const void*);using CameraTickFn=void(*)(void*,ExecuteTime,const void*);
// Verified call-site: x0=jxy, x1=unboxed WeaponContext*, x2=Ray*, x3=length/padding.
// rxjx EMITS the pre-generated rays; redirect before original, not after it.
using EmitFn=void(*)(void*,void*,Ray*,std::uint64_t);
inline std::atomic<CameraTickFn> g_cameraOriginal{nullptr};inline EmitFn g_emitOriginal=nullptr;inline BoundsFn g_bounds=nullptr;inline ClampFn g_clamp=nullptr;inline ApplyFn g_apply=nullptr;inline RefreshFn g_refresh=nullptr;inline bool g_functionsValid=false;
inline std::uintptr_t g_lastNormal=0,g_lastSilent=0;inline Clock::time_point g_lastInstall{};inline void* g_relay=nullptr;
struct Actor{void* player=nullptr;void* world=nullptr;float hp=0;int team=0;std::array<HitVolume,32> volumes{};unsigned count=0;Clock::time_point captured{};};inline std::array<Actor,128> g_actors{};
inline bool unityCode(std::uintptr_t p){auto b=ed::g_runtime.unityBase;return b&&p>=b+offsets::elf::kTextBegin&&p<b+offsets::elf::kTextEnd;}
template<class T> inline bool field(void* o,std::size_t off,T& out){return o&&ed::readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(o)+off),out);}
inline void* object(void* o,std::size_t off){return o?ed::readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(o)+off)):nullptr;}
inline bool named(void* o,const char* n){return o&&ed::nameEquals(ed::className(ed::objectClass(o)),n);}
inline std::uintptr_t native(void* o,const char* n=nullptr){if(!o||(n&&!named(o,n)))return 0;auto p=reinterpret_cast<std::uintptr_t>(object(o,0x10));std::uintptr_t vt=0,first=0;if(!p||!ed::readValue(reinterpret_cast<void*>(p),vt)||!ed::readValue(reinterpret_cast<void*>(vt),first)||!unityCode(first))return 0;return p;}
inline void* part(void* player,const char* n){void* list=object(object(player,offsets::field::mfc::kPartsContainer),0x10);std::int32_t count=0;if(!field(list,0x18,count)||count<=0||count>64)return nullptr;void* arr=object(list,0x10);std::uintptr_t len=0;if(!field(arr,0x18,len)||len<static_cast<unsigned>(count)||len>256)return nullptr;for(int i=0;i<count;++i){void* p=object(arr,0x20+std::size_t(i)*8);if(named(p,n))return p;}return nullptr;}
inline bool localAlive(void* p,ed::PlayerSnapshot& s,void*& world){std::uint8_t local=0;world=object(p,offsets::field::mfc::kWorld);return p&&ed::objectClass(p)==ed::g_runtime.playerClass&&field(p,offsets::field::mfc::kLocalOwned,local)&&local==1&&world&&ed::readPlayerSnapshot(p,s)&&s.localOwned&&s.statsValid&&s.hp>0&&ed::validTeam(s.team)&&s.weaponPart&&s.activeWeapon;}
inline void* localPlayer(){std::lock_guard<std::mutex> l(ed::g_hookPlayerMutex);return ed::g_hookLocalPlayer;}
inline void capture(void* p){
 ed::PlayerSnapshot s{};if(!ed::readPlayerSnapshot(p,s)||s.localOwned||!s.statsValid||!s.hp||!ed::validTeam(s.team))return;
 Actor a{};a.player=p;a.world=object(p,offsets::field::mfc::kWorld);a.hp=s.hp;a.team=s.team;a.captured=Clock::now();if(!a.world)return;
 void* arr=object(part(p,"jgi"),offsets::field::jgi::kHitBoxColliders);std::uintptr_t count=0;if(!field(arr,0x18,count)||!count||count>64)return;
 esp::Vec3 root{};if(!ed::readPlayerPoseSnapshot(p,root))return;
 for(std::uintptr_t i=0;i<count&&a.count<a.volumes.size();++i){void* hb=object(arr,0x20+i*8);if(!named(hb,"HitBoxCollider"))continue;int bone=-1;if(!field(hb,0x20,bone)||bodyForBone(bone)<0)continue;void* c=object(hb,0x28);if(!named(c,"BoxCollider")&&!named(c,"CapsuleCollider")&&!named(c,"SphereCollider"))continue;auto ptr=native(c);std::uintptr_t vt=0,fn=0;if(!ptr||!ed::readValue(reinterpret_cast<void*>(ptr),vt)||!ed::readValue(reinterpret_cast<void*>(vt+0xf0),fn)||!unityCode(fn))continue;
  Bounds b{};g_bounds(ptr,&b);HitVolume v{b.center,b.extents,bone};if(!validVolume(v)||length(sub(v.center,{root.x,root.y,root.z}))>4)continue;a.volumes[a.count++]=v;}
 Actor* dst=nullptr;for(auto& slot:g_actors){if(slot.player==p){dst=&slot;break;}if(!dst&&(!slot.player||a.captured-slot.captured>std::chrono::seconds(1)))dst=&slot;}if(dst)*dst=a;
}
inline Selection select(const Settings& s,void* world,int team,Vec3 origin,Vec3 forward,std::uintptr_t& last){Selection best{};auto now=Clock::now();for(const auto& a:g_actors){if(!a.player||a.world!=world||a.hp<=0||a.team==team||now-a.captured>std::chrono::milliseconds(100))continue;std::uint16_t hp=0;auto stats=object(a.player,offsets::field::ake::kStats);if(ed::objectClass(a.player)!=ed::g_runtime.playerClass||object(a.player,offsets::field::mfc::kWorld)!=world||!field(stats,offsets::field::ylw::kHp,hp)||!hp)continue;for(unsigned i=0;i<a.count;++i)consider(best,s,reinterpret_cast<std::uintptr_t>(a.player),float(hp),origin,forward,a.volumes[i],last);}last=best.valid?best.player:0;return best;}
inline bool transformPose(void* transform,Vec3& origin,Vec3& forward){auto ptr=native(transform,"Transform");if(!ptr||!ed::g_runtime.transformPositionRotation)return false;esp::Vec3 pos{};esp::Quaternion q{};ed::g_runtime.transformPositionRotation(ptr,&pos,&q);origin={pos.x,pos.y,pos.z};float norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;if(!finite(origin)||!std::isfinite(norm)||std::fabs(norm-1)>.1f)return false;forward=normalized({2*(q.x*q.z+q.w*q.y),2*(q.y*q.z-q.w*q.x),1-2*(q.x*q.x+q.y*q.y)});return length(forward)>.9f;}
inline bool gate(const Configuration& c,const Settings& s,std::atomic<int>& status){if(!s.enabled){status.store(0);return false;}if(c.pauseInMenu&&g_menuOpen.load()){status.store(2);return false;}if(!s.bodies){status.store(5);return false;}return true;}
inline bool attacking(void* part){std::uint8_t raw=0;return field(part,offsets::field::nrk::kWeaponState,raw)&&raw==aim_offsets::kAttackState;}
inline bool stackSpan(Ray* rays,int count){static thread_local std::uintptr_t lo=0,hi=0;if(!lo){pthread_attr_t a;void* base=nullptr;std::size_t size=0;if(pthread_getattr_np(pthread_self(),&a))return false;int r=pthread_attr_getstack(&a,&base,&size);pthread_attr_destroy(&a);if(r)return false;lo=reinterpret_cast<std::uintptr_t>(base);hi=lo+size;}auto p=reinterpret_cast<std::uintptr_t>(rays);return count>0&&count<=64&&p>=lo&&p<=hi&&std::size_t(count)*sizeof(Ray)<=hi-p;}
inline void cameraTick(void* self,ExecuteTime time,const void* method){
 auto original=g_cameraOriginal.load(std::memory_order_acquire);if(original)original(self,time,method);if(!ed::isUnityMainCallback()||!g_functionsValid)return;auto c=configuration();auto s=c.normal;if(!gate(c,s,g_normalState))return;
 void* p=object(self,0x18);void* world=nullptr;ed::PlayerSnapshot local{};if(!localAlive(p,local,world)||object(self,0x20)!=world||!named(object(self,0x38),"eir")||!object(self,0x48)||!native(object(self,0x50),"Transform")||!native(object(self,0x58),"Transform")){g_normalState.store(3);return;}if(s.onAttack&&!attacking(local.weaponPart)){g_normalState.store(4);return;}
 Vec3 origin{},forward{};if(!transformPose(object(local.weaponPart,offsets::field::nrk::kHitTransform),origin,forward)){g_normalState.store(3);return;}auto target=select(s,world,local.team,origin,forward,g_lastNormal);if(!target.valid){g_normalState.store(5);return;}auto next=smooth(anglesFor(forward),anglesFor(sub(target.point,origin)),s.smoothing,time.delta);next=g_clamp(self,next,nullptr);if(!std::isfinite(next.pitch)||!std::isfinite(next.yaw))return;g_apply(self,next,nullptr);g_refresh(self,nullptr);g_normalApplications.fetch_add(1);g_normalState.store(6);
}
inline void emitRays(void* self,void* ctx,Ray* rays,std::uint64_t span){
 auto c=configuration();auto s=c.silent;int count=static_cast<std::int32_t>(span);
 if(ed::isUnityMainCallback()&&g_functionsValid&&rays&&stackSpan(rays,count)&&gate(c,s,g_silentState)){
  void* p=localPlayer();void* world=nullptr;ed::PlayerSnapshot local{};std::uint64_t owner=0,id=0;
  if(localAlive(p,local,world)&&object(self,0x18)==world&&field(ctx,0,owner)&&field(p,0x10,id)&&owner==id&&object(ctx,0x48)==local.activeWeapon){
   Vec3 origin{},forward{};Ray ray{};if(ed::readValue(rays,ray)&&transformPose(object(ctx,0x10),origin,forward)){auto target=select(s,world,local.team,ray.origin,forward,g_lastSilent);if(target.valid){unsigned changed=0;for(int i=0;i<count;++i){Ray r{};if(!ed::readValue(rays+i,r)||!finite(r.origin))continue;auto direction=normalized(sub(target.point,r.origin));if(length(direction)<.9f)continue;std::memcpy(&rays[i].direction,&direction,sizeof(direction));++changed;}g_silentApplications.fetch_add(changed);g_silentState.store(changed?6:7);}else g_silentState.store(5);}else g_silentState.store(3);
  }else g_silentState.store(3);
 }
 // Forward ORIGINAL exactly once. No camera/transform writes in this path.
 if(g_emitOriginal)g_emitOriginal(self,ctx,rays,span);
}
inline bool bytesMatch(std::uintptr_t addr,std::initializer_list<std::uint32_t> words){unsigned i=0;for(auto expected:words){std::uint32_t actual=0;if(!ed::readValue(reinterpret_cast<void*>(addr+4*i++),actual)||actual!=expected)return false;}return true;}
inline bool validateFunctions(){auto b=ed::g_runtime.unityBase;if(!b)return false;bool ok=bytesMatch(b+aim_offsets::kCameraApply,{0xd10103ff,0xfd000bea})&&bytesMatch(b+aim_offsets::kCameraClamp,{0x6dbb33ed,0x6d012beb})&&bytesMatch(b+aim_offsets::kCameraTick,{0xa9bf4ffe,0xaa0003f3})&&bytesMatch(b+aim_offsets::kCameraRefresh,{0xd100c3ff,0xa900fbff})&&bytesMatch(b+aim_offsets::kShotEmit,{0xd10183ff,0xa9035ffe,0xa90457f6,0xa9054ff4})&&bytesMatch(b+offsets::unity::kColliderGetBounds,{0xd100c3ff,0xa9024ffe});if(!ok)return false;g_bounds=reinterpret_cast<BoundsFn>(b+offsets::unity::kColliderGetBounds);g_clamp=reinterpret_cast<ClampFn>(b+aim_offsets::kCameraClamp);g_apply=reinterpret_cast<ApplyFn>(b+aim_offsets::kCameraApply);g_refresh=reinterpret_cast<RefreshFn>(b+aim_offsets::kCameraRefresh);g_emitOriginal=reinterpret_cast<EmitFn>(b+aim_offsets::kShotEmit);return true;}
inline bool installCamera(){
#if defined(__aarch64__)
 if(g_normalHookReady.load())return true;void* klass=ed::classFromSlot(offsets::typeinfo::kCameraComponent,"ios");if(!klass)return false;auto target=ed::g_runtime.unityBase+aim_offsets::kCameraTick;
 for(unsigned i=0;i<128;++i){auto slot=reinterpret_cast<std::uintptr_t*>(reinterpret_cast<std::uintptr_t>(klass)+ed::kRuntimeVtableOffset+i*ed::kVirtualInvokeDataSize);std::uintptr_t p=0,info=0;if(!ed::readValue(slot,p)||p!=target||!ed::readValue(slot+1,info)||!info)continue;std::uintptr_t direct=0,virt=0;if(!ed::readValue(reinterpret_cast<void*>(info),direct)||!ed::readValue(reinterpret_cast<void*>(info+8),virt)||(direct!=target&&virt!=target))continue;std::uint64_t probe=p;iovec src{&probe,8},dst{slot,8};if(syscall(__NR_process_vm_writev,getpid(),&src,1,&dst,1,0)!=8)return false;g_cameraOriginal.store(reinterpret_cast<CameraTickFn>(p),std::memory_order_release);__atomic_store_n(slot,reinterpret_cast<std::uintptr_t>(&cameraTick),__ATOMIC_RELEASE);g_normalHookReady.store(true);__android_log_print(ANDROID_LOG_INFO,esp::kLogTag,"AIM camera hook ready");return true;}
#endif
 return false;
}
inline bool installSilent(){
#if defined(__aarch64__)
 if(g_silentHookReady.load())return true;auto site=ed::g_runtime.unityBase+aim_offsets::kShotCallsite;std::uint32_t word=0;if(!ed::readValue(reinterpret_cast<void*>(site),word)||word!=0x97fffd71)return false;auto pageSize=std::uintptr_t(sysconf(_SC_PAGESIZE));if(!pageSize)return false;void* relay=MAP_FAILED;
 // Non-fixed hints cannot replace live mappings; relay must fit BL +/-128 MiB.
 for(unsigned i=1;i<=64&&relay==MAP_FAILED;++i)for(int sign:{1,-1}){auto hint=(site+std::intptr_t(sign)*std::intptr_t(i)*0x100000)&~(pageSize-1);void* p=mmap(reinterpret_cast<void*>(hint),pageSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);if(p==MAP_FAILED)continue;auto delta=std::intptr_t(reinterpret_cast<std::uintptr_t>(p))-std::intptr_t(site);if(delta>=-0x8000000&&delta<0x8000000){relay=p;break;}munmap(p,pageSize);}
 if(relay==MAP_FAILED)return false;std::uint32_t instructions[2]={0x58000051,0xd61f0220};auto replacement=reinterpret_cast<std::uintptr_t>(&emitRays);std::memcpy(relay,instructions,8);std::memcpy(static_cast<char*>(relay)+8,&replacement,8);__builtin___clear_cache(static_cast<char*>(relay),static_cast<char*>(relay)+16);if(mprotect(relay,pageSize,PROT_READ|PROT_EXEC)){munmap(relay,pageSize);return false;}auto page=site&~(pageSize-1);if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_WRITE|PROT_EXEC)){munmap(relay,pageSize);return false;}auto delta=std::intptr_t(reinterpret_cast<std::uintptr_t>(relay))-std::intptr_t(site);auto branch=0x94000000u|(std::uint32_t(delta/4)&0x03ffffff);__atomic_store_n(reinterpret_cast<std::uint32_t*>(site),branch,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC)){__atomic_store_n(reinterpret_cast<std::uint32_t*>(site),word,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC);return false;}g_relay=relay;g_silentHookReady.store(true);__android_log_print(ANDROID_LOG_INFO,esp::kLogTag,"AIM silent shot call-site ready");return true;
#endif
 return false;
}
inline void onPlayer(void* p){if(!g_active.load(std::memory_order_acquire)||!ed::g_runtime.ready||!ed::isUnityMainCallback())return;if(!g_functionsValid){g_functionsValid=validateFunctions();if(!g_functionsValid){g_normalState.store(7);g_silentState.store(7);return;}}auto now=Clock::now();auto c=configuration();if(now-g_lastInstall>std::chrono::seconds(1)){g_lastInstall=now;if(c.normal.enabled&&!installCamera())g_normalState.store(1);if(c.silent.enabled&&!installSilent())g_silentState.store(1);}capture(p);}
}
