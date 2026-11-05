#pragma once
#include "features/aim/core.hpp"
namespace astra::aim::silent_command {
// Command memory is pooled: its address alone is never an identity.
struct Identity {
 std::uintptr_t command=0,player=0,world=0,weapon=0;
 std::uint32_t token=0;
};
inline bool same(const Identity& a,const Identity& b){
 return a.command&&a.player&&a.world&&a.weapon&&a.command==b.command&&
 a.token==b.token&&a.player==b.player&&a.world==b.world&&a.weapon==b.weapon;
}
struct Plan {
 bool valid=false;Identity identity{};double captured=0;
 Selection selection{};Vec3 origin{},point{};Angles angle{};std::int32_t collider=0;
 bool matches(const Identity& current,double now,bool enabled,bool paused)const{
  return valid&&enabled&&!paused&&std::isfinite(now)&&std::isfinite(captured)&&
   now>=captured&&now-captured<=60.0&&same(identity,current);
 }
};
inline bool originMatches(Vec3 a,Vec3 b){return finite(a)&&finite(b)&&length(sub(a,b))<=.1f;}
}
