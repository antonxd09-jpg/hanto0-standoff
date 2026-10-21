#pragma once

#include <cstddef>
#include <cstdint>

namespace lemming::offsets {

inline constexpr char kProfileName[] = "Standoff2-1.0.0-arm64-apk-ida-v2";
inline constexpr char kSourceSha256[] = "cd824a193f10dd8787a3c3bac2a78c8b1b5b1e91bcc0bf56b7043e97c6222f22";
inline constexpr char kApkSha256[] = "863073036c0fcf6532906d50c087677e5fcc5cf0b2566aa98e10eb5f7a2ef0a2";
inline constexpr char kLibUnitySha256[] = "a5d530e0ee7f4982d4bafec50c6109737b6119525935cac3e423e5145ff33e0f";

namespace elf {
inline constexpr std::uintptr_t kTextBegin = 0x535F940;
inline constexpr std::uintptr_t kTextEnd = 0xB882E00;
inline constexpr std::uintptr_t kDataBegin = 0xBF07F60;
inline constexpr std::uintptr_t kDataEnd = 0xC46CF80;
}

// ARM64 IL2CPP entry points recovered from the matching 1.0.0 libunity.so.
// Prefer dlsym where available; use these only after the ARM64 profile gate passes.
namespace il2cpp_api {
inline constexpr std::uintptr_t kResolveIcall = 0x63279F4;
inline constexpr std::uintptr_t kFree = 0x63279F8;
inline constexpr std::uintptr_t kAssemblyGetImage = 0x6327A18;
inline constexpr std::uintptr_t kClassFromName = 0x6327A3C;
inline constexpr std::uintptr_t kClassGetFields = 0x6327AC8;
inline constexpr std::uintptr_t kClassGetFieldFromName = 0x6327AD0;
inline constexpr std::uintptr_t kClassGetName = 0x6327AD8;
inline constexpr std::uintptr_t kClassGetNamespace = 0x6327ADC;
inline constexpr std::uintptr_t kClassGetParent = 0x6327AE0;
inline constexpr std::uintptr_t kDomainGet = 0x6327B70;
inline constexpr std::uintptr_t kDomainAssemblyOpen = 0x6327B74;
inline constexpr std::uintptr_t kFieldGetOffset = 0x6327C44;
inline constexpr std::uintptr_t kFieldGetType = 0x6327C48;
inline constexpr std::uintptr_t kFieldStaticGetValue = 0x6327C50;
inline constexpr std::uintptr_t kThreadAttach = 0x6327D70;
inline constexpr std::uintptr_t kTypeGetName = 0x6327D8C;
}

namespace unity {
inline constexpr std::uintptr_t kCameraGetMain = 0x53653C4;
inline constexpr std::uintptr_t kCameraGetCurrent = 0x53653E8;
inline constexpr std::uintptr_t kCameraGetFov = 0x5364FE4;
inline constexpr std::uintptr_t kCameraWorldToCameraMatrix = 0x5365204;
inline constexpr std::uintptr_t kCameraProjectionMatrix = 0x5365228;
inline constexpr std::uintptr_t kCameraWorldToScreen = 0x5365268;
inline constexpr std::uintptr_t kCameraScreenToWorld = 0x53652FC;
inline constexpr std::uintptr_t kComponentGetTransform = 0x5371D30;
inline constexpr std::uintptr_t kGameObjectGetTransform = 0x53727CC;
inline constexpr std::uintptr_t kTransformGetPositionAndRotation = 0x53764E0;
inline constexpr std::uintptr_t kAnimatorGetBoneTransform = 0x539569C;
inline constexpr std::uintptr_t kRendererGetBounds = 0x53689D0;
inline constexpr std::uintptr_t kColliderGetBounds = 0x538B650;
inline constexpr std::uintptr_t kGameObjectGetComponentsInternal = 0x5372370;
inline constexpr std::uintptr_t kPhysicsRaycastTest = 0x538BCF4;
inline constexpr std::uintptr_t kPhysicsRaycast = 0x538BD0C;
inline constexpr std::uintptr_t kPhysicsRaycastNonAlloc = 0x538BD24;
inline constexpr std::uintptr_t kPhysicsSphereCast = 0x538BDBC;
inline constexpr std::uintptr_t kPhysicsBoxCast = 0x538BE40;
}

// Metadata-usage slots. They become usable only after IL2CPP metadata init and
// must be checked with class name/namespace at runtime.
namespace typeinfo {
inline constexpr std::uintptr_t kClientWorld = 0xC1C2DB8;       // njh
inline constexpr std::uintptr_t kRegistry = 0xC1C0828;          // gat
inline constexpr std::uintptr_t kStore = 0xC1C0CE8;             // gzx
inline constexpr std::uintptr_t kPlayer = 0xC1BEB30;            // ake
inline constexpr std::uintptr_t kNetObject = 0xC1C27C0;         // mfc
inline constexpr std::uintptr_t kTransforms = 0xC1BFC80;        // dwc
inline constexpr std::uintptr_t kPlayerBindings = 0xC1DBA90;
inline constexpr std::uintptr_t kModel = 0xC1C1840;              // jgi
inline constexpr std::uintptr_t kCharacterBindings = 0xC1B0F18;
inline constexpr std::uintptr_t kBonesDictionary = 0xC1B0900;
inline constexpr std::uintptr_t kHitBoxCollider = 0xC202E90;
inline constexpr std::uintptr_t kStats = 0xC1C6810;              // ylw
inline constexpr std::uintptr_t kUntouchable = 0xC1C4A78;        // tbq
inline constexpr std::uintptr_t kVisibility = 0xC1C3BF0;         // qdp
inline constexpr std::uintptr_t kTransport = 0xC1C0D50;          // hbu
inline constexpr std::uintptr_t kActor = 0xC1C48D8;              // stn
inline constexpr std::uintptr_t kWeaponPart = 0xC1C2F80;         // nrk
inline constexpr std::uintptr_t kWeaponInstance = 0xC1C3D18;     // qjf
inline constexpr std::uintptr_t kWeaponProvider = 0xC1C1380;     // ijq
inline constexpr std::uintptr_t kWeaponDefinition = 0xC1EE058;
inline constexpr std::uintptr_t kGunParameters = 0xC1B4AE8;
inline constexpr std::uintptr_t kInputManager = 0xC1C6DC8;       // znn
inline constexpr std::uintptr_t kInputCommand = 0xC1C3568;       // owv
inline constexpr std::uintptr_t kCameraComponent = 0xC1C1498;    // ios
inline constexpr std::uintptr_t kRotation = 0xC1BFF70;           // eir
inline constexpr std::uintptr_t kCameraController = 0xC1FA380;   // nrl
inline constexpr std::uintptr_t kNavigation = 0xC1C2D98;         // nip
inline constexpr std::uintptr_t kMovement = 0xC1C4D08;           // tof
inline constexpr std::uintptr_t kGlobalOptions = 0xC1B4648;
inline constexpr std::uintptr_t kRootLifetimeScope = 0xC1F57C8;
inline constexpr std::uintptr_t kGrenadeCim = 0xC1BF488;
inline constexpr std::uintptr_t kGrenadeQak = 0xC1E9620;
inline constexpr std::uintptr_t kBomb = 0xC1C6DD0;
inline constexpr std::uintptr_t kDrop = 0xC1C2DB0;
inline constexpr std::uintptr_t kRagdoll = 0xC1C24A8;
}

namespace field_candidate {
inline constexpr std::size_t kStatsHp = 0x10;
inline constexpr std::size_t kStatsMaxHp = 0x12;
inline constexpr std::size_t kStatsArmor = 0x14;
inline constexpr std::size_t kStatsMaxArmor = 0x16;
inline constexpr std::size_t kWeaponContextInPart = 0x30;
inline constexpr std::size_t kTeamInWeaponPart = 0x38;
inline constexpr std::size_t kActiveWeaponInPart = 0x78;
inline constexpr std::size_t kWeaponIdInInstance = 0x10;
inline constexpr std::size_t kAmmoInInstance = 0x54;
}

namespace managed {
inline constexpr std::size_t kObjectFirstField = 0x10;
inline constexpr std::size_t kArrayLength = 0x18;
inline constexpr std::size_t kArrayData = 0x20;
inline constexpr std::size_t kListItems = 0x10;
inline constexpr std::size_t kListSize = 0x18;
inline constexpr float kFallbackPlayerHeight = 1.78f;
}

}  // namespace lemming::offsets
