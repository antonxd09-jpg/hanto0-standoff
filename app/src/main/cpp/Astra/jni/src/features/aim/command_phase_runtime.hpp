// Included in the runtime namespace after processAutomation.
using CommandGateFn=bool(*)(void*,void*,const void*);
using CommandEndFn=void(*)(void*,const void*);
inline std::atomic<CommandGateFn> g_commandGateOriginal{nullptr};
inline std::atomic<CommandEndFn> g_commandEndOriginal{nullptr};
inline std::array<void*,2> g_commandRelays{};
inline bool g_lookHookReady=false;
inline bool commandGate(void* self,void* cmd,const void* method){
 debug::commandGateCalls.fetch_add(1);
 bool ours=ed::isUnityMainCallback()&&named(self,"eyc")&&object(self,0x18)==localPlayer();
 if(ours){g_silentPlan.valid=false;debug::commandOurs.fetch_add(1);g_liveCommand=cmd;g_liveCommandAt=autoNow();}
 if(ours&&g_inputHookReady.load()){
  std::uint32_t token=0;field(cmd,0x10,token);
  if(g_phaseLease.cmd){if(g_phaseLease.self==self&&g_phaseLease.cmd==cmd&&g_phaseLease.token==token)finishCommandPhase(self);else discardCommandPhase();}
  if(beginCommandPhase(self,cmd)){debug::commandBegin.fetch_add(1);debug::inputCalls.fetch_add(1);if(configuration().autoFire.enabled)processAutomation(self,cmd);else silentCommandSpoof(cmd);}
  else if(named(cmd,"ubz")){debug::silentNoPhase.fetch_add(1);silentCommandSpoof(cmd);}
 }
 auto original=g_commandGateOriginal.load(std::memory_order_acquire);
 bool accepted=original?original(self,cmd,method):false;
 if(ours&&!accepted)g_silentPlan.valid=false;
 if(ours&&!accepted&&g_ownedAttack)debug::commandRejected.fetch_add(1);
 return accepted;
}
inline void commandEnd(void* self,const void* method){
 debug::commandEndCalls.fetch_add(1);
 auto original=g_commandEndOriginal.load(std::memory_order_acquire);
 if(original)original(self,method);
 if(ed::isUnityMainCallback())finishCommandPhase(self);
}
// ios::lxcq writes the command look from the camera; redirect that BL so the
// spoofer's angle survives regardless of component tick order.
inline bool installLookHook(){
#if defined(__aarch64__)
 if(g_lookHookReady)return true;
 auto b=ed::g_runtime.unityBase;
 if(!bytesMatch(b+silent_offsets::kCommandLook,{0xd10103ff,0xa90257fe,0xa9034ff4,0xf0028675})){debug::log("look hook: setter signature mismatch");return false;}
 auto site=b+silent_offsets::kCameraLookWrite;std::uint32_t word=0;
 if(!ed::readValue(reinterpret_cast<void*>(site),word)||word!=silent_offsets::kCameraLookWriteWord){debug::log("look hook: call-site mismatch %08x",word);return false;}
 long rawPage=sysconf(_SC_PAGESIZE);if(rawPage<=0||!hook_support::pageSizeValid(std::uintptr_t(rawPage)))return false;
 auto pageSize=std::uintptr_t(rawPage);hook_support::AllocationStats stats{};void* relay=hook_support::allocateNear(site,pageSize,stats);
 if(relay==MAP_FAILED){debug::log("look hook: relay allocation failed errno=%d",errno);return false;}
 std::uint32_t branch=0;if(!hook_support::branchWord(site,reinterpret_cast<std::uintptr_t>(relay),branch)){munmap(relay,pageSize);return false;}
 std::uint32_t instructions[2]={0x58000051,0xd61f0220};auto replacement=reinterpret_cast<std::uintptr_t>(&commandLookWrite);
 std::memcpy(relay,instructions,8);std::memcpy(static_cast<char*>(relay)+8,&replacement,8);__builtin___clear_cache(static_cast<char*>(relay),static_cast<char*>(relay)+16);
 if(mprotect(relay,pageSize,PROT_READ|PROT_EXEC)){munmap(relay,pageSize);return false;}
 auto page=site&~(pageSize-1);
 if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_WRITE|PROT_EXEC)){munmap(relay,pageSize);debug::log("look hook: text RWX denied errno=%d",errno);return false;}
 std::uint32_t current=0;if(!ed::readValue(reinterpret_cast<void*>(site),current)||current!=word){mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC);munmap(relay,pageSize);return false;}
 __atomic_store_n(reinterpret_cast<std::uint32_t*>(site),branch,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));
 if(mprotect(reinterpret_cast<void*>(page),pageSize,PROT_READ|PROT_EXEC)){__atomic_store_n(reinterpret_cast<std::uint32_t*>(site),word,__ATOMIC_RELEASE);__builtin___clear_cache(reinterpret_cast<char*>(site),reinterpret_cast<char*>(site+4));munmap(relay,pageSize);return false;}
 g_lookHookReady=true;debug::commandLookHookReady.store(1);debug::log("command look hook ready site=0x%llx relay=%p",(unsigned long long)(site-b),relay);return true;
#else
 return false;
