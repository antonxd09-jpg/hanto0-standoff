#pragma once
// ASTRA_NIGHT_MATERIAL_DIAG_V1 - read-only scene inventory.
#include "features/visuals/worldcolor_runtime.hpp"
#include "imgui.h"
#include "features/visuals/astra_night.hpp"
#include <cmath>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <cstdio>
namespace astra_night_diag {
namespace ed=astra::esp::detail;
namespace wc=astra::worldcolor::detail;
inline std::atomic<bool> requested{false};
inline std::mutex mutex;
inline std::vector<std::string> output;
inline int renderers=0,materials=0;
using Obj=void*(*)(void*,const void*);
using Find=void*(*)(void*,const void*);
inline Obj getName=nullptr,getTransform=nullptr,getParent=nullptr;
inline bool resolve(){
 if(getName&&getTransform&&getParent)return true;
 if(!astra::il2cpp_meta::init(ed::g_runtime.unityBase))return false;
 auto cls=[](const char* n){return astra::il2cpp_meta::unityClass("UnityEngine",n);};
 auto method=[&](const char* c,const char* n){void* k=cls(c);return k?astra::il2cpp_meta::method(k,n,0,nullptr):nullptr;};
 getName=reinterpret_cast<Obj>(method("Object","get_name"));
 getTransform=reinterpret_cast<Obj>(method("Component","get_transform"));
 getParent=reinterpret_cast<Obj>(method("Transform","get_parent"));
 return getName&&getTransform&&getParent;
}
inline std::string name(void* o){
 if(!wc::liveUnityObject(o))return "<dead>";
 void* s=getName(o,nullptr);if(!s)return "<unnamed>";
 // il2cpp_string_chars returns UTF-16; read the length from the ARM64 managed string header.
 int n=0;if(!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(s)+0x10),n)||n<0||n>4096)return "<string-layout?>";
 using Chars=const uint16_t*(*)(void*);
 auto chars=wc::resolve<Chars>(astra::offsets::il2cpp_api::kStringChars);
 if(!chars)return "<no-string-api>";
 const auto* p=chars(s);std::string out;
 for(int i=0;i<n&&i<160;++i){uint16_t ch=0;if(!ed::readValue(p+i,ch))break;
  if(ch<128)out+=char(ch>=32?ch:' ');
  else if(ch<2048){out+=char(0xC0|(ch>>6));out+=char(0x80|(ch&63));}
  else if(ch>=0xD800&&ch<=0xDFFF)out+='?';
  else{out+=char(0xE0|(ch>>12));out+=char(0x80|((ch>>6)&63));out+=char(0x80|(ch&63));}
 }return out;
}
inline uintptr_t count(void* arr,uintptr_t cap){uintptr_t n=0;if(!arr||!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(arr)+0x18),n)||n>cap)return 0;return n;}
inline void* item(void* arr,uintptr_t i){void* o=nullptr;ed::readValue(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(arr)+0x20+8*i),o);return o;}

