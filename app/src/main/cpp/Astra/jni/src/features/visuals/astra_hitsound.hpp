#pragma once
#include "astra_hitsound_pcm.hpp"
#include "imgui.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>

namespace astra_sound {
inline std::atomic<bool> enabled{true};
inline std::atomic<unsigned> pending{0};
inline float gain=.65f;
inline int status=0; // render thread only: 0 idle, 1 ready, -1 failed
inline SLresult error=SL_RESULT_SUCCESS;
inline SLObjectItf engineObject=nullptr, mixObject=nullptr;
inline SLEngineItf engine=nullptr;
struct Voice {
    SLObjectItf object=nullptr;
    SLPlayItf play=nullptr;
    SLAndroidSimpleBufferQueueItf queue=nullptr;
    SLVolumeItf volume=nullptr;
    std::atomic<bool> busy{false};
};
inline std::array<Voice,4> voices{};
inline unsigned dropped=0;
inline void complete(SLAndroidSimpleBufferQueueItf,void* context) {
    static_cast<Voice*>(context)->busy.store(false,std::memory_order_release);
}
inline void request() {
    if(!enabled.load(std::memory_order_relaxed)) return;
    unsigned n=pending.load(std::memory_order_relaxed);
    while(n<4 && !pending.compare_exchange_weak(n,n+1,std::memory_order_relaxed)) {}
}
inline bool ok(SLresult r) {if(r==SL_RESULT_SUCCESS)return true;error=r;return false;}
inline void release() {
    for(auto& v:voices) {
        if(v.object) (*v.object)->Destroy(v.object);
        v.object=nullptr;v.play=nullptr;v.queue=nullptr;v.volume=nullptr;v.busy.store(false);
    }
    if(mixObject)(*mixObject)->Destroy(mixObject);
    if(engineObject)(*engineObject)->Destroy(engineObject);
    mixObject=nullptr;engineObject=nullptr;engine=nullptr;
}
inline bool initialize() {
    if(!ok(slCreateEngine(&engineObject,0,nullptr,0,nullptr,nullptr)))return false;
    if(!ok((*engineObject)->Realize(engineObject,SL_BOOLEAN_FALSE)))return false;
    if(!ok((*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engine)))return false;
    if(!ok((*engine)->CreateOutputMix(engine,&mixObject,0,nullptr,nullptr)))return false;
    if(!ok((*mixObject)->Realize(mixObject,SL_BOOLEAN_FALSE)))return false;
    for(auto& v:voices) {
        SLDataLocator_AndroidSimpleBufferQueue locator{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,1};
        SLDataFormat_PCM format{SL_DATAFORMAT_PCM,1,SL_SAMPLINGRATE_44_1,SL_PCMSAMPLEFORMAT_FIXED_16,SL_PCMSAMPLEFORMAT_FIXED_16,SL_SPEAKER_FRONT_CENTER,SL_BYTEORDER_LITTLEENDIAN};
        SLDataSource source{&locator,&format};
        SLDataLocator_OutputMix output{SL_DATALOCATOR_OUTPUTMIX,mixObject};
        SLDataSink sink{&output,nullptr};
        const SLInterfaceID ids[]={SL_IID_ANDROIDSIMPLEBUFFERQUEUE,SL_IID_VOLUME};
        const SLboolean required[]={SL_BOOLEAN_TRUE,SL_BOOLEAN_TRUE};
        if(!ok((*engine)->CreateAudioPlayer(engine,&v.object,&source,&sink,2,ids,required)))return false;
        if(!ok((*v.object)->Realize(v.object,SL_BOOLEAN_FALSE)))return false;
        if(!ok((*v.object)->GetInterface(v.object,SL_IID_PLAY,&v.play)))return false;
        if(!ok((*v.object)->GetInterface(v.object,SL_IID_ANDROIDSIMPLEBUFFERQUEUE,&v.queue)))return false;
        if(!ok((*v.object)->GetInterface(v.object,SL_IID_VOLUME,&v.volume)))return false;
        if(!ok((*v.queue)->RegisterCallback(v.queue,&complete,&v)))return false;
        if(!ok((*v.play)->SetPlayState(v.play,SL_PLAYSTATE_PLAYING)))return false;
    }
    return true;
}
inline void pump(bool active) {
    unsigned n=pending.exchange(0,std::memory_order_relaxed);
    if(!enabled.load() || !active) {
        for(auto& v:voices) if(v.volume)(*v.volume)->SetVolumeLevel(v.volume,SL_MILLIBEL_MIN);
        return;
    }
    if(!n)return;
    if(status==0) {
        if(initialize())status=1;
        else {release();status=-1;}
    }
    if(status!=1)return;
    const SLmillibel level=gain<=.001f?SL_MILLIBEL_MIN:static_cast<SLmillibel>(std::clamp(2000.f*std::log10(gain),float(SL_MILLIBEL_MIN),0.f));
    while(n--) {
        Voice* slot=nullptr;
        for(auto& v:voices) {
            bool expected=false;
            if(v.busy.compare_exchange_strong(expected,true,std::memory_order_acq_rel)) {slot=&v;break;}
        }
        if(!slot){++dropped;continue;}
        if(!ok((*slot->volume)->SetVolumeLevel(slot->volume,level)) || !ok((*slot->queue)->Enqueue(slot->queue,pcm,static_cast<SLuint32>(sizeof(pcm))))) {
            slot->busy.store(false,std::memory_order_release);++dropped;
        }
    }
}
inline void controls() {
    bool on=enabled.load();
    if(ImGui::Checkbox("Hitsound##astra_sound",&on))enabled.store(on);
    ImGui::SliderFloat("Volume##astra_sound",&gain,0.f,1.f,"%.2f");
    if(ImGui::Button("Test sound##astra_sound"))request();
    ImGui::Text("Audio: %s | code: %u | dropped: %u",status==1?"ready":status<0?"failed":"waiting",static_cast<unsigned>(error),dropped);
    if(status<0 && ImGui::Button("Retry audio##astra_sound")){status=0;error=SL_RESULT_SUCCESS;}
}
}
