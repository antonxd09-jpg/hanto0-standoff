// ASTRA_HITLOG_SIZE_V5
// ASTRA_HITLOG_DISPATCH_V2
#pragma once
#include "features/visuals/esp_runtime.hpp"
#include "imgui.h"
#include "dobby.h"
// ASTRA_HITSOUND_V1
#include "astra_hitsound.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>

namespace astra_hitlog {
namespace ed = astra::esp::detail;
using Clock = std::chrono::steady_clock;
inline std::atomic<bool> enabled{false};
// 0 idle, 1 installing, 2 installed, -1 incompatible code, -2 install failed
inline std::atomic<int> state{0};
inline std::atomic<unsigned long long> received{0}, accepted{0};
struct Identity { void* player{}; void* world{}; uint64_t id{}; std::string name; Clock::time_point time{}; };
struct Line { std::string name; int damage; Clock::time_point time; bool kill=false; };
inline std::mutex mutex;
inline std::array<Identity,128> identities{};
inline std::vector<Line> lines;
inline uint64_t localId{};
inline void* localWorld{};
inline Clock::time_point localTime{};
inline constexpr uintptr_t eventRva = 0xAA3EAFC;

inline std::string readName(void* object) {
    if (!object) return {};
    const auto a = reinterpret_cast<uintptr_t>(object);
    int32_t n=0;
    if (!ed::readValue(reinterpret_cast<void*>(a+0x10), n) || n<=0 || n>128) return {};
    std::array<uint16_t,128> text{};
    if (ed::safeReadMemory(text.data(), reinterpret_cast<void*>(a+0x14), size_t(n)*2) != n*2) return {};
    std::string out;
    for(int i=0;i<n;++i) {
        uint32_t c=text[i];
        if(c>=0xD800 && c<=0xDBFF) {
            if(i+1<n && text[i+1]>=0xDC00 && text[i+1]<=0xDFFF) {
                c=0x10000+((c-0xD800)<<10)+(text[++i]-0xDC00);
            } else c=0xFFFD;
        } else if(c>=0xDC00 && c<=0xDFFF) c=0xFFFD;
        if(c<32 || c==127) c=' ';
        if(c<0x80) out+=char(c);
        else if(c<0x800) {out+=char(0xC0|(c>>6));out+=char(0x80|(c&63));}
        else if(c<0x10000) {out+=char(0xE0|(c>>12));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
        else {out+=char(0xF0|(c>>18));out+=char(0x80|((c>>12)&63));out+=char(0x80|((c>>6)&63));out+=char(0x80|(c&63));}
    }
    return out;
}

inline void event(void*, DobbyRegisterContext* ctx) {
#if defined(__aarch64__)
    if(!enabled.load(std::memory_order_relaxed) || !ctx) return;
    received.fetch_add(1,std::memory_order_relaxed);
    // Unboxed event prefix verified against ARM64 disassembly.
    struct Prefix {uint64_t victim,producer; int32_t damage; uint32_t rest;};
    static_assert(sizeof(Prefix)==24);
    Prefix p{};
    if(!ed::readValue(reinterpret_cast<void*>(ctx->sp),p) || p.damage<=0 || p.damage>10000) return;
    const auto now=Clock::now();
    std::lock_guard<std::mutex> lock(mutex);
    if(!localWorld || now<localTime || now-localTime>std::chrono::milliseconds(500) || p.producer!=localId || p.victim==localId) return;
    std::string name;
    for(const auto& v:identities) {
        if(v.player && v.world==localWorld && v.id==p.victim && now>=v.time && now-v.time<std::chrono::seconds(2)) {
            name=v.name; break;
        }
    }
    if(name.empty()) name="unknown";
    while(lines.size()>=4) lines.erase(lines.begin());
    lines.push_back({name,p.damage,now});
    accepted.fetch_add(1,std::memory_order_relaxed);
    astra_sound::request();
#endif
}


// ASTRA_HITLOG_DEATH_V4
inline std::atomic<int> deathState{0};
inline std::atomic<unsigned long long> deathReceived{0}, deathAccepted{0};
struct DeathSeen {
    void* world{};
    uint64_t victim{}, killer{};
    int32_t damage{};
    std::array<unsigned char,25> hit{};
    Clock::time_point time{};
};
inline std::array<DeathSeen,32> deathSeen{};
inline size_t deathCursor=0;

inline void deathEvent(void*, DobbyRegisterContext* ctx) {
#if defined(__aarch64__)
    if(!ctx || !enabled.load(std::memory_order_relaxed)) return;
    deathReceived.fetch_add(1,std::memory_order_relaxed);
    struct Prefix {
        uint64_t victim,killer,assistant;
        uint8_t weapon,padding;
        uint16_t meta;
        int32_t damage;
    };
    static_assert(sizeof(Prefix)==32);
    static_assert(offsetof(Prefix,damage)==0x1C);
    Prefix p{};
    std::array<unsigned char,25> hit{};
    if(!ed::readValue(reinterpret_cast<void*>(ctx->sp),p) || p.damage<0 || p.damage>10000) return;
    // HitPoint, ShotPoint and WasPenetrated, excluding structure padding.
    if(!ed::readValue(reinterpret_cast<void*>(ctx->sp+0x24),hit)) return;
    const auto now=Clock::now();
    std::lock_guard<std::mutex> lock(mutex);
    if(!localWorld || now<localTime || now-localTime>std::chrono::seconds(2) || p.killer!=localId || p.victim==localId) return;
    // Bounded heuristic for repeated death notifications, not normal bullets.
    for(const auto& v:deathSeen)
        if(v.world==localWorld && v.victim==p.victim && v.killer==p.killer && v.damage==p.damage && v.hit==hit && now>=v.time && now-v.time<std::chrono::seconds(1)) return;
    deathSeen[deathCursor++%deathSeen.size()]={localWorld,p.victim,p.killer,p.damage,hit,now};
    std::string name;
    for(const auto& v:identities)
        if(v.player && v.world==localWorld && v.id==p.victim && now>=v.time && now-v.time<std::chrono::seconds(5)) {name=v.name;break;}
    if(name.empty()) name="unknown";
    while(lines.size()>=4) lines.erase(lines.begin());
    lines.push_back({name,p.damage,now,true});
    accepted.fetch_add(1,std::memory_order_relaxed);
    astra_sound::request();
    deathAccepted.fetch_add(1,std::memory_order_relaxed);
#endif
}
inline void installDeathHook() {
#if defined(__aarch64__)
    int expected=0;
    if(!deathState.compare_exchange_strong(expected,1)) return;
    const std::array<uint32_t,4> signature{0x394033f4,0x394013e8,0x6b08029f,0x54000580};
    std::array<uint32_t,4> actual{};
    auto target=reinterpret_cast<void*>(ed::g_runtime.unityBase+0xA78D3CC);
    if(!ed::readValue(target,actual) || actual!=signature) {deathState.store(-1);return;}
    deathState.store(DobbyInstrument(target,&deathEvent)==0?2:-2);
#endif
}
inline void deathDiagnostics() {
    astra_sound::controls();
    const int st=deathState.load();
    ImGui::Text("Death hook: %s",st==2?"installed":st==-1?"code mismatch":st==-2?"failed":st==1?"installing":"waiting");
    ImGui::Text("Death events: %llu | Local kills: %llu",deathReceived.load(),deathAccepted.load());
}

inline void onPlayer(void* player) {
#if defined(__aarch64__)
    if(!enabled.load(std::memory_order_relaxed) || !ed::isUnityMainCallback() || !ed::g_runtime.ready || !player) return;
    if(ed::objectClass(player)!=ed::g_runtime.playerClass) return;
    const auto base=reinterpret_cast<uintptr_t>(player);
    uint64_t id=0; uint8_t local=0;
    auto world=ed::readObject(reinterpret_cast<void*>(base+astra::offsets::field::mfc::kWorld));
    if(!world || !ed::readValue(reinterpret_cast<void*>(base+0x10),id) || !ed::readValue(reinterpret_cast<void*>(base+astra::offsets::field::mfc::kLocalOwned),local)) return;
    const auto now=Clock::now();
    {
        std::lock_guard<std::mutex> lock(mutex);
        if(local) {
            if(localWorld!=world || localId!=id) {identities={};lines.clear();deathSeen={};deathCursor=0;}
            localWorld=world; localId=id; localTime=now;
        }
    }
    if(local) installDeathHook();
    if(local && state.load()==0) {
        int expected=0;
        if(state.compare_exchange_strong(expected,1)) {
            std::array<uint32_t,4> prologue{};
            const std::array<uint32_t,4> expectedCode{0x910003e1,0xaa1403e0,0xf942a108,0xf9400102};
            auto target=reinterpret_cast<void*>(ed::g_runtime.unityBase+eventRva);
            if(!ed::readValue(target,prologue) || prologue!=expectedCode) state.store(-1);
            else state.store(DobbyInstrument(target,&event)==0?2:-2);
        }
    }
    {
        std::lock_guard<std::mutex> lock(mutex);
        for(const auto& v:identities)
            if(v.player==player && v.id==id && v.world==world && now-v.time<std::chrono::milliseconds(250)) return;
    }
    // Managed calls only from the live player's UnityMain callback.
    using ActorFn=void*(*)(void*,const void*);
    void* actor=reinterpret_cast<ActorFn>(ed::g_runtime.unityBase+0x7e30f90)(player,nullptr);
    std::string name;
    if(actor && ed::nameEquals(ed::className(ed::objectClass(actor)),"uwa"))
        name=readName(ed::readObject(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(actor)+0x18)));
    std::lock_guard<std::mutex> lock(mutex);
    Identity* slot=nullptr;
    for(auto& v:identities) if(v.player==player) {slot=&v;break;}
    if(!slot) for(auto& v:identities) if(!v.player || now-v.time>std::chrono::seconds(2)) {slot=&v;break;}
    if(slot) *slot={player,world,id,std::move(name),now};
#endif
}

inline void render(bool menuOpen) {
    astra_sound::pump(enabled.load(std::memory_order_relaxed));
    // The menu owns the foreground while it is open; entries would draw over it.
    if(astra::aim::g_menuOpen.load(std::memory_order_relaxed)) return;
    if(menuOpen) {
        ImGui::SetNextWindowSize(ImVec2(310,0),ImGuiCond_FirstUseEver);
        if(ImGui::Begin("Hitlog##astra",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            bool v=enabled.load();
            if(ImGui::Checkbox("Hitlog",&v)) {
                enabled.store(v);
                if(!v) {std::lock_guard<std::mutex> lock(mutex); lines.clear();localWorld=nullptr;}
            }
            const int st=state.load();
            ImGui::TextUnformatted(st==2?"Hook installed":st==-1?"Code mismatch":st==-2?"Hook installation failed":st==1?"Installing...":"Waiting for enabled / local player");
            ImGui::Text("Events: %llu | Local hits: %llu",received.load(),accepted.load());
            ImGui::TextUnformatted("Name | damage; no bones");
        }
        ImGui::End();
    }
    if(!enabled.load()) return;
    const auto now=Clock::now();
    std::vector<Line> copy;
    {
        std::lock_guard<std::mutex> lock(mutex);
        if(!localWorld || now-localTime>std::chrono::seconds(1)) {lines.clear();return;}
        lines.erase(std::remove_if(lines.begin(),lines.end(),[&](const Line& l){return now-l.time>std::chrono::seconds(4);}),lines.end());
        copy=lines;
    }
    // ASTRA_HITLOG_STYLE_V3
    const auto screen=ImGui::GetIO().DisplaySize;
    if(screen.x<160 || screen.y<120) return;
    const float scale=std::clamp(screen.y/1080.f,.85f,1.35f);
    const float fs=22.f*scale, h=50.f*scale, gap=9.f*scale;
    const float w=std::min(425.f*scale,screen.x-32.f);
    const float cx=screen.x*.5f;
    auto* d=ImGui::GetForegroundDrawList();
    auto* font=ImGui::GetFont();
    float bottom=screen.y-std::max(65.f*scale,screen.y*.10f);
    d->PushClipRect(ImVec2(0,0),screen,true);
    // Newest at the bottom, older entries stack upwards.
    for(auto it=copy.rbegin();it!=copy.rend();++it){
        const auto& l=*it;
        const float age=std::chrono::duration<float>(now-l.time).count();
        const float enter=std::clamp(age/.18f,0.f,1.f);
        const float ease=1.f-(1.f-enter)*(1.f-enter)*(1.f-enter);
        const float alpha=ease*std::clamp((4.f-age)/.55f,0.f,1.f);
        const float y=bottom-h+(1.f-ease)*10.f*scale;
        if(y<12.f) break;
        const float x=cx-w*.5f;
        auto col=[&](int r,int g,int b,int a){return IM_COL32(r,g,b,int(float(a)*alpha));};
        d->AddRectFilled(ImVec2(x+2,y+4),ImVec2(x+w+2,y+h+4),col(0,0,0,65),9.f*scale);
        d->AddRectFilled(ImVec2(x,y),ImVec2(x+w,y+h),col(16,20,29,235),9.f*scale);
        d->AddRect(ImVec2(x,y),ImVec2(x+w,y+h),col(120,147,188,90),9.f*scale);
        d->AddRectFilled(ImVec2(x,y+11*scale),ImVec2(x+3*scale,y+h-11*scale),col(130,185,245,230),2*scale);
        char value[48];std::snprintf(value,sizeof(value),l.kill?"KILL | %d dmg":"%d dmg",l.damage);
        const float tw=font->CalcTextSizeA(fs,10000,0,value).x;
        const float bw=tw+22.f*scale;
        const float bx=x+w-bw-10.f*scale;
        d->AddRectFilled(ImVec2(bx,y+7*scale),ImVec2(x+w-10*scale,y+h-7*scale),col(72,112,156,70),6*scale);
        const float ty=y+(h-fs)*.5f;
        d->AddText(font,fs,ImVec2(bx+11*scale,ty),col(178,217,255,255),value);
        const float nx=x+17.f*scale, available=bx-nx-13.f*scale;
        std::string label=l.name;
        const float dots=font->CalcTextSizeA(fs,10000,0,"...").x;
        if(font->CalcTextSizeA(fs,10000,0,label.c_str()).x>available){
            while(!label.empty() && font->CalcTextSizeA(fs,10000,0,label.c_str()).x+dots>available){
                size_t pos=label.size()-1;
                while(pos>0 && (static_cast<unsigned char>(label[pos])&0xC0)==0x80) --pos;
                label.resize(pos);
            }
            label+="...";
        }
        d->PushClipRect(ImVec2(nx,y),ImVec2(bx-8*scale,y+h),true);
        d->AddText(font,fs,ImVec2(nx,ty),col(233,239,249,255),label.c_str());
        d->PopClipRect();
        bottom-=h+gap;
    }
    d->PopClipRect();
}
}
