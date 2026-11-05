#pragma once
#include "features/visuals/esp_runtime.hpp"
#include "dobby.h"
#include "imgui.h"
#include <atomic>
#include <mutex>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <array>
#include <android/log.h>
namespace astra_camera_test {
namespace ed=astra::esp::detail;
using V=astra::esp::Vec3;
using Fn=void(*)(void*,const void*);
inline Fn original=nullptr;
inline std::atomic<bool> enabled{false};
inline std::atomic<int> state{0}, result{0};
inline std::atomic<unsigned long long> calls{0}, applied{0}, rejected{0}, restored{0};
inline std::atomic<long long> lease{0};
inline std::atomic<uintptr_t> baseSeen{0}, lastObject{0};
inline std::mutex lock;
inline V settings{0,.2f,-2.f}, lastBefore{}, lastDuring{};
inline char saveStatus[320]="Not saved";
inline std::array<uint32_t,4> observed{};
inline long long nowMs(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline bool finite(V v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&std::fabs(v.x)<100&&std::fabs(v.y)<100&&std::fabs(v.z)<100;}
// Only xmt.syzn is intercepted, never shared nrl.LateTick or arms controller.
inline void hook(void* self,const void* method){
    calls.fetch_add(1,std::memory_order_relaxed);
    auto fn=original;
    if(!fn)return;
    const auto age=nowMs()-lease.load(std::memory_order_relaxed);
    if(!enabled.load(std::memory_order_relaxed)||!self||age<0||age>500||!ed::isUnityMainCallback()) {fn(self,method);return;}
    V old{},delta{};
    auto field=reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(self)+0x38);
    // self is the live receiver of the game's own call. No cached receiver retained.
    if(!ed::readValue(field,old)||!finite(old)) {++rejected;fn(self,method);return;}
    {std::lock_guard<std::mutex> g(lock);delta=settings;}
    V next{old.x+delta.x,old.y+delta.y,old.z+delta.z};
    if(!finite(next)){++rejected;fn(self,method);return;}
    lastObject.store(reinterpret_cast<uintptr_t>(self));
    {std::lock_guard<std::mutex> g(lock);lastBefore=old;lastDuring=next;}
    // Restore immediately after this invocation; no delayed stale-object write.
    std::memcpy(field,&next,sizeof(next));
    fn(self,method);
    std::memcpy(field,&old,sizeof(old));
    ++applied; ++restored;
}
inline void onPlayer(void* player){
#if defined(__aarch64__)
    if(!enabled.load()||!ed::isUnityMainCallback()||!ed::g_runtime.ready||!player)return;
    if(ed::objectClass(player)!=ed::g_runtime.playerClass)return;
    auto p=reinterpret_cast<uintptr_t>(player);
    uint8_t local=0;
    if(!ed::readValue(reinterpret_cast<void*>(p+astra::offsets::field::mfc::kLocalOwned),local)||!local)return;
    if(!ed::readObject(reinterpret_cast<void*>(p+astra::offsets::field::mfc::kWorld)))return;
    lease.store(nowMs(),std::memory_order_relaxed);
    int expected=0;
    if(!state.compare_exchange_strong(expected,1))return;
    auto base=ed::g_runtime.unityBase;
    baseSeen.store(base);
    auto target=reinterpret_cast<void*>(base+0x98C9EF8);
    const std::array<uint32_t,4> wanted{0xFC1D0FEA,0x6D00A3E9,0xF9000FFE,0xA9024FF4};
    std::array<uint32_t,4> actual{};
    bool read=ed::readValue(target,actual);
    {std::lock_guard<std::mutex> g(lock);observed=actual;}
    if(!read||actual!=wanted){state.store(-1);return;}
    // Installed from UnityMain, before any subsequent camera callback on that thread.
    int r=DobbyHook(target,reinterpret_cast<void*>(&hook),reinterpret_cast<void**>(&original));
    result.store(r);
    state.store(r==0&&original?2:-2);
    __android_log_print(ANDROID_LOG_INFO,"MixCameraTest","install=%d result=%d base=%p",state.load(),r,reinterpret_cast<void*>(base));
#else
    (void)player; state.store(-3);
#endif
}
inline void save(){
    V s{},o{},n{};std::array<uint32_t,4> code{};
    {std::lock_guard<std::mutex> g(lock);s=settings;o=lastBefore;n=lastDuring;code=observed;}
    char report[2048];
    std::snprintf(report,sizeof(report),
      "CameraTest V1\nExperimental offset only; no body/arms/collision changes.\n"
      "state=%d (0 waiting,1 installing,2 installed,-1 code mismatch,-2 hook failure,-3 unsupported)\n"
      "enabled=%d result=%d base=%p receiver=%p\nlease_age_ms=%lld\n"
      "calls=%llu applied=%llu restored=%llu rejected=%llu\n"
      "delta=%.3f %.3f %.3f\nbefore=%.3f %.3f %.3f\nduring=%.3f %.3f %.3f\n"
      "code=%08x %08x %08x %08x\nplayer_hook=%d player_calls=%llu\n"
      "Note: applied counts hook execution, not verified rendered displacement.\n",
      state.load(),int(enabled.load()),result.load(),reinterpret_cast<void*>(baseSeen.load()),reinterpret_cast<void*>(lastObject.load()),
      nowMs()-lease.load(),calls.load(),applied.load(),restored.load(),rejected.load(),
      s.x,s.y,s.z,o.x,o.y,o.z,n.x,n.y,n.z,code[0],code[1],code[2],code[3],
      int(astra::esp::g_playerUpdateHookReady.load()),(unsigned long long)astra::esp::g_playerUpdateHookCalls.load());
    __android_log_print(ANDROID_LOG_INFO,"MixCameraTest","%s",report);
    char path[240];std::snprintf(path,sizeof(path),"/storage/emulated/0/Download/mix_camera_%lld.txt",nowMs());
    FILE* f=std::fopen(path,"w");
    if(!f){int err=errno;std::snprintf(saveStatus,sizeof(saveStatus),"Save failed: %s (%d). Report in logcat: MixCameraTest",std::strerror(err),err);return;}
    size_t size=std::strlen(report);bool ok=std::fwrite(report,1,size,f)==size;
    int closeResult=std::fclose(f);
    std::snprintf(saveStatus,sizeof(saveStatus),"%s: %s",ok&&closeResult==0?"Saved":"Write failed",path);
}
inline void reset(){enabled.store(false);std::lock_guard<std::mutex> g(lock);settings={0,.2f,-2.f};}
inline void controls(){
    ImGui::PushID("camera-test-v1");
    bool on=enabled.load();
    if(ImGui::Checkbox("Third person (test)",&on))enabled.store(on);
    V s{};{std::lock_guard<std::mutex> g(lock);s=settings;}
    float distance=-s.z;bool changed=false;
    changed|=ImGui::SliderFloat("Distance",&distance,0.f,4.f,"%.2f");
    changed|=ImGui::SliderFloat("Height",&s.y,-1.f,1.f,"%.2f");
    changed|=ImGui::SliderFloat("Side",&s.x,-1.f,1.f,"%.2f");s.z=-distance;
    if(changed){std::lock_guard<std::mutex> g(lock);settings=s;}
    ImGui::Text("Hook: %d | result: %d",state.load(),result.load());
    ImGui::Text("Calls: %llu | Applied: %llu",calls.load(),applied.load());
    ImGui::Text("Restored: %llu | Rejected: %llu",restored.load(),rejected.load());
    ImGui::TextWrapped("Experimental camera offset. No body, arms or collision fix. Disable before spectating.");
    if(ImGui::Button("Save camera diagnostics"))save();
    ImGui::TextWrapped("%s",saveStatus);
    ImGui::PopID();
}
}