// PYRAMID_NIGHT_TEST_V1
// Experimental original-shader tint. No shader replacement, no global material edits.
namespace pyramid_test {
// PYRAMID_MPB_V2: original-shader experiment. Main-thread operations only.
using Color=astra::worldcolor::Color4;
using Handle=uintptr_t;
inline std::atomic<int> command{0};
inline std::atomic<int> state{0},changed{0},skipped{0};
inline std::mutex logMutex;
inline std::vector<std::string> logLines;
inline void log(const std::string& s){
 std::lock_guard<std::mutex> l(logMutex);
 if(logLines.size()<2048)logLines.push_back(s);
 else if(logLines.size()==2048)logLines.push_back("Log truncated at 2048 entries");
}
template<class T> inline T fn(uintptr_t r){return wc::resolve<T>(r);}
using Root=Handle(*)(void*,Handle,int,const void*);
using Target=void*(*)(Handle,const void*);
using Free=void(*)(Handle,const void*);
using BlockCall=void(*)(void*,void*,const void*);
using VoidCall=void(*)(void*,const void*);
using Empty=bool(*)(void*,const void*);
using Alloc=void*(*)(void*);
using Count=int(*)(void*,const void*);
using Index=int(*)(void*,int,const void*);
using Name=void*(*)(void*,int,const void*);
using ReadColor=Color(*)(void*,int,const void*);
using WriteColor=void(*)(void*,int,Color,const void*);
inline Root root=nullptr;inline Target target=nullptr;inline Free freeHandle=nullptr;
inline BlockCall getBlock=nullptr,setBlock=nullptr;
inline VoidCall ctor=nullptr,dispose=nullptr;
inline Empty empty=nullptr;inline Alloc alloc=nullptr;
inline Count propertyCount=nullptr;inline Index propertyType=nullptr,propertyId=nullptr;
inline Name propertyName=nullptr;
inline ReadColor materialColor=nullptr,blockColor=nullptr;
inline WriteColor setColor=nullptr;
inline void* blockClass=nullptr;
struct Saved {Handle renderer=0,block=0;int property=0;Color applied{};};
inline std::vector<Saved> saved;
inline std::string str(void* s){
 if(!s)return "<null>";int n=0;
 if(!ed::readValue(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(s)+0x10),n)||n<0||n>512)return "<invalid-string>";
 using Chars=const uint16_t*(*)(void*);
 auto chars=fn<Chars>(astra::offsets::il2cpp_api::kStringChars);if(!chars)return "<no-chars>";
 const auto* data=chars(s);std::string result;
 for(int i=0;i<n;++i){uint16_t c=0;if(!ed::readValue(data+i,c))return "<unreadable>";result+=char(c<128?c:'?');}return result;
}
inline bool close(Color a,Color b){return std::isfinite(a.r)&&std::isfinite(a.g)&&std::isfinite(a.b)&&std::isfinite(a.a)&&std::fabs(a.r-b.r)<.002f&&std::fabs(a.g-b.g)<.002f&&std::fabs(a.b-b.b)<.002f&&std::fabs(a.a-b.a)<.002f;}
inline bool api(){
 if(!resolve()){log("Base name/transform API unavailable");return false;}
 root=fn<Root>(0x6a041a8);target=fn<Target>(0x6a089b4);freeHandle=fn<Free>(0x6a025d0);
 getBlock=fn<BlockCall>(0x72fe550);setBlock=fn<BlockCall>(0x72de4b8);
 ctor=fn<VoidCall>(0x7305bb0);dispose=fn<VoidCall>(0x72dce70);empty=fn<Empty>(0x72f536c);
 alloc=fn<Alloc>(astra::offsets::il2cpp_api::kObjectNew);
 propertyCount=fn<Count>(0x72d585c);propertyName=fn<Name>(0x72f8468);
 propertyType=fn<Index>(0x72db4cc);propertyId=fn<Index>(0x72db358);
 materialColor=fn<ReadColor>(0x72e7478);blockColor=fn<ReadColor>(0x72fe3ec);setColor=fn<WriteColor>(0x72e5738);
 blockClass=astra::il2cpp_meta::unityClass("UnityEngine","MaterialPropertyBlock");
 bool ok=root&&target&&freeHandle&&getBlock&&setBlock&&ctor&&dispose&&empty&&alloc&&propertyCount&&propertyName&&propertyType&&propertyId&&materialColor&&blockColor&&setColor&&blockClass;
 log(ok?"API resolved by profile RVAs; runtime ABI still experimental":"Required API/class missing");return ok;
}
inline void release(Saved& s){void* block=target(s.block,nullptr);if(block)dispose(block,nullptr);if(s.block)freeHandle(s.block,nullptr);if(s.renderer)freeHandle(s.renderer,nullptr);s={};}
inline void restore(){
 int conflicts=0;
 for(auto& s:saved){void* renderer=target(s.renderer,nullptr);void* block=target(s.block,nullptr);
  if(wc::liveUnityObject(renderer)){
   if(!block){++conflicts;continue;}
   getBlock(renderer,block,nullptr);
   if(!empty(block,nullptr)&&!close(blockColor(block,s.property,nullptr),s.applied)){
    log("Restore conflict: block changed by game; not overwriting it");++conflicts;continue;
   }
   // Only originally-empty renderer blocks are ever modified by this test.
   setBlock(renderer,nullptr,nullptr);
  }
  release(s);
 }
 saved.erase(std::remove_if(saved.begin(),saved.end(),[](const Saved& s){return !s.renderer;}),saved.end());
 changed.store(int(saved.size()));state.store(conflicts?6:0);
 log(conflicts?"Restore pending: restart game if conflict persists":"Restored original empty blocks; managed roots released");
}

// NIGHT_SANDYARDS_V6

// NIGHT_AUTO_V9
inline std::string lowerName(std::string value){
 for(char& c:value)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return value;
}
inline bool environmentRoot(const std::string& n){
 // Structural heuristic, not a claim that every game map uses these names.
 return n=="map"||n=="environment"||n=="geometry"||n=="levelgeometry"||
  (n.size()>5&&n.compare(n.size()-5,5,"_mesh")==0)||
  (n.size()>7&&n.compare(n.size()-7,7,"_meshes")==0);
}

// NIGHT_MULTIMAP_SELECTION_V1

// NIGHT_GPU_COVERAGE_V2
inline bool selected(void* r){
 auto reject=[&](const std::string& why){
  log("FILTER renderer="+name(r)+" reason="+why);return false;
 };
 if(!wc::liveUnityObject(r))return reject("dead");
 void* t=getTransform(r,nullptr);
 if(!wc::liveUnityObject(t))return reject("transform unavailable");
 for(int depth=0;depth<64;++depth){
  if(!wc::liveUnityObject(t))return reject("invalid ancestor");
  const auto n=lowerName(name(t));
  // Mesh decals are allowed. Dynamic actors and effects remain excluded.
  for(const char* banned:{
   "collider","occlusion","player","ragdoll","weapon","viewmodel",
   "bullet","particle","effect","skybox","tracer","projectile",
   "character","avatar","firstperson","first_person","hands",
   "muzzle","canvas","hud","interface"})
   if(n.find(banned)!=std::string::npos)
    return reject("ancestor="+n+" token="+banned);
  void* parent=getParent(t,nullptr);
  if(!parent)return true;
  if(parent==t)return reject("parent cycle");
  t=parent;
 }
 return reject("hierarchy depth exceeded");
}

