"""Regression: execute real command identity/plan policy and commandLookWrite.
No Android SDK needed. Uses g++ and a temporary Android log declaration.
"""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];src=root/'jni/src'
s=(src/'features/aim/automation_runtime.hpp').read_text()
def function(name):
 start=s.index('inline ',s.index(name)-50) if False else s.rfind('inline ',0,s.index(name+'('))
 body=s.index('{',start);level=1;end=body+1
 while level:
  level+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
cpp=r'''
#include "features/aim/silent_command.hpp"
#include "features/aim/debug.hpp"
#include <cstring>
#include <cassert>
using namespace astra::aim;
namespace ed {struct PlayerSnapshot{void* activeWeapon=nullptr;};inline bool isUnityMainCallback(){return true;}}
struct CommandLook {std::uint8_t has=1,padding[3]{};float pitch=0,yaw=0;};
alignas(16) unsigned char cmd[128]{};
void* player=reinterpret_cast<void*>(0x2000);void* worldValue=reinterpret_cast<void*>(0x3000);
void* weapon=reinterpret_cast<void*>(0x4000);double milliseconds=100;
silent_command::Plan g_silentPlan{};
void* localPlayer(){return player;}
bool localAlive(void*,ed::PlayerSnapshot& local,void*& world){local.activeWeapon=weapon;world=worldValue;return true;}
bool named(void* p,const char*){return p==cmd;}
template<class T>bool field(void* p,size_t off,T& out){if(p!=cmd||off+sizeof(T)>sizeof(cmd))return false;std::memcpy(&out,cmd+off,sizeof(T));return true;}
double autoNow(){return milliseconds;}
CommandLook received{};unsigned calls=0;
void setter(void*,CommandLook v,const void*){received=v;++calls;}
inline auto g_commandLook=&setter;
'''
cpp+='\n'.join(function(n) for n in ['silentIdentity','currentSilentPlan','commandLookWrite'])
cpp+=r'''
void prepare(std::uint32_t token=7){
 std::memcpy(cmd+0x10,&token,4);milliseconds=100;
 auto c=Configuration{};c.silent.enabled=true;c.pauseInMenu=true;publish(c);g_menuOpen=false;
 g_silentPlan={};g_silentPlan.valid=true;
 g_silentPlan.identity=silentIdentity(cmd,player,worldValue,weapon);
 g_silentPlan.captured=100;g_silentPlan.angle={10,20};
}
void write(){unsigned before=calls;commandLookWrite(cmd,CommandLook{1,{},1,2},nullptr);assert(calls==before+1);}
void unchanged(){write();assert(received.pitch==1&&received.yaw==2);}
int main(){
 prepare();write();assert(received.pitch==10&&received.yaw==20);
 // Same address, different command generation must forward the incoming camera angle.
 prepare();std::uint32_t next=8;std::memcpy(cmd+0x10,&next,4);unchanged();assert(!g_silentPlan.valid);
 prepare();milliseconds=161;unchanged();
 prepare();milliseconds=99;unchanged();
 prepare();g_menuOpen=true;unchanged();
 prepare();auto c=configuration();c.silent.enabled=false;publish(c);unchanged();
 prepare();weapon=reinterpret_cast<void*>(0x5000);unchanged();weapon=reinterpret_cast<void*>(0x4000);
 prepare();worldValue=reinterpret_cast<void*>(0x6000);unchanged();worldValue=reinterpret_cast<void*>(0x3000);
 prepare();player=reinterpret_cast<void*>(0x7000);unchanged();player=reinterpret_cast<void*>(0x2000);
 prepare(0);write();assert(received.pitch==10); // zero is a valid generation
 assert(silent_command::originMatches({1,2,3},{1.01f,2,3}));
 assert(!silent_command::originMatches({1,2,3},{2,2,3}));
 for(Vec3 point: {Vec3{2,3,40},Vec3{-20,10,3},Vec3{0,-10,5}}){
  auto a=anglesFor(point);auto direction=forwardFor(a);
  assert(dot(direction,normalized(point))>.99999f);
 }
}
'''
# Integration guard: the actual ray hook consumes the command plan and has no
# independent target-selection or prediction fallback anymore.
runtime=(src/'features/aim/runtime.hpp').read_text();emit=runtime[runtime.index('inline void emitRays('):runtime.index('inline bool bytesMatch(',runtime.index('inline void emitRays('))]
assert 'silentShotPlan(' in emit and 'selectSilent(' not in emit and 'predictPoint(' not in emit
assert emit.count('g_emitOriginal(self,ctx,rays,span)')==1
with tempfile.TemporaryDirectory() as td:
 td=Path(td);(td/'android').mkdir();(td/'android/log.h').write_text('#define ANDROID_LOG_INFO 4\nextern "C" int __android_log_print(int,const char*,const char*,...);\n')
 (td/'sync.cpp').write_text(cpp)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-I',str(src),'-I',str(td),str(td/'sync.cpp'),'-o',str(td/'test')],check=True)
 subprocess.run([str(td/'test')],check=True)
print('PASS: actual look hook: recycled command, time bounds, pause, disable, owner/world/weapon switch, zero token, original forwarding; direction agreement; ray-hook routing.')
