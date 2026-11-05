"""Static checks only. Usage: python checks/verify_offsets_110.py /path/to/1.1.0/dump"""
import argparse,json,re,struct,hashlib,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('dump_directory',type=Path);args=p.parse_args()
project=Path(__file__).resolve().parents[1];src=project/'jni/src';d=args.dump_directory
b=(d/'libunity.so').read_bytes();script=json.loads((d/'script.json').read_text());cs=(d/'dump.cs').read_text()
assert b[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',b,18)[0]==183
expected=bytes.fromhex('040000000800000003000000474e550069815f2befe135c2')
assert b[0x270:0x288]==expected,'Wrong binary Build ID'
phoff=struct.unpack_from('<Q',b,32)[0];psz,pnum=struct.unpack_from('<HH',b,54);loads=[]
for i in range(pnum):
 t,f,o,v,_,fs,ms,_=struct.unpack_from('<IIQQQQQQ',b,phoff+i*psz)
 if t==1:loads.append((v,v+fs,v+ms,o,f))
def raw(a,n):
 for lo,hi,mem,o,flags in loads:
  if lo<=a and a+n<=hi:return b[o+a-lo:o+a-lo+n]
 raise AssertionError(hex(a)+' is not file backed')
def executable(a):return a%4==0 and any(lo<=a<hi and f&1 for lo,hi,mem,o,f in loads)
header=(src/'game/offsets.hpp').read_text()
for key,file in [('kSourceSha256','script.json'),('kDumpCsSha256','dump.cs'),('kIl2CppHeaderSha256','il2cpp.h'),('kStringLiteralSha256','stringliteral.json'),('kLibUnitySha256','libunity.so')]:
 assert re.search(key+r'\[\] = "([^"]+)"',header)[1]==hashlib.sha256((d/file).read_bytes()).hexdigest(),key
records=json.loads((project/'checks/port_changes.json').read_text())
methods={x['Address'] for x in script['ScriptMethod']};metadata={x['Address'] for x in script['ScriptMetadata']}
for e in records:
 a=int(e['new'],16)
 if e.get('key','').startswith(('typeinfo::','typevar::')):assert a in metadata,e
 else:
  assert executable(a),e
  if e['evidence'] not in ['native-api-wrapper','native-icall-registration']:assert a in methods,e
# Exact TypeInfo/name pairs used by ESP, with no transitive rename chains.
classkeys={'kClientWorld':'ggk','kRegistry':'nba','kStore':'ibc','kNetObject':'wwv','kPlayer':'odq','kTransforms':'mtc','kModel':'bis','kStats':'ydm','kWeaponPart':'uyg','kWeaponInstance':'jrx'}
meta={e['Name']:e['Address'] for e in script['ScriptMetadata']}
esp=(src/'features/visuals/esp_runtime.hpp').read_text()
for key,name in classkeys.items():
 a=int(re.search(r'\b'+key+r' = (0x[\da-fA-F]+)',header)[1],16)
 assert a==meta[name+'_TypeInfo'],(key,name)
 assert 'classFromSlot(offsets::typeinfo::'+key+', "'+name+'")' in esp
assert 'kRuntimeVtableOffset = 0x1B0' in esp
assert 'targetMethodSlot = 50;' in esp and '"fixo"' in esp
assert 'methodInfo + offsets::method_info::kMethodPointer' in esp
assert 'UnityEngine' in cs and 'public class odq :' in cs
# Verify instruction guards against actual 1.1.0 bytes, including in-function event sites.
cam=(src/'features/visuals/astra_camera_test.hpp').read_text()
a=int(re.search(r'kSyzn\s*=\s*(0x[\da-fA-F]+)',cam)[1],16)
expected_words=tuple(int(x,16) for x in re.search(r'wanted\{([^}]+)',cam)[1].split(', '))
assert raw(a,16)==struct.pack('<4I',*expected_words)
hit=(src/'features/visuals/astra_hitlog.hpp').read_text()
for name,a in [('expectedCode',0xAA3EAFC),('signature',0xA78D3CC)]:
 w=[int(x.strip(),16) for x in re.search(name+r'\{([^}]+)',hit)[1].split(',')];assert raw(a,16)==struct.pack('<4I',*w)
assert 'eventRva = 0xAA3EAFC' in hit and '0xA78D3CC' in hit
night=(src/'features/visuals/astra_night.hpp').read_text();start=night.index('const C checks[]');end=night.index('for(auto c:checks)',start)
for a,w in re.findall(r'\{(0x[\da-fA-F]+),(0x[\da-fA-F]+)\}',night[start:end]):assert raw(int(a,16),4)==struct.pack('<I',int(w,16))
core=(src/'core/entry.cpp').read_text();gui=(src/'ui/kazakh/menu.hpp').read_text()
assert 'unityProfileMatches(base)' in core and 'actual == expected' in core
assert 'Binary profile: %s' in gui and 'waiting libunity.so' in gui and 'Camera cache: %s' in gui
assert 'kTabs[] = {"visuals", "silent", "config"}' in gui
assert 'kProfileName[] = "Standoff2-1.1.0-arm64-visuals"' in header
# Execute the actual profile matcher against a bounded host memory reader.
start=core.index('bool unityProfileMatches(');end=core.index('\nvoid configureImGuiStyle()',start)
matcher=core[start:end]
host_prefix = r'''#include <array>
#include <cstring>
#include <cassert>
#include <cstdint>
static uintptr_t beginAddress=0, endAddress=0;
namespace astra::esp::detail {
template<class T> bool readValue(const void* address,T& out){
    auto a=reinterpret_cast<uintptr_t>(address);
    if(a<beginAddress || a>endAddress || sizeof(T)>endAddress-a)return false;
    std::memcpy(&out,address,sizeof(T));return true;
}}
'''
host_main=r'''int main(){
    std::array<unsigned char,0x288> bytes{};
    const unsigned char note[]={4,0,0,0,8,0,0,0,3,0,0,0,'G','N','U',0,0x69,0x81,0x5f,0x2b,0xef,0xe1,0x35,0xc2};
    std::memcpy(bytes.data()+0x270,note,sizeof(note));
    beginAddress=reinterpret_cast<uintptr_t>(bytes.data());endAddress=beginAddress+bytes.size();
    assert(unityProfileMatches(beginAddress));
    bytes[0x280]^=1;assert(!unityProfileMatches(beginAddress));
    bytes[0x280]^=1;bytes[0x270]=8;assert(!unityProfileMatches(beginAddress));
    assert(!unityProfileMatches(0));assert(!unityProfileMatches(endAddress));
}
'''
# Host checks of actual offset header and profile matcher; not an Android build.
with tempfile.TemporaryDirectory() as td:
 test=Path(td)/'offsets.cpp';test.write_text('#include "'+str(src/'game/offsets.hpp')+'"\nstatic_assert(astra::offsets::field::nrk::kActiveWeapon==0x80);\nstatic_assert(astra::offsets::method_info::kMethodPointer==0x68);\n')
 subprocess.run(['g++','-std=c++17','-fsyntax-only',str(test)],check=True)
 test.write_text(host_prefix+matcher+host_main)
 exe=Path(td)/'profile_test'
 subprocess.run(['g++','-std=c++17',str(test),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS:',len(records),'recorded RVA changes; TypeInfo identities; provenance hashes; ARM64 ELF ranges; instruction guards; profile/diagnostic wiring; host header syntax.')
print('Not tested: Android compilation, runtime ABI, game behaviour or unload.')