inline void logMaterialCoverage(void* mat){
 if(!wc::liveUnityObject(mat)){log("MATERIAL dead");return;}
 void* shader=wc::g_fn.getShader(mat,nullptr);
 log("MATERIAL "+name(mat)+" SHADER "+name(shader));
 if(!wc::liveUnityObject(shader))return;
 int n=propertyCount(shader,nullptr);
 if(n<0||n>1024){log("PROPERTY invalid count="+std::to_string(n));return;}
 for(int i=0;i<n;++i){
  const int type=propertyType(shader,i,nullptr);
  if(type!=0&&type!=4)continue;
  void* key=propertyName(shader,i,nullptr);
  const auto label=str(key);
  log("PROPERTY "+label+" type="+std::to_string(type));
  // Diagnostic only: never recolor normal/mask textures indiscriminately.
  if(type==4&&key&&wc::g_fn.getTexture){
   void* tex=wc::g_fn.getTexture(mat,key,nullptr);
   log("TEXTURE "+label+" object="+name(tex)+
       " main="+std::to_string(tex==wc::g_fn.getMainTexture(mat,nullptr)));
  }
 }
}

inline void unusedMpbRun(){
 int action=command.exchange(0);if(!action)return;
 {std::lock_guard<std::mutex> l(logMutex);logLines.clear();}
 if(!wc::initialize()||!api()){state.store(2);return;}
 restore();if(!saved.empty()||action==2)return;
 if(astra::worldcolor::g_settings.enabled){state.store(3);log("Disable native Night and World Color");return;}
 state.store(1);skipped.store(0);changed.store(0);
 auto find=fn<Find>(0x694117c);
 void* type=wc::g_fn.typeGetType(wc::makeString("UnityEngine.MeshRenderer, UnityEngine.CoreModule"),nullptr);
 void* arr=type&&find?find(type,nullptr):nullptr;
 if(!arr){state.store(2);log("Scene array unavailable");return;}
 Handle arrayRoot=root(arr,0,2,nullptr); // GCHandleType.Normal = 2, pointer-sized token
 if(!arrayRoot){state.store(2);log("Scene array root allocation failed");return;}
 int found=0;
 for(uintptr_t i=0;i<count(arr,100000);++i){
  void* renderer=item(arr,i);if(!wc::liveUnityObject(renderer)||!selected(renderer))continue;
  ++found;log("Renderer: "+name(renderer));
  void* mats=wc::g_fn.getSharedMaterials(renderer,nullptr);uintptr_t n=count(mats,64);
  Handle matsRoot=mats?root(mats,0,2,nullptr):0;
  if(!n||!matsRoot){if(matsRoot)freeHandle(matsRoot,nullptr);log("SKIP: no material array/root");skipped.fetch_add(1);continue;}
  int chosen=0;bool first=true,supported=true;Color original{};
  for(uintptr_t j=0;j<n;++j){void* mat=item(mats,j);
   if(!wc::liveUnityObject(mat)){supported=false;continue;}
   void* shader=wc::g_fn.getShader(mat,nullptr);if(!wc::liveUnityObject(shader)){supported=false;continue;}
   log("Material: "+name(mat)+" | Shader: "+name(shader));
   int pc=propertyCount(shader,nullptr),id=0;bool has=false;
   if(pc<0||pc>1024){supported=false;log("Invalid property count");continue;}
   log("Property count: "+std::to_string(pc));
   for(int k=0;k<pc;++k){std::string key=str(propertyName(shader,k,nullptr));int t=propertyType(shader,k,nullptr);
    log("  "+key+" | type="+std::to_string(t));
    if(t==0&&(key=="_Color"||key=="_BaseColor")){id=propertyId(shader,k,nullptr);has=true;}
   }
   if(!has){supported=false;log("SKIP: no declared _Color/_BaseColor Color property");continue;}
   Color c=materialColor(mat,id,nullptr);
   if(!close(c,c)){supported=false;log("SKIP: invalid original color");continue;}
   if(first){chosen=id;original=c;first=false;}
   else if(chosen!=id||!close(original,c)){supported=false;log("SKIP: slots need different colors; per-slot API not available here");}
  }
  freeHandle(matsRoot,nullptr);
  if(!supported||first){skipped.fetch_add(1);continue;}
  Saved record;record.renderer=root(renderer,0,2,nullptr);
  if(!record.renderer){log("SKIP: renderer root failed");skipped.fetch_add(1);continue;}
  void* block=alloc(blockClass);record.block=block?root(block,0,2,nullptr):0;
  if(!record.block){freeHandle(record.renderer,nullptr);log("SKIP: block allocation/root failed");skipped.fetch_add(1);continue;}
  ctor(block,nullptr);getBlock(renderer,block,nullptr);
  if(!empty(block,nullptr)){log("SKIP: existing renderer PropertyBlock; preserve game data");release(record);skipped.fetch_add(1);continue;}
  record.property=chosen;record.applied=original;
  record.applied.r*=.38f;record.applied.g*=.40f;record.applied.b*=.45f;
  setColor(block,chosen,record.applied,nullptr);setBlock(renderer,block,nullptr);
  getBlock(renderer,block,nullptr);
  if(!close(blockColor(block,chosen,nullptr),record.applied)){
   setBlock(renderer,nullptr,nullptr);release(record);skipped.fetch_add(1);log("SKIP: readback mismatch; restored");continue;
  }
  saved.push_back(record);changed.fetch_add(1);log("Block readback accepted; visual change not yet confirmed");
 }
 freeHandle(arrayRoot,nullptr);
 state.store(!found?4:changed.load()?5:7);
 log("Selected="+std::to_string(found)+" Applied="+std::to_string(changed.load())+" Skipped="+std::to_string(skipped.load()));
}

