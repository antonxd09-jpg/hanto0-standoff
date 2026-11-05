// ASTRA_PERSISTENT_TEST_V4
#include <cstring>
#pragma once
#include "features/visuals/esp_runtime.hpp"
#include "imgui.h"
#include <string>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <cstdio>

namespace astra_chams_probe {
namespace ed=astra::esp::detail;
namespace off=astra::offsets;
using Clock=std::chrono::steady_clock;
inline std::atomic<bool> enabled{false};
// ASTRA_MATERIAL_PROBE_V2
inline std::atomic<bool> materialProbe{false};

inline std::atomic<unsigned long long> callbacks{0};
struct Row {
    uintptr_t player=0,world=0,model=0,bindings=0,renderer=0,native=0;
    uintptr_t arms=0,gloves=0,outline=0,untouchable=0;
    int materialStatus=0,materialCount=0,shaderCount=0;
    uintptr_t arrayResult=0,firstShader=0;
    int stage=0;
    unsigned char local=0;
    Clock::time_point when{};
};
inline std::mutex mutex;
inline std::array<Row,128> rows{};
// Safe reads only: no Unity methods or material setters.
inline uintptr_t ptr(uintptr_t base,size_t offset) {
    if(!base)return 0;
    return reinterpret_cast<uintptr_t>(ed::readObject(reinterpret_cast<void*>(base+offset)));
}

// ASTRA_SHADER_NAMES_V3
inline std::atomic<bool> inspectNames{false};
struct ShaderReport {uintptr_t address=0;std::string text;};
inline std::array<ShaderReport,32> shaderReports{};
inline std::mutex shaderMutex;
inline std::string shaderString(void* object) {
    if(!object)return "<null>";
    uintptr_t p=reinterpret_cast<uintptr_t>(object);
    int32_t n=0;
    if(!ed::readValue(reinterpret_cast<void*>(p+0x10),n) || n<0 || n>512)return "<invalid string>";
    std::array<uint16_t,512> data{};
    if(n && ed::safeReadMemory(data.data(),reinterpret_cast<void*>(p+0x14),size_t(n)*2)!=n*2)return "<read failed>";
    std::string out;
    for(int i=0;i<n;++i){
        uint32_t c=data[i];
        if(c>=0xD800 && c<=0xDBFF && i+1<n && data[i+1]>=0xDC00 && data[i+1]<=0xDFFF)
            c=0x10000+((c-0xD800)<<10)+(data[++i]-0xDC00);
        else if(c>=0xD800 && c<=0xDFFF)c=0xFFFD;
        if(c<32 || c==127)c=' ';
        if(c<128)out+=char(c);
        else if(c<2048){out+=char(0xC0|(c>>6));out+=char(0x80|(c&63));}
        else if(c<65536){out+=char(0xE0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
        else{out+=char(0xF0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
    }
    return out;
}
inline void inspectShader(uintptr_t shader) {
    if(!inspectNames.load())return;
    // One enumeration per shader address until manual cache reset.
    size_t slot=32;
    {
        std::lock_guard<std::mutex> lock(shaderMutex);
        for(size_t i=0;i<32;++i){
            if(shaderReports[i].address==shader)return;
            if(slot==32 && !shaderReports[i].address)slot=i;
        }
        if(slot==32)return;
        shaderReports[slot]={shader,"Reading..."};
    }
    const uintptr_t base=ed::g_runtime.unityBase;
    struct Check {uintptr_t rva;uint32_t first;};
    const Check checks[]={{0x6805E28,0x17FFBB17},{0x75F6F24,0xF81E0FFE},{0x7601EB8,0xF81E0FFE},{0x75E34B0,0xF81E0FFE},{0x75F3B24,0xF81E0FFE}};
    bool valid=true;
    for(auto c:checks){uint32_t word=0;if(!ed::readValue(reinterpret_cast<void*>(base+c.rva),word)||word!=c.first){valid=false;break;}}
    std::string text;
    if(!valid)text="Code mismatch; getters not called.";
    else {
        using Name=void*(*)(void*,const void*);
        using Count=int(*)(void*,const void*);
        using IntProperty=int(*)(void*,int,const void*);
        using PropertyName=void*(*)(void*,int,const void*);
        void* obj=reinterpret_cast<void*>(shader);
        text=shaderString(reinterpret_cast<Name>(base+0x6805E28)(obj,nullptr));
        int count=reinterpret_cast<Count>(base+0x75F6F24)(obj,nullptr);
        text+="\nProperties: "+std::to_string(count);
        if(count<0 || count>256)text+=" (rejected)";
        else {
            const int limit=std::min(count,64);
            for(int i=0;i<limit;++i){
                int type=reinterpret_cast<IntProperty>(base+0x75E34B0)(obj,i,nullptr);
                int id=reinterpret_cast<IntProperty>(base+0x7601EB8)(obj,i,nullptr);
                std::string name=shaderString(reinterpret_cast<PropertyName>(base+0x75F3B24)(obj,i,nullptr));
                text+="\n"+name+" | type="+std::to_string(type)+" | id="+std::to_string(id);
            }
            if(count>limit)text+="\nRemaining properties omitted (64 limit).";
        }
    }
    std::lock_guard<std::mutex> lock(shaderMutex);
    if(shaderReports[slot].address==shader)shaderReports[slot].text=std::move(text);
}


// ASTRA_DONOR_SEARCH_V1
inline std::atomic<bool> donorRequested{false};
inline std::mutex donorMutex;
inline std::string donorReport="Donor search not run.\n";
inline void donorSearch() {
    if(!donorRequested.exchange(false))return;
    const uintptr_t base=ed::g_runtime.unityBase;
    std::string result="DONOR SHADER SEARCH\n";
    struct Check {uintptr_t rva;uint32_t word;};
    const Check checks[]={{0x644A69C,0xA9BE57FE},{0x644A6A4,0x37F80362},
        {0x75FAF74,0xA9BE57FE},{0x651292C,0xAA1F03E1},
        {0x65017E0,0x15175DDD},{0x64FEBF4,0x151768BF}};
    for(auto c:checks){uint32_t v=0;
        if(!ed::readValue(reinterpret_cast<void*>(base+c.rva),v)||v!=c.word){
            std::lock_guard<std::mutex> lock(donorMutex);
            donorReport=result+"Code mismatch; search cancelled.\n";return;
        }
    }
    using StringCtor=void*(*)(const uint16_t*,int32_t,int32_t,const void*);
    using Alloc=uintptr_t(*)(void*,const void*);
    using Target=void*(*)(uintptr_t,const void*);
    using Free=void(*)(uintptr_t,const void*);
    using Find=void*(*)(void*,const void*);
    auto alloc=reinterpret_cast<Alloc>(base+0x651292C);
    auto target=reinterpret_cast<Target>(base+0x65017E0);
    auto freeHandle=reinterpret_cast<Free>(base+0x64FEBF4);
    const char* names[]={"Hidden/Internal-Colored","Legacy Shaders/Diffuse","Legacy Shaders/Transparent/Diffuse"};
    for(const char* name:names) {
        result+=std::string(name)+": ";
        std::array<uint16_t,128> chars{};
        const size_t n=std::strlen(name);
        for(size_t i=0;i<n;++i)chars[i]=static_cast<unsigned char>(name[i]);
        void* str=reinterpret_cast<StringCtor>(base+0x644A69C)(chars.data(),0,static_cast<int32_t>(n),nullptr);
        if(!str){result+="STRING FAILED\n";continue;}
        const uintptr_t stringHandle=alloc(str,nullptr);
        if(!stringHandle){result+="STRING HANDLE FAILED\n";continue;}
        str=target(stringHandle,nullptr);
        if(!str || shaderString(str)!=name){freeHandle(stringHandle,nullptr);result+="STRING VERIFICATION FAILED\n";continue;}
        void* shader=reinterpret_cast<Find>(base+0x75FAF74)(str,nullptr);
        // Root the returned shader before other managed calls.
        uintptr_t shaderHandle=shader?alloc(shader,nullptr):0;
        freeHandle(stringHandle,nullptr);
        if(!shader){result+="NOT FOUND\n";continue;}
        if(!shaderHandle){result+="SHADER HANDLE FAILED\n";continue;}
        shader=target(shaderHandle,nullptr);
        if(shader && ed::nameEquals(ed::className(ed::objectClass(shader)),"Shader") && ptr(reinterpret_cast<uintptr_t>(shader),0x10)) {
            result+="FOUND (not yet applied)\n";
            inspectShader(reinterpret_cast<uintptr_t>(shader));
        }else result+="INVALID SHADER RESULT\n";
        freeHandle(shaderHandle,nullptr);
    }
    std::lock_guard<std::mutex> lock(donorMutex);donorReport=std::move(result);
}
inline void donorControls() {
    if(ImGui::Button("Find donor shaders")) {
        enabled.store(true);inspectNames.store(true);
        {std::lock_guard<std::mutex> lock(donorMutex);donorReport="Waiting for UnityMain player callback...\n";}
        donorRequested.store(true);
    }
    std::string report;
    {std::lock_guard<std::mutex> lock(donorMutex);report=donorReport;}
    ImGui::TextWrapped("%s",report.c_str());
}

// ASTRA_SHADER_EXPORT_V4
inline void shaderExportControls() {
    donorControls();
    auto report=[]() {
        std::string out="ASTRA SHADER REPORT\n";
        {std::lock_guard<std::mutex> lock(donorMutex);out+=donorReport+"\n";}
        std::lock_guard<std::mutex> lock(shaderMutex);
        unsigned count=0;
        for(const auto& r:shaderReports) {
            if(!r.address)continue;
            out+="\n=== Shader "+std::to_string(++count)+" ===\n";
            out+=r.text+"\n";
        }
        if(!count)out+="No shader reports collected.\n";
        return out;
    };
    static std::string status;
    if(ImGui::Button("Copy all shader reports")) {
        const auto text=report();
        ImGui::SetClipboardText(text.c_str());
        status="Sent to ImGui clipboard; Android clipboard depends on backend.";
    }
    if(ImGui::Button("Save shader reports")) {
        const auto text=report();
        char package[256]{};
        if(FILE* f=std::fopen("/proc/self/cmdline","r")) {
            std::fread(package,1,sizeof(package)-1,f);
            std::fclose(f);
        }
        if(char* colon=std::strchr(package,':'))*colon=0;
        const std::string app(package);
        std::vector<std::string> paths{
            "/storage/emulated/0/Download/astra_shader_report.txt"
        };
        if(!app.empty() && app.find('/')==std::string::npos) {
            paths.push_back("/storage/emulated/0/Android/data/"+app+
                            "/files/astra_shader_report.txt");
            paths.push_back("/data/data/"+app+
                            "/files/astra_shader_report.txt");
        }
        status="Cannot write report to available locations.";
        for(const auto& path:paths) {
            FILE* f=std::fopen(path.c_str(),"wb");
            if(!f)continue;
            const size_t written=std::fwrite(text.data(),1,text.size(),f);
            const int closed=std::fclose(f);
            if(written==text.size() && closed==0) {
                status="Saved: "+path;
                break;
            }
        }
    }
    if(!status.empty())ImGui::TextWrapped("%s",status.c_str());
}

inline void shaderNamesControls() {
    shaderExportControls();
    bool on=inspectNames.load();
    if(ImGui::Checkbox("Read shader names / properties",&on))inspectNames.store(on);
    if(ImGui::Button("Clear shader report cache")) {
        std::lock_guard<std::mutex> lock(shaderMutex);shaderReports={};
    }
    ImGui::TextWrapped("Enable both material reading and name reading. Reports are cached; clear after a map change. No material writes.");
    if(ImGui::TreeNode("Shader reports")) {
        std::array<ShaderReport,32> copy;
        {std::lock_guard<std::mutex> lock(shaderMutex);copy=shaderReports;}
        for(size_t i=0;i<copy.size();++i)if(copy[i].address){
            ImGui::PushID(static_cast<int>(i));
            if(ImGui::TreeNode("Shader","Shader %zu",i+1)) {
                ImGui::TextWrapped("%s",copy[i].text.c_str());
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
}


// ASTRA_GC_TEST_V1
inline std::atomic<bool> gcTestRequested{false};
inline std::atomic<int> gcTestStatus{0};
inline std::atomic<uintptr_t> gcTestObject{0},gcTestTarget{0},gcTestHandle{0};
inline void runGcTest(uintptr_t material) {
    if(!gcTestRequested.exchange(false,std::memory_order_acq_rel))return;
    gcTestStatus.store(2,std::memory_order_release);
    const uintptr_t base=ed::g_runtime.unityBase;
    struct Check {uintptr_t rva;uint32_t word;};
    const Check checks[]={
        {0x651292C,0xAA1F03E1},{0x6512930,0x52800042},{0x6512934,0x15171989},
        {0x65017E0,0x15175DDD},{0x64FEBF4,0x151768BF}
    };
    for(const auto& c:checks) {
        uint32_t actual=0;
        if(!ed::readValue(reinterpret_cast<void*>(base+c.rva),actual) || actual!=c.word) {
            gcTestStatus.store(-1,std::memory_order_release);return;
        }
    }
    if(!material || !ptr(material,0x10) ||
       !ed::nameEquals(ed::className(ed::objectClass(reinterpret_cast<void*>(material))),"Material")) {
        gcTestStatus.store(-2,std::memory_order_release);return;
    }
    using Alloc=uintptr_t(*)(void*,const void*);
    using Target=void*(*)(uintptr_t,const void*);
    using Free=void(*)(uintptr_t,const void*);
    gcTestObject.store(material);
    const uintptr_t handle=reinterpret_cast<Alloc>(base+0x651292C)(reinterpret_cast<void*>(material),nullptr);
    gcTestHandle.store(handle);
    if(!handle){gcTestStatus.store(-3,std::memory_order_release);return;}
    const uintptr_t target=reinterpret_cast<uintptr_t>(reinterpret_cast<Target>(base+0x65017E0)(handle,nullptr));
    gcTestTarget.store(target);
    // Exactly one release for the handle allocated by this test.
    reinterpret_cast<Free>(base+0x64FEBF4)(handle,nullptr);
    gcTestStatus.store(target==material?3:-4,std::memory_order_release);
}
inline void gcTestControls() {
    ImGui::TextUnformatted("GCHandle test");
    if(ImGui::Button("Run GCHandle test")) {
        if(gcTestStatus.load()!=2 && !gcTestRequested.load()) {
            enabled.store(true);materialProbe.store(true);
            gcTestObject.store(0);gcTestTarget.store(0);gcTestHandle.store(0);
            gcTestStatus.store(1,std::memory_order_release);
            gcTestRequested.store(true,std::memory_order_release);
        }
    }
    const int st=gcTestStatus.load(std::memory_order_acquire);
    const char* text=st==0?"Not tested":st==1?"Waiting for live material callback":st==2?"Running":st==3?"PASS: target matches; FreeHandle returned":st==-1?"Code mismatch":st==-2?"Material rejected":st==-3?"Allocation returned zero":"FAIL: target mismatch; FreeHandle returned";
    ImGui::TextWrapped("%s",text);
    if(st==3 || st<0) {
        ImGui::Text("Handle value: 0x%llx",static_cast<unsigned long long>(gcTestHandle.load()));
        ImGui::Text("Source: %p",reinterpret_cast<void*>(gcTestObject.load()));
        ImGui::Text("Target: %p",reinterpret_cast<void*>(gcTestTarget.load()));
    }
    ImGui::TextWrapped("One-shot test. Materials unchanged. PASS checks immediate round-trip only, not lifetime across GC or scene changes.");
}


// ASTRA_VISUAL_TEST_V1
inline std::atomic<bool> visualRequested{false},visualActive{false},visualStop{false};
// ASTRA_TWO_TESTS_V3
inline std::atomic<int> visualStatus{0};
inline std::atomic<bool> visualWallMode{false};
inline uintptr_t visualRenderer=0,visualOriginal=0,visualClone=0,visualNative=0;
inline Clock::time_point visualEnd{};
// ASTRA_MATERIAL_WATCH_V5
inline uintptr_t watchedPlayerHandle=0,watchedWorld=0;
inline std::atomic<unsigned long long> watchReapplies{0};
inline std::atomic<int> watchState{0};

struct VisualColor {float r,g,b,a;};
template<class T> inline T visualFn(uintptr_t rva){return reinterpret_cast<T>(ed::g_runtime.unityBase+rva);}
inline uintptr_t visualRoot(void* o){return o?visualFn<uintptr_t(*)(void*,const void*)>(0x651292C)(o,nullptr):0;}
inline void* visualTarget(uintptr_t h){return h?visualFn<void*(*)(uintptr_t,const void*)>(0x65017E0)(h,nullptr):nullptr;}
inline void visualFree(uintptr_t& h){if(h)visualFn<void(*)(uintptr_t,const void*)>(0x64FEBF4)(h,nullptr);h=0;}
inline bool visualCode() {
 struct C{uintptr_t r;uint32_t w;};
 const C checks[]={{0x644A69C,0xA9BE57FE},{0x75FAF74,0xA9BE57FE},{0x67EE62C,0xA9BE57FE},{0x67F6B6C,0xA9BE57FE},
 {0x75E5FC0,0xF81D0FFE},{0x75E5FD0,0xAA0103F4},{0x75E6008,0xF9400AB3},{0x75E6034,0xF9400A94},{0x7602D8C,0xF81D0FFE},{0x75FC688,0x14000001},{0x7611348,0x17FF7FC8},{0x75EB8A0,0x14000001},
 {0x651292C,0xAA1F03E1},{0x65017E0,0x15175DDD},{0x64FEBF4,0x151768BF},
 {0x75F6F24,0xF81E0FFE},{0x7601EB8,0xF81E0FFE},{0x75F3B24,0xF81E0FFE}};
 for(auto c:checks){uint32_t v=0;if(!ed::readValue(reinterpret_cast<void*>(ed::g_runtime.unityBase+c.r),v)||v!=c.w)return false;}
 return true;
}
inline bool visualNamed(void* o,const char* n){return o && ed::nameEquals(ed::className(ed::objectClass(o)),n);}
inline void visualCleanup() {
 void* clone=visualTarget(visualClone);
 if(clone && ptr(reinterpret_cast<uintptr_t>(clone),0x10))
  visualFn<void(*)(void*,const void*)>(0x67F6B6C)(clone,nullptr);
 visualFree(visualClone);visualFree(visualOriginal);visualFree(visualRenderer);
 visualFree(watchedPlayerHandle);watchedWorld=0;
 visualNative=0;
}
inline void visualRestoreTick() {
 if(!visualActive.load() || !visualStop.load())return;
 void* renderer=visualTarget(visualRenderer);
 void* original=visualTarget(visualOriginal);
 void* clone=visualTarget(visualClone);
 int status=-6;
 if(renderer && ptr(reinterpret_cast<uintptr_t>(renderer),0x10)==visualNative && original) {
  using Getter=void*(*)(void*,const void*);
  void* current=visualFn<Getter>(0x7612138)(renderer,nullptr);
  uintptr_t count=0;
  if(!current || !ed::readValue(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(current)+off::managed::kArrayLength),count) || !count || count>32){visualStatus.store(-9);return;}
  uintptr_t first=ptr(reinterpret_cast<uintptr_t>(current),off::managed::kArrayData);
  if(first==reinterpret_cast<uintptr_t>(clone)) {
   uintptr_t saved=ptr(reinterpret_cast<uintptr_t>(original),off::managed::kArrayData);
   if(!saved || !ptr(saved,0x10)){visualStatus.store(-9);return;}
   visualFn<void(*)(void*,void*,const void*)>(0x75E5FC0)(renderer,reinterpret_cast<void*>(saved),nullptr);
   status=3;
  }else status=4; // Game already replaced our first slot.
 }
 visualCleanup();visualStop.store(false);visualActive.store(false);visualStatus.store(status);
}
inline void visualTry(const Row& row,const std::array<uintptr_t,32>& materials,size_t n) {
 if(!visualRequested.load() || visualActive.load() || !n || !materials[0] || row.local)return;
 ed::PlayerSnapshot snap{};
 if(!ed::readPlayerSnapshot(reinterpret_cast<void*>(row.player),snap) || !snap.statsValid || !snap.hp || !ed::validTeam(snap.team))return;
 int localTeam=astra::esp::g_localTeam.load();
 if(!ed::validTeam(static_cast<uint8_t>(localTeam)) || snap.team==localTeam)return;
 uintptr_t world=0;
 {std::lock_guard<std::mutex> lock(mutex);for(auto& r:rows)if(r.local && Clock::now()-r.when<std::chrono::seconds(2)){world=r.world;break;}}
 if(!world || world!=row.world)return;
 astra::esp::Vec3 pos{},screen{};
 astra::esp::CameraMatrixCache matrices{};uintptr_t camera=0;
 if(!ed::readPlayerPoseSnapshot(reinterpret_cast<void*>(row.player),pos) || !ed::readCameraForPresent(matrices,camera))return;
 if(!ed::project(matrices,pos,1000,1000,screen) || screen.x<0 || screen.x>1000 || screen.y<0 || screen.y>1000)return;

 using Getter=void*(*)(void*,const void*);
 void* source=reinterpret_cast<void*>(materials[0]);
 void* oldShader=visualFn<Getter>(0x75F9610)(source,nullptr);
 if(!oldShader || shaderString(visualFn<Getter>(0x6805E28)(oldShader,nullptr))!="SO2/Agents/CoreAgent")return;
 if(!visualRequested.exchange(false))return;
 if(!visualCode()){visualStatus.store(-1);return;}
 // Retain original array and renderer before allocation or assignment.
 visualOriginal=visualRoot(reinterpret_cast<void*>(row.arrayResult));
 visualRenderer=visualRoot(reinterpret_cast<void*>(row.renderer));visualNative=row.native;
 if(!visualOriginal || !visualRenderer){visualCleanup();visualStatus.store(-2);return;}
 const bool wall=visualWallMode.load();
 const char* name=wall?"Hidden/Internal-Colored":"Legacy Shaders/Diffuse";
 std::array<uint16_t,64> chars{};size_t length=std::strlen(name);
 for(size_t i=0;i<length;++i)chars[i]=static_cast<unsigned char>(name[i]);
 void* str=visualFn<void*(*)(const uint16_t*,int,int,const void*)>(0x644A69C)(chars.data(),0,int(length),nullptr);
 uintptr_t hs=visualRoot(str);
 if(!hs){visualCleanup();visualStatus.store(-2);return;}
 void* shader=visualFn<Getter>(0x75FAF74)(visualTarget(hs),nullptr);
 uintptr_t hshader=visualRoot(shader);visualFree(hs);
 shader=visualTarget(hshader);
 if(!visualNamed(shader,"Shader") || !ptr(reinterpret_cast<uintptr_t>(shader),0x10)) {
  visualFree(hshader);visualCleanup();visualStatus.store(-3);return;
 }
 int colorId=0,texId=0;bool colorFound=false,texFound=false;
 int depthId=0,writeId=0,srcId=0,dstId=0,cullId=0;
 unsigned depthMask=0;
 int count=visualFn<int(*)(void*,const void*)>(0x75F6F24)(shader,nullptr);
 if(count>=0 && count<=64)for(int i=0;i<count;++i){
  std::string prop=shaderString(visualFn<void*(*)(void*,int,const void*)>(0x75F3B24)(shader,i,nullptr));
  if(prop=="_Color"){colorId=visualFn<int(*)(void*,int,const void*)>(0x7601EB8)(shader,i,nullptr);colorFound=true;}
  if(wall){
   const int id=visualFn<int(*)(void*,int,const void*)>(0x7601EB8)(shader,i,nullptr);
   if(prop=="_ZTest"){depthId=id;depthMask|=1;}
   if(prop=="_ZWrite"){writeId=id;depthMask|=2;}
   if(prop=="_SrcBlend"){srcId=id;depthMask|=4;}
   if(prop=="_DstBlend"){dstId=id;depthMask|=8;}
   if(prop=="_Cull"){cullId=id;depthMask|=16;}
  }
  if(prop=="_MainTex"){texId=visualFn<int(*)(void*,int,const void*)>(0x7601EB8)(shader,i,nullptr);texFound=true;}
 }
 if(!colorFound || (!wall && !texFound) || (wall && depthMask!=31)){visualFree(hshader);visualCleanup();visualStatus.store(-4);return;}
 // Original array's strong handle retains source material.
 void* original=visualTarget(visualOriginal);
 uintptr_t sourceNow=ptr(reinterpret_cast<uintptr_t>(original),off::managed::kArrayData);
 void* clone=visualFn<Getter>(0x67EE62C)(reinterpret_cast<void*>(sourceNow),nullptr);
 visualClone=visualRoot(clone);clone=visualTarget(visualClone);
 if(!visualNamed(clone,"Material") || clone==reinterpret_cast<void*>(sourceNow) || !ptr(reinterpret_cast<uintptr_t>(clone),0x10)){
  if(clone==reinterpret_cast<void*>(sourceNow))visualFree(visualClone);
  visualFree(hshader);visualCleanup();visualStatus.store(-5);return;
 }
 visualFn<void(*)(void*,void*,const void*)>(0x7602D8C)(clone,visualTarget(hshader),nullptr);
 if(texFound)visualFn<void(*)(void*,int,void*,const void*)>(0x7611348)(clone,texId,nullptr,nullptr);
 if(wall){
  // Managed Material.SetFloat(int,float), RVA from the current 1.0.0 dump.
  // New call path: runtime behavior is not yet tested on this device.
  using SetFloat=void(*)(void*,int,float,const void*);
  auto set=visualFn<SetFloat>(0x760C250);
  set(clone,depthId,8.f,nullptr);
  set(clone,writeId,0.f,nullptr);
  set(clone,srcId,1.f,nullptr);
  set(clone,dstId,0.f,nullptr);
  set(clone,cullId,0.f,nullptr);
 }
 visualFn<void(*)(void*,int,VisualColor,const void*)>(0x75FC688)(clone,colorId,VisualColor{1.f,.05f,.65f,1.f},nullptr);
 visualFree(hshader);

 void* renderer=visualTarget(visualRenderer);
 if(!renderer || ptr(reinterpret_cast<uintptr_t>(renderer),0x10)!=visualNative){visualCleanup();visualStatus.store(-6);return;}
 visualEnd=Clock::now()+std::chrono::seconds(5);
 watchedPlayerHandle=visualRoot(reinterpret_cast<void*>(row.player));
 watchedWorld=row.world;
 if(!watchedPlayerHandle){visualCleanup();visualStatus.store(-2);return;}
 watchReapplies.store(0);watchState.store(1);
 visualStop.store(false);visualActive.store(true);
 visualFn<void(*)(void*,void*,const void*)>(0x75E5FC0)(renderer,visualTarget(visualClone),nullptr);
 visualStatus.store(2);

}

inline void watchMaterial(const Row& row,const std::array<uintptr_t,32>& materials,size_t n) {
 if(!visualActive.load() || visualStop.load())return;
 if(reinterpret_cast<uintptr_t>(visualTarget(watchedPlayerHandle))!=row.player)return;
 if(row.world!=watchedWorld){watchState.store(-1);visualStop.store(true);return;}
 void* renderer=visualTarget(visualRenderer);
 if(row.renderer!=reinterpret_cast<uintptr_t>(renderer) || row.native!=visualNative){watchState.store(2);return;}
 void* clone=visualTarget(visualClone);
 if(!clone || !ptr(reinterpret_cast<uintptr_t>(clone),0x10)){watchState.store(-2);visualStop.store(true);return;}
 if(!n || !materials[0]){watchState.store(3);return;}
 if(materials[0]==reinterpret_cast<uintptr_t>(clone)){watchState.store(1);return;}
 using Getter=void*(*)(void*,const void*);
 void* shader=visualFn<Getter>(0x75F9610)(reinterpret_cast<void*>(materials[0]),nullptr);
 if(!shader || !ptr(reinterpret_cast<uintptr_t>(shader),0x10) || shaderString(visualFn<Getter>(0x6805E28)(shader,nullptr))!="SO2/Agents/CoreAgent"){
  watchState.store(4);return;
 }
 // Save the game's latest original, not an outdated skin/material array.
 uintptr_t fresh=visualRoot(reinterpret_cast<void*>(row.arrayResult));
 if(!fresh){watchState.store(-3);return;}
 visualFree(visualOriginal);visualOriginal=fresh;
 visualFn<void(*)(void*,void*,const void*)>(0x75E5FC0)(renderer,visualTarget(visualClone),nullptr);
 watchReapplies.fetch_add(1);watchState.store(1);
}

inline void visualControls() {
 ImGui::TextUnformatted("Chams / material watch V5");
 ImGui::Text("Reapplied: %llu",watchReapplies.load());
 const int ws=watchState.load();
 ImGui::TextWrapped("Watch: %s",ws==0?"idle":ws==1?"same renderer / material tracked":ws==2?"RENDERER CHANGED - LOD not followed":ws==3?"no first material":ws==4?"different first-slot shader; not overwritten":ws==-1?"world changed":ws==-2?"clone unavailable":"GC root failed");
 auto request=[](bool wall){
  if(visualActive.load() || visualRequested.load())return;
  visualWallMode.store(wall);
  enabled.store(true);materialProbe.store(true);
  visualStatus.store(1);visualRequested.store(true);
 };
 if(ImGui::Button("Enable visible chams"))request(false);
 if(ImGui::Button("Enable through-wall chams"))request(true);
 if(ImGui::Button("Disable / restore")){
  visualRequested.store(false);visualStop.store(true);
  if(!visualActive.load())visualStatus.store(0);
 }
 ImGui::TextWrapped("One enemy. No timer. Disable before switching modes. Game occlusion is unchanged.");
 const int v=visualStatus.load();
 const char* msg=v==0?"Not started":v==1?"Waiting for enemy with CoreAgent in slot 0":v==2?"Enabled until Disable / restore":v==3?"Original first material restored":v==4?"Game replaced material; test cleaned up":v==-9?"Restore pending: original/current material unavailable":v==-1?"Code mismatch":v==-2?"GC root failed":v==-3?"Selected shader missing":v==-4?"Properties missing":v==-5?"Clone failed":v==-6?"Renderer gone/changed; clone cleanup":v==-7?"Array consistency failed":"Write barrier unavailable; not applied";
 ImGui::TextWrapped("%s",msg);
}

inline void onPlayer(void* player) {
    if(!player || !ed::g_runtime.ready || !ed::isUnityMainCallback())return;
    visualRestoreTick();
    if(!enabled.load(std::memory_order_relaxed) && !visualActive.load())return;
    callbacks.fetch_add(1,std::memory_order_relaxed);
    if(!ed::isUnityMainCallback() || ed::objectClass(player)!=ed::g_runtime.playerClass)return;
    donorSearch();
    const auto now=Clock::now();
    const uintptr_t p=reinterpret_cast<uintptr_t>(player);
    {
        std::lock_guard<std::mutex> lock(mutex);
        for(const auto& r:rows) if(r.player==p && now-r.when<std::chrono::milliseconds(500))return;
    }
    Row row{};row.player=p;row.when=now;
    row.world=ptr(p,off::field::mfc::kWorld);
    ed::readValue(reinterpret_cast<void*>(p+off::field::mfc::kLocalOwned),row.local);
    auto scan=[&]() {
        if(!row.world)return;
        const uintptr_t container=ptr(p,off::field::mfc::kPartsContainer);
        if(!container)return;
        row.stage=1;
        const uintptr_t list=ptr(container,off::field::part_container::kPartList);
        if(!list)return;
        int32_t count=0;
        if(!ed::readValue(reinterpret_cast<void*>(list+off::field::managed_list::kSize),count) || count<0 || count>64)return;
        const uintptr_t array=ptr(list,off::field::managed_list::kItems);
        uintptr_t length=0;
        if(!array || !ed::readValue(reinterpret_cast<void*>(array+off::managed::kArrayLength),length) || length<static_cast<uintptr_t>(count) || length>256)return;
        row.stage=2;
        if(!ed::g_runtime.modelClass)return;
        for(int i=0;i<count;++i) {
            const uintptr_t part=ptr(array,off::managed::kArrayData+size_t(i)*sizeof(void*));
            if(part && ed::objectClass(reinterpret_cast<void*>(part))==ed::g_runtime.modelClass) {row.model=part;break;}
        }
        if(!row.model)return;
        row.stage=3;
        row.bindings=ptr(row.model,0x30);
        if(!row.bindings || !ed::objectClass(reinterpret_cast<void*>(row.bindings)))return;
        row.stage=4;
        row.renderer=ptr(row.bindings,0x38);
        row.arms=ptr(row.bindings,0x40);row.gloves=ptr(row.bindings,0x48);
        row.untouchable=ptr(row.bindings,0x50);row.outline=ptr(row.bindings,0x58);
        if(!row.renderer || !ed::objectClass(reinterpret_cast<void*>(row.renderer)))return;
        row.stage=5;
        // Candidate m_CachedPtr, not a validated native renderer.
        row.native=ptr(row.renderer,0x10);
        if(row.native)row.stage=6;
        if(!row.native || (!materialProbe.load(std::memory_order_relaxed) && !visualActive.load()))return;
        auto named=[](uintptr_t obj,const char* name) {
            return obj && ed::nameEquals(ed::className(ed::objectClass(reinterpret_cast<void*>(obj))),name);
        };
        if(!named(row.bindings,"CharacterBindings") || !named(row.renderer,"SkinnedMeshRenderer")) {row.materialStatus=-1;return;}
        uint32_t instruction=0;
        const uintptr_t base=ed::g_runtime.unityBase;
        if(!ed::readValue(reinterpret_cast<void*>(base+0x7612138),instruction) || instruction!=0x17FFE2C3) {row.materialStatus=-2;return;}
        const std::array<uint32_t,2> expected{0xA9BE57FE,0xA9014FF4};
        std::array<uint32_t,2> actual{};
        if(!ed::readValue(reinterpret_cast<void*>(base+0x75F9610),actual) || actual!=expected) {row.materialStatus=-2;return;}
        using Getter=void*(*)(void*,const void*);
        // Managed wrapper, not the internal native entry point.
        row.arrayResult=reinterpret_cast<uintptr_t>(reinterpret_cast<Getter>(base+0x7612138)(reinterpret_cast<void*>(row.renderer),nullptr));
        if(!row.arrayResult){row.materialStatus=-3;return;}
        uintptr_t n=0;
        if(!ed::objectClass(reinterpret_cast<void*>(row.arrayResult)) ||
           !ed::readValue(reinterpret_cast<void*>(row.arrayResult+off::managed::kArrayLength),n) || n>32) {row.materialStatus=-4;return;}
        row.materialCount=static_cast<int>(n);
        if(!n){row.materialStatus=1;return;}
        std::array<uintptr_t,32> materials{};
        const size_t bytes=n*sizeof(uintptr_t);
        if(ed::safeReadMemory(materials.data(),reinterpret_cast<void*>(row.arrayResult+off::managed::kArrayData),bytes)!=static_cast<ssize_t>(bytes)) {row.materialStatus=-4;return;}
        // Validate every non-null entry before calling any Material getter.
        for(size_t i=0;i<n;++i)
            if(materials[i] && (!named(materials[i],"Material") || !ptr(materials[i],0x10))) {row.materialStatus=-5;return;}
        for(size_t i=0;i<n;++i) {
            if(!materials[i])continue;
            runGcTest(materials[i]);
            const uintptr_t shader=reinterpret_cast<uintptr_t>(reinterpret_cast<Getter>(base+0x75F9610)(reinterpret_cast<void*>(materials[i]),nullptr));
            if(shader && named(shader,"Shader") && ptr(shader,0x10)) {
                ++row.shaderCount;if(!row.firstShader)row.firstShader=shader;
                inspectShader(shader);
            }
        }
        watchMaterial(row,materials,n);
        visualTry(row,materials,n);
        row.materialStatus=2;

    };
    scan();
    std::lock_guard<std::mutex> lock(mutex);
    Row* slot=nullptr;
    for(auto& r:rows)if(r.player==p){slot=&r;break;}
    if(!slot)for(auto& r:rows)if(!r.player || now-r.when>std::chrono::seconds(2)){slot=&r;break;}
    if(slot)*slot=row;
}
inline void controls() {
    if(!astra_sections::header("Chams diagnostics",ImGui::CalcItemWidth()))return;
    visualControls();
    bool on=enabled.load();
    if(ImGui::Checkbox("Read-only probe",&on)) {
        enabled.store(on);
        std::lock_guard<std::mutex> lock(mutex);
        rows={};callbacks.store(0);
    }
    ImGui::TextWrapped("Probe reads objects; enabled chams change one enemy material.");
    ImGui::Text("Callbacks: %llu",callbacks.load());
    if(!on)return;
    bool test=materialProbe.load();
    if(ImGui::Checkbox("Read materials / shaders",&test)) {
        materialProbe.store(test);
        std::lock_guard<std::mutex> lock(mutex);rows={};
    }
    ImGui::TextWrapped("Optional Unity getter test. No setters. Counts and references only, not shader names.");

    std::array<Row,128> copy;
    {std::lock_guard<std::mutex> lock(mutex);copy=rows;}
    const auto now=Clock::now();
    uintptr_t world=0;
    for(const auto& r:copy)if(r.local && r.world && now-r.when<std::chrono::seconds(2)) {world=r.world;break;}
    int players=0,models=0,bindings=0,renderers=0,natives=0,outlines=0;
    std::array<int,7> stages{};
    for(const auto& r:copy) {
        if(!r.player || now-r.when>std::chrono::seconds(2) || !world || r.world!=world)continue;
        ++players;++stages[r.stage];
        models+=r.stage>=3;bindings+=r.stage>=4;renderers+=r.stage>=5;natives+=r.stage>=6;outlines+=r.outline!=0;
    }
    ImGui::Text("Local world: %s",world?"found":"waiting");
    ImGui::Text("Players: %d | Models: %d",players,models);
    ImGui::Text("Bindings: %d | Body refs: %d",bindings,renderers);
    ImGui::Text("Native candidates: %d | Outline refs: %d",natives,outlines);
    ImGui::Text("Stages: %d / %d / %d / %d / %d / %d / %d",stages[0],stages[1],stages[2],stages[3],stages[4],stages[5],stages[6]);
    ImGui::TextWrapped("Stages: 0 world/parts missing; 1 list invalid; 2 model missing; 3 bindings missing; 4 body missing; 5 native missing; 6 chain found. References are candidates, not validated rendering objects.");

    if(materialProbe.load()) {
        int success=0,failed=0,slots=0,shaders=0,lastError=0;
        for(const auto& r:copy) {
            if(!r.player || !world || r.world!=world || now-r.when>std::chrono::seconds(2))continue;
            if(r.materialStatus>0){++success;slots+=r.materialCount;shaders+=r.shaderCount;}
            if(r.materialStatus<0){++failed;lastError=r.materialStatus;}
        }
        ImGui::Text("Material reads OK: %d | failed: %d",success,failed);
        ImGui::Text("Slots: %d | Shader refs: %d",slots,shaders);
        ImGui::Text("Last error: %d",lastError);
        ImGui::TextWrapped("Errors: -1 type; -2 code; -3 null result; -4 array layout; -5 material entry. Shader names not read yet.");
    }
    gcTestControls();
    shaderNamesControls();
    if(ImGui::TreeNode("Object details")) {
        int shown=0;
        for(const auto& r:copy) {
            if(!r.player || now-r.when>std::chrono::seconds(2) || !world || r.world!=world)continue;
            ImGui::Text("%s | stage %d",r.local?"LOCAL":"REMOTE",r.stage);
            ImGui::Text("Model %p",reinterpret_cast<void*>(r.model));
            ImGui::Text("Bindings %p",reinterpret_cast<void*>(r.bindings));
            ImGui::Text("Body %p",reinterpret_cast<void*>(r.renderer));
            ImGui::Text("Material test: %d | slots: %d | shaders: %d",r.materialStatus,r.materialCount,r.shaderCount);
            ImGui::Text("Array %p | shader %p",reinterpret_cast<void*>(r.arrayResult),reinterpret_cast<void*>(r.firstShader));

            if(++shown>=16)break;
        }
        ImGui::TreePop();
    }
}
}
