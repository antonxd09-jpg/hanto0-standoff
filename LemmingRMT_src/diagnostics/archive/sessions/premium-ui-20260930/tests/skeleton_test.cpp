#include "esp_runtime.hpp"
#include <cassert>
#include <limits>
#include <iostream>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
using namespace lemming::esp;using namespace lemming::esp::detail;
template<class T>void put(std::vector<unsigned char>& v,std::size_t off,T x){assert(off+sizeof(T)<=v.size());std::memcpy(v.data()+off,&x,sizeof(T));}
int main(){
 std::vector<unsigned char> klass(0x220),managed(0x30),native(0x70),hierarchy(0x40),player(0x100);
 std::array<std::uintptr_t,64> vt{};std::array<SkeletonTrs,3> trs{};std::array<int,3> parents{{1,-1,-1}};
 static const char name[]="Transform";put(klass,0x198,static_cast<const char*>(name));
 put(managed,0,static_cast<void*>(klass.data()));put(managed,0x10,static_cast<void*>(native.data()));
 g_runtime.unityBase=reinterpret_cast<std::uintptr_t>(vt.data())-lemming::offsets::elf::kDataRelRoBegin;
 put(native,0,reinterpret_cast<std::uintptr_t>(vt.data()));put(native,0x28,static_cast<void*>(hierarchy.data()));put(native,0x30,std::int32_t{0});
 put(hierarchy,0x18,reinterpret_cast<std::uintptr_t>(trs.data()));put(hierarchy,0x20,reinterpret_cast<std::uintptr_t>(parents.data()));
 trs[0].position[0]=1;trs[1].position[0]=10;trs[1].rotation[2]=std::sqrt(.5f);trs[1].rotation[3]=std::sqrt(.5f);
 for(auto& t:trs)for(int i=0;i<3;++i)t.scale[i]=2;
 Vec3 p{};assert(readSkeletonTransformPosition(managed.data(),p));assert(std::fabs(p.x-10)<.0001f&&std::fabs(p.y-2)<.0001f&&std::fabs(p.z)<.0001f);
 parents[1]=1;assert(!readSkeletonTransformPosition(managed.data(),p));parents[1]=-1;
 parents[0]=9000;assert(!readSkeletonTransformPosition(managed.data(),p));parents[0]=1;
 put(native,0x30,std::int32_t{9000});assert(!readSkeletonTransformPosition(managed.data(),p));put(native,0x30,std::int32_t{0});
 trs[1].rotation[3]=0;assert(!readSkeletonTransformPosition(managed.data(),p));trs[1].rotation[3]=std::sqrt(.5f);
 trs[0].position[0]=std::numeric_limits<float>::quiet_NaN();assert(!readSkeletonTransformPosition(managed.data(),p));trs[0].position[0]=1;
 put(managed,0x10,std::uintptr_t{0});assert(!readSkeletonTransformPosition(managed.data(),p));put(managed,0x10,static_cast<void*>(native.data()));
 put(klass,0x198,std::uintptr_t{0x1234});assert(!readSkeletonTransformPosition(managed.data(),p));put(klass,0x198,static_cast<const char*>(name));
 assert(!readSkeletonTransformPosition(reinterpret_cast<void*>(0x1234),p));
 int world;put(player,lemming::offsets::field::mfc::kWorld,static_cast<void*>(&world));
 SkeletonPose pose{};pose.player=player.data();pose.world=&world;pose.valid=1;pose.points[0]={1,2,3};pose.captured=std::chrono::steady_clock::now();g_skeletonPoses[0]=pose;g_skeletonEnabled=true;
 SkeletonPose output{};assert(readSkeletonSnapshot(player.data(),output)&&output.valid==1);g_skeletonPoses[0].captured-=std::chrono::seconds(1);assert(!readSkeletonSnapshot(player.data(),output));
 g_skeletonPoses[0]=pose;put(player,lemming::offsets::field::mfc::kWorld,static_cast<void*>(native.data()));assert(!readSkeletonSnapshot(player.data(),output));
 prctl(PR_SET_NAME,"UnityGfxDeviceW",0,0,0);captureSkeletonFromCallback(player.data());assert(g_skeletonCaptures==0);g_skeletonEnabled=false;captureSkeletonFromCallback(player.data());assert(g_skeletonCaptures==0);
 std::cout<<"PASS real TRS translation/rotation/non-uniform scale composition, cycles, bounds, NaN, quaternion, invalid managed/native/class pointers, expiry/world identity and wrong-thread rejection; no Unity getter calls\n";
}