// PYRAMID_TEXTURE_V3 - readable Texture2D only; originals never modified.
struct TextureEntry {Handle renderer=0,originals=0,assigned=0;std::vector<Handle> owned;};
inline std::vector<TextureEntry> textureSaved;
// NIGHT_CONTROLS_V5
inline std::mutex nightSettingsMutex;
struct NightSettings {float strength=.78f;float tint[3]={.72f,.80f,1.f};};
inline NightSettings nightRequested;
inline std::atomic<bool> nightBusy{false};
inline void requestNight(int action,const NightSettings& settings){
 if(nightBusy.exchange(true))return;
 {std::lock_guard<std::mutex> lock(nightSettingsMutex);nightRequested=settings;}
 command.store(action);requested.store(true);
}
struct NightBusyGuard {~NightBusyGuard(){nightBusy.store(false);}};

using ObjectFn=void*(*)(void*,const void*);
using ArraySet=void(*)(void*,void*,int,const void*);
using SetTexture=void(*)(void*,void*,const void*);
using Readable=bool(*)(void*,const void*);
using Apply=void(*)(void*,bool,bool,const void*);
inline bool sameArray(void* a,void* b){
 if(!a||!b)return false;uintptr_t n=count(a,64);if(n!=count(b,64))return false;
 for(uintptr_t i=0;i<n;++i)if(item(a,i)!=item(b,i))return false;return true;
}
inline void cleanup(TextureEntry& e){
 auto destroy=fn<VoidCall>(0x6951a28);
 for(auto h:e.owned){void* o=target(h,nullptr);if(wc::liveUnityObject(o))destroy(o,nullptr);freeHandle(h,nullptr);}
 for(auto h:{e.renderer,e.originals,e.assigned})if(h)freeHandle(h,nullptr);
 e={};
}

// PYRAMID_GPU_V4: GPU readback; only call from Unity main callback.
struct ReadRect {float x,y,w,h;};
struct GpuReadScope {
 void* rt=nullptr;void* previous=nullptr;Handle previousRoot=0;
 using SetActive=void(*)(void*,const void*);
 using Release=void(*)(void*,const void*);
 ~GpuReadScope(){
  fn<SetActive>(0x72ad628)(previous,nullptr);
  if(rt)fn<Release>(0x72a7c9c)(rt,nullptr);
  if(previousRoot)freeHandle(previousRoot,nullptr);
 }
};
inline bool gpuRead(void* source,void* destination,int w,int h){
 using GetTemp=void*(*)(int,int,int,int,int,const void*);
 using Active=void*(*)(const void*);
 using Blit=void(*)(void*,void*,const void*);
 using Read=void(*)(void*,ReadRect,int,int,bool,const void*);
 auto getTemp=fn<GetTemp>(0x72a9444);
 auto active=fn<Active>(0x72b7828);
 auto setActive=fn<GpuReadScope::SetActive>(0x72ad628);
 auto release=fn<GpuReadScope::Release>(0x72a7c9c);
 auto blit=fn<Blit>(0x72e9074);
 auto read=fn<Read>(0x72b5fdc);
 if(!getTemp||!active||!setActive||!release||!blit||!read){log("GPU API unavailable");return false;}
 GpuReadScope scope;scope.previous=active(nullptr);
 if(scope.previous){scope.previousRoot=root(scope.previous,0,2,nullptr);if(!scope.previousRoot){log("Previous render target root failed");return false;}}
 // RenderTextureFormat.ARGB32=0; RenderTextureReadWrite.sRGB=2.
 // Five-int overload: width,height,depthBuffer,format,readWrite.
 scope.rt=getTemp(w,h,0,0,2,nullptr);
 if(!wc::liveUnityObject(scope.rt)){scope.rt=nullptr;log("Temporary render target unavailable");return false;}
 blit(source,scope.rt,nullptr);setActive(scope.rt,nullptr);
 read(destination,ReadRect{0,0,float(w),float(h)},0,0,false,nullptr);
 log("GPU ReadPixels returned: "+std::to_string(w)+"x"+std::to_string(h));
 return true;
}


