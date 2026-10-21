#pragma once
// Included inside lemming::esp::detail, after safe memory/pose helpers.
// No Unity/native/managed getter is called by the skeleton reader.
inline constexpr std::array<int, 19> kSkeletonBoneIds{{0,7,8,54,9,10,11,13,15,17,12,14,16,18,1,3,5,2,4}};
inline constexpr std::array<std::array<int,2>, 17> kSkeletonLinks{{
    {{0,1}},{{1,2}},{{2,4}},{{4,5}},{{2,6}},{{6,7}},{{7,8}},{{8,9}},
    {{2,10}},{{10,11}},{{11,12}},{{12,13}},{{0,14}},{{14,15}},{{15,16}},{{0,17}},{{17,18}}
}};
// Right foot is separately appended as bone 19. Enum values are Unity HumanBodyBones.
inline constexpr std::size_t kSkeletonPointCount = 20;
inline std::atomic<bool> g_skeletonEnabled{false};
inline std::atomic<std::uint64_t> g_skeletonCaptures{0}, g_skeletonRejects{0};
inline std::atomic<int> g_skeletonProjected{0};
struct SkeletonPose {
    void* player{}; void* world{};
    std::array<Vec3,kSkeletonPointCount> points{};
    std::uint32_t valid{};
    std::chrono::steady_clock::time_point captured{};
};
inline std::mutex g_skeletonMutex;
inline std::array<SkeletonPose,128> g_skeletonPoses{};
inline std::chrono::steady_clock::time_point g_lastSkeletonLog{};

