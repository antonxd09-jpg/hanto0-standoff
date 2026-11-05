"""Verify Silent-specific 1.1.0 addresses, instruction guards, BL targets and layout evidence.
Usage: python checks/verify_silent_110.py /path/to/dump
"""
from pathlib import Path
import argparse, json, re, struct, hashlib
p=argparse.ArgumentParser();p.add_argument('dump',type=Path);d=p.parse_args().dump
root=Path(__file__).resolve().parents[1];src=root/'jni/src';b=(d/'libunity.so').read_bytes()
assert hashlib.sha256(b).hexdigest()=='ba0966976204fa3f7a2755c9588d32299fe7eb794b712c9f1a444e046cbe7c13'
off=struct.unpack_from('<Q',b,32)[0];size,count=struct.unpack_from('<HH',b,54)
loads=[struct.unpack_from('<IIQQQQQQ',b,off+i*size) for i in range(count)]
def raw(a,n):
 for t,f,o,v,_,fs,ms,_ in loads:
  if t==1 and f&1 and v<=a and a+n<=v+fs:return b[o+a-v:o+a-v+n]
 raise AssertionError(hex(a))
h=(src/'features/aim/silent_offsets.hpp').read_text();vals={k:int(v,16) for k,v in re.findall(r'uintptr_t (\w+)=(0x[\da-fA-F]+)',h)}
for record in json.loads((root/'checks/silent_110_guards.json').read_text()):
 a=vals[record['key']];words=[int(w,16) for w in record['words']]
 assert a==int(record['rva'],16) and raw(a,4*len(words))==struct.pack('<'+'I'*len(words),*words)
for site,target in [('kShotCallsite','kShotEmit'),('kCommandGateSite','kCommandGate'),('kCommandEndSite','kCommandEnd'),('kCameraLookWrite','kCommandLook')]:
 a=vals[site];w=struct.unpack('<I',raw(a,4))[0];assert w>>26==0b100101
 imm=w&0x3ffffff;imm=imm-(1<<26) if imm&(1<<25) else imm
 assert a+imm*4==vals[target]
 assert int(re.search(site+r'Word=(0x[\da-f]+)',h)[1],16)==w
cs=(d/'dump.cs').read_text()
for cls in ['kaa','eyc','ubz','bis','mtc']:assert re.search(r'public class '+cls+r'\b',cs)
for method,a in [('tfqa',vals['kShotEmit']),('uduz',vals['kCommandGate']),('dfee',vals['kCommandEnd']),('syha',vals['kCommandLook'])]:
 assert re.search(r'// RVA: 0x'+format(a,'X')+r' .*\n\s*(?:private|public).*?\b'+method+r'\(',cs)
assert 'public Transform HitTransform; // 0x18' in cs and 'public jrx ActiveWeapon; // 0x50' in cs
runtime=(src/'features/aim/runtime.hpp').read_text();cmd=(src/'features/aim/command_phase_runtime.hpp').read_text()
assert 'object(ctx,0x50)==local.activeWeapon' in runtime
assert 'silentShotPlan(p,world,local.activeWeapon,first.origin' in runtime
assert 'offsets::field::nrk::kHitTransform),origin,forward' in (src/'features/aim/automation_runtime.hpp').read_text()
assert 'else silentCommandSpoof(cmd)' in cmd and 'if(!installLookHook())return' in cmd
print('PASS: binary hash, Silent guards, four BL targets, method identities, context layout, independent command route.')
print('LIMIT: static validation; Android build and live game test are still required.')