// NIGHT_PIXEL_RANGE_FIX_V1
inline bool pixelBufferReadWrite(const void* pointer, size_t bytes){
 if(!ed::plausibleAddress(pointer,bytes))return false;
 uintptr_t cursor=reinterpret_cast<uintptr_t>(pointer);
 const uintptr_t end=cursor+bytes;
 FILE* f=std::fopen("/proc/self/maps","r");
 if(!f)return false;
 char line[768];
 while(cursor<end && std::fgets(line,sizeof(line),f)){
  unsigned long long lo=0,hi=0;char perms[5]{};
  if(std::sscanf(line,"%llx-%llx %4s",&lo,&hi,perms)!=3)continue;
  if(hi<=cursor)continue;
  if(lo>cursor || perms[0]!='r' || perms[1]!='w')break;
  cursor=std::min(end,static_cast<uintptr_t>(hi));
 }
 std::fclose(f);
 return cursor==end;
}

inline void restoreTextures(){
 auto assign=fn<SetTexture>(0x72ff268);
 for(auto& e:textureSaved){void* renderer=target(e.renderer,nullptr);
  if(wc::liveUnityObject(renderer)){
   void* old=target(e.originals,nullptr);void* current=wc::g_fn.getSharedMaterials(renderer,nullptr);
   if(!sameArray(current,target(e.assigned,nullptr))){log("Restore conflict: material slots changed by game; retained copies");continue;}
   assign(renderer,old,nullptr);
   if(!sameArray(wc::g_fn.getSharedMaterials(renderer,nullptr),old)){log("Restore readback failed; retained copies");continue;}
  }
  cleanup(e);
 }
 textureSaved.erase(std::remove_if(textureSaved.begin(),textureSaved.end(),[](const TextureEntry& e){return !e.renderer;}),textureSaved.end());
 state.store(textureSaved.empty()?0:6);changed.store(int(textureSaved.size()));
}
inline void run(){
 int action=command.exchange(0);if(!action)return;
 NightBusyGuard busyGuard;
 NightSettings settings;
 {std::lock_guard<std::mutex> lock(nightSettingsMutex);settings=nightRequested;}
 const float strength=std::clamp(settings.strength,0.f,.97f);
 float gain[3];
 for(int channel=0;channel<3;++channel){
  const float tint=std::clamp(settings.tint[channel],0.f,1.f);
  gain[channel]=(1.f-strength)*((1.f-strength)+strength*tint);
 }

 {std::lock_guard<std::mutex> l(logMutex);logLines.clear();}
 if(!wc::initialize()||!api()){state.store(2);return;}
 if(!saved.empty()){restore();if(!saved.empty())return;}
 restoreTextures();if(!textureSaved.empty()||action==2)return;
 if(astra::worldcolor::g_settings.enabled){state.store(3);return;}
 auto cloneArray=fn<ObjectFn>(0x680ac58);
 auto setArray=fn<ArraySet>(0x6805abc);
 auto instantiate=fn<ObjectFn>(0x6947414);
 auto readable=fn<Readable>(0x72b6cfc);
 auto getPixels=fn<ObjectFn>(0x72aa7fc);
 auto putPixels=fn<SetTexture>(0x72bb754);
 auto apply=fn<Apply>(0x72b243c);
 auto setTexture=fn<SetTexture>(0x72e1034);
 auto assign=fn<SetTexture>(0x72ff268);
 auto width=fn<Count>(0x72ae698),height=fn<Count>(0x72aa300);
 if(!cloneArray||!setArray||!instantiate||!readable||!getPixels||!putPixels||!apply||!setTexture||!assign||!width||!height){state.store(2);log("Texture/array API unresolved");return;}
 auto find=fn<Find>(0x694117c);
 void* type=wc::g_fn.typeGetType(wc::makeString("UnityEngine.MeshRenderer, UnityEngine.CoreModule"),nullptr);
 void* arr=type&&find?find(type,nullptr):nullptr;
 Handle ar=arr?root(arr,0,2,nullptr):0;if(!ar){state.store(2);return;}
 state.store(1);changed.store(0);skipped.store(0);int found=0;
 const auto operationStart=std::chrono::steady_clock::now();
 unsigned filtered=0;
 log("GPU COVERAGE V2: active MeshRenderers="+std::to_string(count(arr,100000)));
 log("Mesh decals allowed. Terrain/DecalProjector are not covered by this pass.");
 uint64_t applicationPixels=0;
 for(uintptr_t i=0;i<count(arr,100000);++i){
  void* renderer=item(arr,i);
  if(!selected(renderer)){++filtered;continue;}
  ++found;
  log("Renderer: "+name(renderer));
  TextureEntry e;e.renderer=root(renderer,0,2,nullptr);
  void* originals=wc::g_fn.getSharedMaterials(renderer,nullptr);
  e.originals=originals?root(originals,0,2,nullptr):0;
  uintptr_t n=count(originals,16);
  if(!e.renderer||!e.originals||!n){
   log("SKIP: renderer/material-array root or slot count rejected");
   cleanup(e);skipped.fetch_add(1);continue;
  }
  // Preflight all slots before allocating large textures. Each renderer is all-or-nothing.
  bool ok=true;uint64_t pixelsTotal=0;
  for(uintptr_t j=0;j<n;++j){void* mat=item(originals,j);
   log("SLOT "+std::to_string(j));
   logMaterialCoverage(mat);
   if(!wc::liveUnityObject(mat)){ok=false;continue;}
   void* shader=wc::g_fn.getShader(mat,nullptr);
   const auto shaderName=lowerName(name(shader));
   if(!wc::liveUnityObject(shader)||shaderName.find("particle")!=std::string::npos||shaderName.find("skybox")!=std::string::npos){
    log("SKIP: effect/sky shader");ok=false;continue;
   }
   void* tex=wc::g_fn.getMainTexture(mat,nullptr);
   if(!wc::liveUnityObject(tex)||!ed::nameEquals(ed::className(ed::objectClass(tex)),"Texture2D")){log("SKIP: non-Texture2D");ok=false;continue;}
   int w=width(tex,nullptr),h=height(tex,nullptr);bool can=readable(tex,nullptr);
   log(name(mat)+" texture="+name(tex)+" size="+std::to_string(w)+"x"+std::to_string(h)+" readable="+(can?"yes":"NO"));
   if(w<=0||h<=0||w>2048||h>2048){
 log("SKIP: texture dimensions outside supported 1..2048 range");
 ok=false;continue;
}
   pixelsTotal+=uint64_t(w)*uint64_t(h);
  }
  // Prevent an uncontrolled allocation spike in this synchronous test.
  if(pixelsTotal>24ull*1024*1024){log("SKIP: renderer exceeds 24M-pixel budget");ok=false;}
  if(applicationPixels+pixelsTotal>32ull*1024*1024){
   log("SKIP: application texture budget exceeded (32M pixels)");ok=false;
  }
  if(!ok){cleanup(e);skipped.fetch_add(1);continue;}
  applicationPixels+=pixelsTotal;
  void* assigned=cloneArray(originals,nullptr);e.assigned=assigned?root(assigned,0,2,nullptr):0;
  if(!e.assigned){cleanup(e);skipped.fetch_add(1);continue;}
  auto own=[&](void* obj){if(!obj)return false;Handle h=root(obj,0,2,nullptr);if(!h){fn<VoidCall>(0x6951a28)(obj,nullptr);return false;}e.owned.push_back(h);return true;};
  for(uintptr_t j=0;j<n&&ok;++j){void* mat=item(originals,j);void* source=wc::g_fn.getMainTexture(mat,nullptr);
   int w=width(source,nullptr),h=height(source,nullptr);
   void* textureClass=astra::il2cpp_meta::unityClass("UnityEngine","Texture2D");
   void* tex=textureClass?alloc(textureClass):nullptr;
   if(!tex){ok=false;log("Readback Texture2D allocation failed");break;}
   Handle readbackRoot=root(tex,0,2,nullptr);
   if(!readbackRoot){ok=false;log("Readback root failed");break;}
   using ReadbackCtor=void(*)(void*,int,int,int,bool,bool,const void*);
   fn<ReadbackCtor>(0x72b75a4)(tex,w,h,4,false,false,nullptr);
   if(!gpuRead(source,tex,w,h)){
    fn<VoidCall>(0x6951a28)(tex,nullptr);freeHandle(readbackRoot,nullptr);ok=false;break;
   }
   void* pixels=getPixels(tex,nullptr);Handle ph=pixels?root(pixels,0,2,nullptr):0;
   fn<VoidCall>(0x6951a28)(tex,nullptr);freeHandle(readbackRoot,nullptr);
   uintptr_t num=count(pixels,8ull*1024*1024);
   if(!ph||num!=uint64_t(w)*uint64_t(h)){if(ph)freeHandle(ph,nullptr);ok=false;log("Pixel array length/root failed");break;}
   void* cls=astra::il2cpp_meta::unityClass("UnityEngine","Texture2D");
   void* dark=cls?alloc(cls):nullptr;if(!own(dark)){freeHandle(ph,nullptr);ok=false;break;}
   using Ctor=void(*)(void*,int,int,int,bool,bool,const void*);
   fn<Ctor>(0x72b75a4)(dark,w,h,4,true,false,nullptr); // TextureFormat.RGBA32, mipmaps, sRGB
   auto* data=reinterpret_cast<unsigned char*>(reinterpret_cast<uintptr_t>(pixels)+0x20);
   if(!pixelBufferReadWrite(data,num*4)){
 log("SKIP: fresh pixel range is not fully readable/writable");
 freeHandle(ph,nullptr);ok=false;break;
}
log("Pixel buffer RW verified: "+std::to_string(num*4)+" bytes");
   // Always derive from original atlases, never from previously tinted copies.
   // RGB byte modulation is artistic sRGB-space tint, not physical exposure.
   unsigned char lut[3][256];
   for(int channel=0;channel<3;++channel)
    for(int value=0;value<256;++value)
     lut[channel][value]=static_cast<unsigned char>(std::clamp(value*gain[channel]+.5f,0.f,255.f));
   for(uintptr_t k=0;k<num;++k){
    data[k*4]=lut[0][data[k*4]];
    data[k*4+1]=lut[1][data[k*4+1]];
    data[k*4+2]=lut[2][data[k*4+2]];
   }
   log("RGB gain="+std::to_string(gain[0])+","+std::to_string(gain[1])+","+std::to_string(gain[2])+" alpha=original");
   putPixels(dark,pixels,nullptr);apply(dark,true,true,nullptr);freeHandle(ph,nullptr);
   void* material=instantiate(mat,nullptr);if(!own(material)){ok=false;break;}
   setTexture(material,dark,nullptr);setArray(assigned,material,int(j),nullptr);
   if(item(assigned,j)!=material||wc::g_fn.getMainTexture(material,nullptr)!=dark){ok=false;log("Copy assignment readback failed");}
  }
  if(!ok){cleanup(e);skipped.fetch_add(1);continue;}
  assign(renderer,assigned,nullptr);
  if(!sameArray(wc::g_fn.getSharedMaterials(renderer,nullptr),assigned)){
   assign(renderer,originals,nullptr);
   if(!sameArray(wc::g_fn.getSharedMaterials(renderer,nullptr),originals)){textureSaved.push_back(e);state.store(6);log("Assignment/rollback conflict; retain copies, restart game");break;}
   cleanup(e);skipped.fetch_add(1);continue;
  }
  textureSaved.push_back(e);changed.fetch_add(1);log("Assigned dark copies; visual check required");
 }
 freeHandle(ar,nullptr);
 if(state.load()!=6)state.store(!found?4:changed.load()?5:7);
 log("SUMMARY selected="+std::to_string(found)+
     " filtered="+std::to_string(filtered)+
     " assigned="+std::to_string(changed.load())+
     " skipped="+std::to_string(skipped.load())+
     " chargedPixels="+std::to_string(applicationPixels)+
     " elapsedMs="+std::to_string(
       std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::steady_clock::now()-operationStart).count()));
}