inline bool skeletonFinite(const Vec3& v) {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
        std::fabs(v.x)<100000&&std::fabs(v.y)<100000&&std::fabs(v.z)<100000;
}
struct SkeletonTrs { float position[4]; float rotation[4]; float scale[4]; };
static_assert(sizeof(SkeletonTrs)==48,"TransformHierarchy TRS stride");
inline bool skeletonApplyParent(const SkeletonTrs& t, Vec3& v) {
    for (float f:t.position) if(!std::isfinite(f))return false;
    for (float f:t.rotation) if(!std::isfinite(f))return false;
    for (float f:t.scale) if(!std::isfinite(f)||std::fabs(f)>1000)return false;
    const float x=t.rotation[0],y=t.rotation[1],z=t.rotation[2],w=t.rotation[3];
    const float norm=x*x+y*y+z*z+w*w;
    if(norm<0.95f||norm>1.05f)return false;
    const Vec3 s{v.x*t.scale[0],v.y*t.scale[1],v.z*t.scale[2]};
    const Vec3 c{2*(y*s.z-z*s.y),2*(z*s.x-x*s.z),2*(x*s.y-y*s.x)};
    v={s.x+w*c.x+(y*c.z-z*c.y)+t.position[0],
       s.y+w*c.y+(z*c.x-x*c.z)+t.position[1],
       s.z+w*c.z+(x*c.y-y*c.x)+t.position[2]};
    return skeletonFinite(v);
}
inline bool skeletonClassNameIs(void* object,const char* expected) {
    // Actual obfuscated IL2CPP class_get_name -> [klass+0x198], verified at 0x632ED6C.
    // Kernel-copy both pointers and text: do not consult the renderer's mutable g_maps.
    void* klass=objectClass(object); if(!klass)return false;
    std::uintptr_t name=0;char text[64]{};const auto size=std::strlen(expected)+1;
    if(size>sizeof(text)||!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(klass)+0x198),name)||!name)return false;
    return safeReadMemory(text,reinterpret_cast<void*>(name),size)==ssize_t(size)&&std::memcmp(text,expected,size)==0;
}
inline bool readSkeletonTransformPosition(void* managed, Vec3& output) {
    if(!managed || !skeletonClassNameIs(managed,"Transform"))return false;
    const auto m=reinterpret_cast<std::uintptr_t>(managed);
    std::uintptr_t native=0, vtable=0, hierarchy=0, matrices=0, parents=0;
    std::int32_t index=-1;
    if(!readValue(reinterpret_cast<void*>(m+0x10),native)||!native ||
       !readValue(reinterpret_cast<void*>(native),vtable)||
       vtable<g_runtime.unityBase+offsets::elf::kDataRelRoBegin||
       vtable>=g_runtime.unityBase+offsets::elf::kDataEnd ||
       !readValue(reinterpret_cast<void*>(native+0x28),hierarchy)||!hierarchy ||
       !readValue(reinterpret_cast<void*>(native+0x30),index)||index<0||index>=8192 ||
       !readValue(reinterpret_cast<void*>(hierarchy+0x18),matrices)||!matrices ||
       !readValue(reinterpret_cast<void*>(hierarchy+0x20),parents)||!parents)return false;
    SkeletonTrs trs{}; std::int32_t parent=-1;
    if(!readValue(reinterpret_cast<void*>(matrices+std::size_t(index)*48),trs)||
       !readValue(reinterpret_cast<void*>(parents+std::size_t(index)*4),parent))return false;
    Vec3 v{trs.position[0],trs.position[1],trs.position[2]};
    if(!skeletonFinite(v))return false;
    std::array<std::int32_t,128> visited{};std::size_t count=0;
    visited[count++]=index;
    while(parent>=0){
        if(parent>=8192||count>=visited.size())return false;
        for(std::size_t i=0;i<count;++i)if(visited[i]==parent)return false;
        visited[count++]=parent;
        if(!readValue(reinterpret_cast<void*>(matrices+std::size_t(parent)*48),trs)||
           !skeletonApplyParent(trs,v)||
           !readValue(reinterpret_cast<void*>(parents+std::size_t(parent)*4),parent))return false;
    }
    if(parent!=-1)return false;
    std::uintptr_t currentNative=0,currentHierarchy=0;std::int32_t currentIndex=-1;
    if(!readValue(reinterpret_cast<void*>(m+0x10),currentNative)||currentNative!=native||
       !readValue(reinterpret_cast<void*>(native+0x28),currentHierarchy)||currentHierarchy!=hierarchy||
       !readValue(reinterpret_cast<void*>(native+0x30),currentIndex)||currentIndex!=index)return false;
    output=v;return true;
}
inline std::size_t skeletonFieldOffset(void* klass,const char* name){
    if(!klass||!g_runtime.unityBase||!g_runtime.api.fieldGetOffset||!g_runtime.api.classGetParent)return 0;
    using GetField=void*(*)(void*,const char*);
    auto getField=reinterpret_cast<GetField>(g_runtime.unityBase+offsets::il2cpp_api::kClassGetFieldFromName);
    for(int i=0;klass&&i<8;++i,klass=g_runtime.api.classGetParent(klass)){
        void* field=getField(klass,name);if(!field)continue;
        auto off=g_runtime.api.fieldGetOffset(field);return off>=0x10&&off<=0x200?off:0;
    }return 0;
}
inline void* skeletonFieldObject(void* object,const char* name){
    auto off=skeletonFieldOffset(objectClass(object),name);
    return off?readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object)+off)):nullptr;
}
struct SkeletonDictionaryEntry { std::int32_t hash,next,key,padding;std::uintptr_t value; };
static_assert(sizeof(SkeletonDictionaryEntry)==24,"Dictionary<int,Transform> ARM64 stride");
inline void captureSkeletonFromCallback(void* player){
    if(!g_skeletonEnabled.load(std::memory_order_relaxed)||!player||!isUnityMainCallback())return;
    if(objectClass(player)!=g_runtime.playerClass)return;
    const auto now=std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(g_skeletonMutex);
        for(const auto& p:g_skeletonPoses)if(p.player==player&&now-p.captured<std::chrono::milliseconds(25))return;
    }
    SkeletonPose candidate{};candidate.player=player;candidate.captured=now;
    const auto base=reinterpret_cast<std::uintptr_t>(player);
    candidate.world=readObject(reinterpret_cast<void*>(base+offsets::field::mfc::kWorld));
    std::uint8_t local=0;
    if(!candidate.world||!readValue(reinterpret_cast<void*>(base+offsets::field::mfc::kLocalOwned),local)||local)return;
    auto finish=[&]{
        std::lock_guard<std::mutex> lock(g_skeletonMutex);SkeletonPose* dest=nullptr;
        for(auto& p:g_skeletonPoses){if(p.player==player){dest=&p;break;}if(!dest&&(!p.player||now-p.captured>std::chrono::seconds(1)))dest=&p;}
        if(dest)*dest=candidate;
        if(candidate.valid)g_skeletonCaptures.fetch_add(1,std::memory_order_relaxed);else g_skeletonRejects.fetch_add(1,std::memory_order_relaxed);
    };
    Vec3 feet{};if(!readPlayerPoseSnapshot(player,feet)){finish();return;}
    void* parts=readObject(reinterpret_cast<void*>(base+offsets::field::mfc::kPartsContainer));
    void* list=parts?readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(parts)+offsets::field::part_container::kPartList)):nullptr;
    void* items=list?readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(list)+offsets::managed::kListItems)):nullptr;
    int count=0;std::uintptr_t length=0;
    if(!list||!items||!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(list)+offsets::managed::kListSize),count)||
       count<1||count>64||!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(items)+offsets::managed::kArrayLength),length)||
       length<std::uintptr_t(count)||length>256){finish();return;}
    void* bindings=nullptr;
    for(int i=0;i<count;++i){
        void* part=readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(items)+offsets::managed::kArrayData+std::size_t(i)*kPtr));
        if(objectClass(part)==g_runtime.modelClass){bindings=readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(part)+offsets::field::jgi::kCharacterBindings));break;}
    }
    if(!bindings||!skeletonClassNameIs(bindings,"CharacterBindings")){finish();return;}
    void* bones=readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(bindings)+offsets::field::character_bindings::kBones));
    if(!bones||!skeletonClassNameIs(bones,"BonesDictionary")){finish();return;}
    auto accept=[&](int id,void* transform){
        std::size_t slot=20;
        for(std::size_t i=0;i<kSkeletonBoneIds.size();++i)if(kSkeletonBoneIds[i]==id){slot=i;break;}
        if(id==6)slot=19;if(slot>=20||candidate.valid&(1u<<slot))return;
        Vec3 v{};if(!readSkeletonTransformPosition(transform,v))return;
        const float dx=v.x-feet.x,dy=v.y-feet.y,dz=v.z-feet.z;
        if(dx*dx+dy*dy+dz*dz>16.0f)return;
        candidate.points[slot]=v;candidate.valid|=1u<<slot;
    };
    void* dict=skeletonFieldObject(bones,"m_dict");
    void* entries=dict?skeletonFieldObject(dict,"_entries"):nullptr;
    auto countOff=skeletonFieldOffset(objectClass(dict),"_count");
    int entryCount=0;
    if(entries&&countOff&&readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(dict)+countOff),entryCount)&&entryCount>0&&entryCount<=128&&
       readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(entries)+offsets::managed::kArrayLength),length)&&length>=std::uintptr_t(entryCount)&&length<=256){
        std::array<SkeletonDictionaryEntry,128> snapshot{};
        if(safeReadMemory(snapshot.data(),reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(entries)+0x20),std::size_t(entryCount)*sizeof(snapshot[0]))==ssize_t(std::size_t(entryCount)*sizeof(snapshot[0])))
            for(int i=0;i<entryCount;++i)if(snapshot[i].hash>=0&&snapshot[i].key>=0&&snapshot[i].key<=54)accept(snapshot[i].key,reinterpret_cast<void*>(snapshot[i].value));
    }
    // Serialized arrays are an alternate real source, not a synthetic skeleton.
    if(!candidate.valid){
        void* keys=skeletonFieldObject(bones,"m_keys");void* values=skeletonFieldObject(bones,"m_values");std::uintptr_t valueCount=0;
        if(keys&&values&&readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(keys)+0x18),length)&&length>0&&length<=128&&
           readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(values)+0x18),valueCount)&&length==valueCount){
            std::array<int,128> ids{};std::array<std::uintptr_t,128> refs{};
            if(safeReadMemory(ids.data(),reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(keys)+0x20),length*4)==ssize_t(length*4)&&
               safeReadMemory(refs.data(),reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(values)+0x20),length*kPtr)==ssize_t(length*kPtr))
                for(std::size_t i=0;i<length;++i)accept(ids[i],reinterpret_cast<void*>(refs[i]));
        }
    }
    finish();
    if(now-g_lastSkeletonLog>std::chrono::seconds(5)){
        g_lastSkeletonLog=now;__android_log_print(ANDROID_LOG_INFO,kLogTag,
            "ESP skeleton snapshot: source=CharacterBindings.BonesDictionary mask=0x%x captures=%llu rejects=%llu nativeGetters=0",candidate.valid,
            (unsigned long long)g_skeletonCaptures.load(),(unsigned long long)g_skeletonRejects.load());
    }
}
inline bool readSkeletonSnapshot(void* player,SkeletonPose& output){
    if(!g_skeletonEnabled.load(std::memory_order_relaxed))return false;
    void* world=readObject(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(player)+offsets::field::mfc::kWorld));
    const auto now=std::chrono::steady_clock::now();std::lock_guard<std::mutex> lock(g_skeletonMutex);
    for(const auto& p:g_skeletonPoses)if(p.player==player&&p.world==world&&p.valid&&now-p.captured<std::chrono::milliseconds(150)){output=p;return true;}
    return false;
}
