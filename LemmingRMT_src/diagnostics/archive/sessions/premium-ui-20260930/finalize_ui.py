from pathlib import Path
P=Path('/home/leftcode/Projects/LemmingRMT')
s=(P/'jni/skeleton_runtime.hpp').read_text()
helper='''inline bool skeletonClassNameIs(void* object,const char* expected) {
    // Actual obfuscated IL2CPP class_get_name -> [klass+0x198], verified at 0x632ED6C.
    // Kernel-copy both pointers and text: do not consult the renderer's mutable g_maps.
    void* klass=objectClass(object); if(!klass)return false;
    std::uintptr_t name=0;char text[64]{};const auto size=std::strlen(expected)+1;
    if(size>sizeof(text)||!readValue(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(klass)+0x198),name)||!name)return false;
    return safeReadMemory(text,reinterpret_cast<void*>(name),size)==ssize_t(size)&&std::memcmp(text,expected,size)==0;
}
'''
a='inline bool readSkeletonTransformPosition(void* managed, Vec3& output) {'
assert s.count(a)==1;s=s.replace(a,helper+a,1)
s=s.replace('nameEquals(className(objectClass(managed)),"Transform")','skeletonClassNameIs(managed,"Transform")').replace('nameEquals(className(objectClass(bindings)),"CharacterBindings")','skeletonClassNameIs(bindings,"CharacterBindings")').replace('nameEquals(className(objectClass(bones)),"BonesDictionary")','skeletonClassNameIs(bones,"BonesDictionary")')
(P/'jni/skeleton_runtime.hpp').write_text(s)
s=(P/'jni/main.cpp').read_text();a=s.index('bool inside(float x, float y, float left, float top, float width, float height) {');b=s.index('void handleTouch(',a);s=s[:a]+s[b:];(P/'jni/main.cpp').write_text(s)
with (P/'Standoff2_1.0.0_ARM64_internal_offsets.txt').open('a') as f:f.write('IL2CPP_CLASS_NAME_ACTUAL_LAYOUT=0x198 (class_get_name 0x6327AD8 -> 0x632ED6C)\nSKELETON_CLASS_NAME_READ=kernel safe copies, no shared maps\n')
print('Finalized safe class-name reader and removed obsolete touch hitbox')