inline void panel(){
 static NightSettings draft;
 static bool enabled=false,dirty=false;
 static bool useLighting=false;
 static float lightStrength=.35f;
 if(ImGui::Checkbox("Night mode",&enabled)){
  dirty=true;
  auto lighting=astra_night::configuration();
  lighting.enabled=enabled&&useLighting;lighting.strength=lightStrength;
  astra_night::publish(lighting);
 }
 if(ImGui::TreeNode("Night settings")){
  ImGui::SliderFloat("Darkness",&draft.strength,0.f,.97f,"%.2f");
  if(ImGui::IsItemDeactivatedAfterEdit())dirty=true;
  ImGui::ColorEdit3("Night color",draft.tint);
  if(ImGui::IsItemDeactivatedAfterEdit())dirty=true;
  if(ImGui::Button("Cold Night")){
   draft=NightSettings{};dirty=true;
  }
  bool lightEdited=ImGui::Checkbox("Lighting modulation",&useLighting);
  lightEdited=ImGui::SliderFloat("Lighting darkness",&lightStrength,0.f,1.f,"%.2f")||lightEdited;
  if(lightEdited){
   auto lighting=astra_night::configuration();
   lighting.enabled=enabled&&useLighting;lighting.strength=lightStrength;
   astra_night::publish(lighting);
  }
  ImGui::TextWrapped("Texture/color changes apply on edit release. Lighting also affects lit weapons and players. Sky unchanged.");
  ImGui::TreePop();
 }
 // Keep pending OFF requests until synchronous processing finishes.
 if(dirty&&!nightBusy.load()){
  if(enabled||changed.load()>0||state.load()==6)requestNight(enabled?1:2,draft);
  dirty=false;
 }
 if(nightBusy.load())ImGui::TextUnformatted("Night mode: applying...");
 else if(enabled&&state.load()==4)ImGui::TextUnformatted("Night mode: environment root not recognized");
 else if(state.load()==6)ImGui::TextUnformatted("Night mode: restore conflict");
 else if(enabled&&state.load()==2)ImGui::TextUnformatted("Night mode: API unavailable");
 else if(enabled&&state.load()==3)ImGui::TextUnformatted("Night mode: disable World Color");
 else if(enabled&&skipped.load()>0)ImGui::TextUnformatted("Night mode: some surfaces were skipped");
 else if(enabled&&state.load()==7)ImGui::TextUnformatted("Night mode: no compatible surfaces");
}
}