#endif
}
inline bool installInput(void* p){
 if(g_inputHookReady.load()){g_inputInstallReason.store(input_init::Ready);return true;}
 auto attempt=debug::inputAttempts.fetch_add(1)+1;
 auto result=[&](int code){int previous=g_inputInstallReason.exchange(code);if(previous!=code||attempt<=3||attempt%10==0)debug::log("input install attempt=%llu backend=ltc-command-phase reason=%s local=%p",(unsigned long long)attempt,input_init::reasonText(code),p);return code==input_init::Ready;};
 if(!ed::isUnityMainCallback())return result(input_init::WrongThread);
 if(!p||p!=localPlayer())return result(input_init::NoLocal);
 if(!ed::g_runtime.ready||!g_functionsValid)return result(input_init::RuntimeNotReady);
#if defined(__aarch64__)
 ed::PlayerSnapshot local{};void* world=nullptr;if(!localAlive(p,local,world))return result(input_init::InvalidLocal);
 auto b=ed::g_runtime.unityBase;
 if(!(bytesMatch(b+silent_offsets::kCommandGate,{0xa9be57fe,0xa9014ff4,0xb001a295,0xaa0103f4})&&bytesMatch(b+silent_offsets::kCommandEnd,{0xa9be57fe,0xa9014ff4,0xb001a233,0xaa0003f4})&&bytesMatch(b+silent_offsets::kCommandLook,{0xd10103ff,0xa90257fe,0xa9034ff4,0xf0028675})))return result(input_init::SignatureRejected);
 if(!installLookHook())return result(input_init::CallsiteRejected);
 long raw=sysconf(_SC_PAGESIZE);if(raw<=0||!hook_support::pageSizeValid(std::uintptr_t(raw)))return result(input_init::TextProtectionFailed);
 auto pageSize=std::uintptr_t(raw);
 std::array<std::uintptr_t,2> sites{b+silent_offsets::kCommandGateSite,b+silent_offsets::kCommandEndSite};
 std::array<std::uint32_t,2> old{silent_offsets::kCommandGateSiteWord,silent_offsets::kCommandEndSiteWord},branches{};
 std::array<std::uintptr_t,2> destinations{reinterpret_cast<std::uintptr_t>(&commandGate),reinterpret_cast<std::uintptr_t>(&commandEnd)};
 std::array<void*,2> relays{};
 auto drop=[&](){for(auto* r:relays)if(r&&r!=MAP_FAILED)munmap(r,pageSize);};
 for(unsigned i=0;i<2;++i){hook_support::AllocationStats stats{};relays[i]=hook_support::allocateNear(sites[i],pageSize,stats);if(relays[i]==MAP_FAILED){drop();return result(input_init::RelayAllocationFailed);}if(!hook_support::branchWord(sites[i],reinterpret_cast<std::uintptr_t>(relays[i]),branches[i])){drop();return result(input_init::RelayAllocationFailed);}std::uint32_t instructions[2]{0x58000051,0xd61f0220};std::memcpy(relays[i],instructions,8);std::memcpy(static_cast<char*>(relays[i])+8,&destinations[i],8);__builtin___clear_cache(static_cast<char*>(relays[i]),static_cast<char*>(relays[i])+16);if(mprotect(relays[i],pageSize,PROT_READ|PROT_EXEC)){drop();return result(input_init::RelayProtectionFailed);}}
  g_commandLook=reinterpret_cast<CommandLookFn>(b+silent_offsets::kCommandLook);
  g_commandGateOriginal.store(reinterpret_cast<CommandGateFn>(b+silent_offsets::kCommandGate),std::memory_order_release);g_commandEndOriginal.store(reinterpret_cast<CommandEndFn>(b+silent_offsets::kCommandEnd),std::memory_order_release);
 struct Ops {bool read(std::uintptr_t p,std::uint32_t& v){return ed::readValue(reinterpret_cast<void*>(p),v);}bool rwx(std::uintptr_t p,std::uintptr_t n){return mprotect(reinterpret_cast<void*>(p),n,PROT_READ|PROT_WRITE|PROT_EXEC)==0;}bool rx(std::uintptr_t p,std::uintptr_t n){return mprotect(reinterpret_cast<void*>(p),n,PROT_READ|PROT_EXEC)==0;}void write(std::uintptr_t p,std::uint32_t v){__atomic_store_n(reinterpret_cast<std::uint32_t*>(p),v,__ATOMIC_RELEASE);}void flush(std::uintptr_t p){__builtin___clear_cache(reinterpret_cast<char*>(p),reinterpret_cast<char*>(p+4));}} ops;
 auto installed=native_pair::apply(ops,sites,old,branches,pageSize);
 if(installed!=native_pair::Ready){
  std::uint32_t a=0,z=0;bool untouched=ops.read(sites[0],a)&&ops.read(sites[1],z)&&a==old[0]&&z==old[1];
  if(untouched)drop();else{g_commandRelays=relays;debug::log("input pair rollback incomplete; relays retained, automation fail-closed");}
  return result(installed==native_pair::Changed?input_init::CallsiteRejected:(installed==native_pair::WritableDenied?input_init::TextProtectionFailed:(installed==native_pair::RestoreDenied?input_init::RestoreProtectionFailed:input_init::PairReadbackFailed)));
 }
 g_commandRelays=relays;if(!installLookHook())debug::log("command look hook unavailable: silent look may be overwritten by ios::lxcq");g_inputHookReady.store(true,std::memory_order_release);debug::log("input hook ready backend=ltc-command-phase: before attack gate / after late WeaponAction; exact ownership retained");return result(input_init::Ready);
#else
 return result(input_init::UnsupportedArchitecture);
#endif
}