inline void tick(void* player){
 // NIGHT_LIGHTING_INTEGRATION_V7
 astra_night::tick(player);

 if(!requested.load()||!player||!ed::g_runtime.ready||!ed::isUnityMainCallback())return;
 void* local=nullptr;{std::lock_guard<std::mutex> l(ed::g_hookPlayerMutex);local=ed::g_hookLocalPlayer;}
 if(player!=local)return;
 if(!requested.exchange(false))return;
 if(pyramid_test::command.load()){pyramid_test::run();return;}
 std::vector<std::string> rows;int nr=0,nm=0;
 // Reuse read-only world material accessors; initialize resolves a shader but does not assign it.
 if(!wc::initialize()||!resolve())rows.push_back("Resolve failed: no scan performed.");
 else {
  auto find=wc::resolve<Find>(0x694117c); // Object.FindObjectsOfType(Type), active scene objects
  void* type=wc::g_fn.typeGetType(wc::makeString("UnityEngine.MeshRenderer, UnityEngine.CoreModule"),nullptr);
  void* arr=(type&&find)?find(type,nullptr):nullptr;
  const uintptr_t n=count(arr,100000);
  rows.push_back("Read-only active MeshRenderer inventory. No object is approved for tinting by this scan.");
  rows.push_back("Found="+std::to_string(n)+"; scan limit=1200; parent depth=10.");
  for(uintptr_t i=0;i<n&&i<1200;++i){
   void* r=item(arr,i);if(!wc::liveUnityObject(r))continue;++nr;
   void* tr=getTransform(r,nullptr);std::string path;
   for(int depth=0;depth<10&&wc::liveUnityObject(tr);++depth){path=name(tr)+(path.empty()?"":"/"+path);void* parent=getParent(tr,nullptr);if(parent==tr)break;tr=parent;}
   rows.push_back("OBJECT "+name(r)+" | PATH "+path);
   void* mats=wc::g_fn.getSharedMaterials(r,nullptr);uintptr_t m=count(mats,64);
   for(uintptr_t j=0;j<m;++j){void* mat=item(mats,j);if(!wc::liveUnityObject(mat))continue;
    ++nm;void* shader=wc::g_fn.getShader(mat,nullptr);
    std::string props;
    if(wc::g_fn.hasProperty)for(const char* prop:{"_Color","_BaseColor","_MainTex","_BaseMap","_EmissionColor"})
     if(wc::g_fn.hasProperty(mat,wc::makeString(prop),nullptr))props+=std::string(prop)+" ";
    char ptr[48];std::snprintf(ptr,sizeof(ptr),"%p",mat);
    rows.push_back("  MATERIAL "+name(mat)+" @"+ptr+" | SHADER "+name(shader)+" | PROPERTIES "+props);
   }
  }
  if(!arr)rows.push_back("No array returned; inventory unavailable.");
 }
 {std::lock_guard<std::mutex> l(mutex);output.swap(rows);renderers=nr;materials=nm;}
}

inline void saveRecoveryReport(){
 std::vector<std::string> rows;
 {std::lock_guard<std::mutex> lock(pyramid_test::logMutex);rows=pyramid_test::logLines;}
 pyramid_test::NightSettings settings;
 {std::lock_guard<std::mutex> lock(pyramid_test::nightSettingsMutex);settings=pyramid_test::nightRequested;}
 const char* path="/storage/emulated/0/Download/astra_gpu_night_report.txt";
 const char* tmp="/storage/emulated/0/Download/astra_gpu_night_report.txt.tmp";
 FILE* f=std::fopen(tmp,"wb");
 if(!f){pyramid_test::log("EXPORT FAILED: cannot open Download report");return;}
 std::fprintf(f,"ASTRA GPU NIGHT COVERAGE V2\n"
  "Not proof of visible rendering. Snapshot counters may change concurrently.\n"
  "state=%d busy=%d requested=%d command=%d changed=%d skipped=%d\n"
  "strength=%.4f tint=%.4f %.4f %.4f nativeLightingState=%d\n"
  "playerHook=%d playerCallbacks=%llu\n"
  "States: 0 restored/off, 1 applying, 2 API failure, 3 World Color active, "
  "4 environment not found, 5 copies assigned, 6 restore conflict, 7 no compatible surfaces\n\n",
  pyramid_test::state.load(),int(pyramid_test::nightBusy.load()),int(requested.load()),
  pyramid_test::command.load(),pyramid_test::changed.load(),pyramid_test::skipped.load(),
  double(settings.strength),double(settings.tint[0]),double(settings.tint[1]),double(settings.tint[2]),
  astra_night::status.load(),int(astra::esp::g_playerUpdateHookReady.load()),
  (unsigned long long)astra::esp::g_playerUpdateHookCalls.load());
 for(const auto& line:rows)std::fprintf(f,"%s\n",line.c_str());
 bool ok=!std::ferror(f);
 if(std::fclose(f)!=0)ok=false;
 if(ok)ok=std::rename(tmp,path)==0;
 if(!ok)std::remove(tmp);
 pyramid_test::log(ok?"EXPORT OK: Download/astra_gpu_night_report.txt":"EXPORT FAILED: write/rename");
}
inline void controls(){
 pyramid_test::panel();
 ImGui::Separator();
 ImGui::Text("GPU Night: state=%d changed=%d skipped=%d",
  pyramid_test::state.load(),pyramid_test::changed.load(),pyramid_test::skipped.load());
 ImGui::Text("Pending=%d | Busy=%d | Player hook=%d",
  int(requested.load()),int(pyramid_test::nightBusy.load()),
  int(astra::esp::g_playerUpdateHookReady.load()));
 if(ImGui::Button("Save Night diagnostics"))saveRecoveryReport();
 std::string last;
 {std::lock_guard<std::mutex> lock(pyramid_test::logMutex);
  if(!pyramid_test::logLines.empty())last=pyramid_test::logLines.back();}
 if(!last.empty())ImGui::TextWrapped("%s",last.c_str());
 ImGui::TextWrapped("Save after applying, before another apply/restore clears the operation log.");
}

}
