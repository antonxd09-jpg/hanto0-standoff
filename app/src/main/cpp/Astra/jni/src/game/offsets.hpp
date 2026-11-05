#pragma once

#include <cstddef>
#include <cstdint>

namespace astra::offsets {

inline constexpr char kProfileName[] = "Standoff2-1.1.0-arm64-visuals";
inline constexpr char kSourceSha256[] = "be659584a895690594ca097793b3c0043cd7c613c4449d5a9b8b7a8ce540c2ed";
inline constexpr char kDumpCsSha256[] = "06bf67d75455cd36911f2a0e3b72533788b4a555a51c4a15dcc08583d2ccf77d";
inline constexpr char kIl2CppHeaderSha256[] = "f3572e9c7ed070e2d7d79332bc097e255560af5eabd603b5c125d5a57c70651e";
inline constexpr char kStringLiteralSha256[] = "2c207ec42ad2c4398a768c039f24c577e24f73b1a1cbb5934f6c30ec2733c88f";
inline constexpr char kApkSha256[] = "unavailable";
inline constexpr char kLibUnitySha256[] = "ba0966976204fa3f7a2755c9588d32299fe7eb794b712c9f1a444e046cbe7c13";
inline constexpr char kLibUnityBuildId[] = "69815f2befe135c2";

namespace elf {
inline constexpr std::uintptr_t kTextBegin = 0x57b9480;
inline constexpr std::uintptr_t kTextEnd = 0xc25d7c0;
inline constexpr std::uintptr_t kDataRelRoBegin = 0xc262950;
inline constexpr std::uintptr_t kDataBegin = 0xc997e00;
inline constexpr std::uintptr_t kDataEnd = 0xd307bb8;
}

// ARM64 IL2CPP entry points verified against the supplied 1.1.0 libunity.so.
// Prefer dlsym where available; use these only after the ARM64 profile gate passes.
namespace il2cpp_api {
inline constexpr std::uintptr_t kResolveIcall = 0x67C0188;
inline constexpr std::uintptr_t kFree = 0x677EC28;
inline constexpr std::uintptr_t kAssemblyGetImage = 0x6781F68;
inline constexpr std::uintptr_t kClassFromName = 0x6781F8C;
inline constexpr std::uintptr_t kClassGetFields = 0x6782018;
inline constexpr std::uintptr_t kClassGetFieldFromName = 0x6782020;
inline constexpr std::uintptr_t kClassGetName = 0x6782028;
inline constexpr std::uintptr_t kClassGetNamespace = 0x678202C;
inline constexpr std::uintptr_t kClassGetParent = 0x6782030;
inline constexpr std::uintptr_t kDomainGet = 0x67820C4;
inline constexpr std::uintptr_t kDomainAssemblyOpen = 0x67820C8;
inline constexpr std::uintptr_t kFieldGetOffset = 0x6782198;
inline constexpr std::uintptr_t kFieldGetType = 0x678219C;
inline constexpr std::uintptr_t kFieldStaticGetValue = 0x67821A4;
inline constexpr std::uintptr_t kThreadAttach = 0x67822C0;
inline constexpr std::uintptr_t kThreadDetach = 0x67822C4;
inline constexpr std::uintptr_t kTypeGetName = 0x67822E0;
}

namespace unity {
inline constexpr std::uintptr_t kNativeInjectEvent = 0x5AC2F20;
// 0x53653C4 returns a GC handle, NOT a managed Camera object.
inline constexpr std::uintptr_t kCameraGetMain = 0x57BF044;  // diagnostics only
// Native Camera::GetMain; verified call target of the GC-handle wrapper.
inline constexpr std::uintptr_t kNativeCameraGetMain = 0x591B4FC;
inline constexpr std::uintptr_t kCameraGetCurrent = 0x57BF068;
inline constexpr std::uintptr_t kCameraGetFov = 0x57BEC64;
inline constexpr std::uintptr_t kCameraWorldToCameraMatrix = 0x57BEE84;
inline constexpr std::uintptr_t kCameraProjectionMatrix = 0x57BEEA8;
inline constexpr std::uintptr_t kCameraWorldToScreen = 0x57BEEE8;
inline constexpr std::uintptr_t kCameraScreenToWorld = 0x57BEF7C;
inline constexpr std::uintptr_t kComponentGetTransform = 0x57CB9C0;
inline constexpr std::uintptr_t kGameObjectGetTransform = 0x57CC45C;
inline constexpr std::uintptr_t kTransformGetPositionAndRotation = 0x57D0184;
inline constexpr std::uintptr_t kAnimatorGetBoneTransform = 0x539569C;
inline constexpr std::uintptr_t kRendererGetBounds = 0x57C2650;
inline constexpr std::uintptr_t kColliderGetBounds = 0x538B650;
inline constexpr std::uintptr_t kGameObjectGetComponentsInternal = 0x5372370;
inline constexpr std::uintptr_t kPhysicsRaycastTest = 0x538BCF4;
inline constexpr std::uintptr_t kPhysicsRaycast = 0x538BD0C;
inline constexpr std::uintptr_t kPhysicsRaycastNonAlloc = 0x538BD24;
inline constexpr std::uintptr_t kPhysicsSphereCast = 0x538BDBC;
inline constexpr std::uintptr_t kPhysicsBoxCast = 0x538BE40;
}

// Metadata usage slots from the matching ARM64 script.json. Zero means that
// script.json exposes no direct TypeInfo slot; resolve that class by name at runtime.
namespace typeinfo {
inline constexpr std::uintptr_t kClientWorld = 0xCD2F9B0;       // njh_TypeInfo
inline constexpr std::uintptr_t kRegistry = 0xCD32028;          // gat_TypeInfo
inline constexpr std::uintptr_t kStore = 0xCD303E8;             // gzx_TypeInfo
inline constexpr std::uintptr_t kPlayer = 0xCD32658;            // ake_TypeInfo
inline constexpr std::uintptr_t kNetObject = 0xCD359A0;         // mfc_TypeInfo
inline constexpr std::uintptr_t kTransforms = 0xCD31E68;        // dwc_TypeInfo
inline constexpr std::uintptr_t kPlayerBindings = 0;            // no direct slot
inline constexpr std::uintptr_t kModel = 0xCD2DDE8;             // jgi_TypeInfo
inline constexpr std::uintptr_t kCharacterBindings = 0xC1F4440;
inline constexpr std::uintptr_t kBonesDictionary = 0xC1F3E28;
inline constexpr std::uintptr_t kHitBoxCollider = 0;             // no direct slot
inline constexpr std::uintptr_t kStats = 0xCD360F0;             // ylw_TypeInfo
inline constexpr std::uintptr_t kUntouchable = 0xCD30C38;       // tbq_TypeInfo
inline constexpr std::uintptr_t kVisibility = 0xCD341C0;        // qdp_TypeInfo
inline constexpr std::uintptr_t kTransport = 0xCD30C28;         // hbu_TypeInfo
inline constexpr std::uintptr_t kActor = 0xCD34C50;             // stn_TypeInfo
inline constexpr std::uintptr_t kWeaponPart = 0xCD34CE0;        // nrk_TypeInfo
inline constexpr std::uintptr_t kWeaponInstance = 0xCD30D00;    // qjf_TypeInfo
inline constexpr std::uintptr_t kWeaponProvider = 0xCD36030;    // ijq_TypeInfo
inline constexpr std::uintptr_t kWeaponDefinition = 0;          // no direct slot
inline constexpr std::uintptr_t kGunParameters = 0xC1F8010;
inline constexpr std::uintptr_t kInputManager = 0xCD337A0;      // znn_TypeInfo
inline constexpr std::uintptr_t kInputCommand = 0xCD347C0;      // owv_TypeInfo
inline constexpr std::uintptr_t kCameraComponent = 0xCD36248;   // ios_TypeInfo
inline constexpr std::uintptr_t kRotation = 0xCD31080;          // eir_TypeInfo
inline constexpr std::uintptr_t kCameraController = 0;          // nrl has no direct slot
inline constexpr std::uintptr_t kNavigation = 0xCD2EAA0;        // nip_TypeInfo
inline constexpr std::uintptr_t kMovement = 0xCD2FE20;          // tof_TypeInfo
inline constexpr std::uintptr_t kGlobalOptions = 0xC1F7B70;
inline constexpr std::uintptr_t kRootLifetimeScope = 0;         // no direct slot
inline constexpr std::uintptr_t kGrenadeCim = 0xCD2E348;
inline constexpr std::uintptr_t kGrenadeQak = 0;                // no direct slot
inline constexpr std::uintptr_t kBomb = 0xCD36938;
inline constexpr std::uintptr_t kDrop = 0xCD321F0;
inline constexpr std::uintptr_t kRagdoll = 0xCD36900;
inline constexpr std::uintptr_t kModeNbs = 0xC206188;
inline constexpr std::uintptr_t kModeXlm = 0xC2097F8;
inline constexpr std::uintptr_t kModeYmn = 0xC209D68;
inline constexpr std::uintptr_t kModeQin = 0xC207208;
inline constexpr std::uintptr_t kModeHog = 0xC2044E8;
}

namespace typevar {
inline constexpr std::uintptr_t kCharacterBindings = 0xC1D6C38;
inline constexpr std::uintptr_t kWeaponContext = 0xC1DAF50;
inline constexpr std::uintptr_t kGlobalOptions = 0xC1D8328;
inline constexpr std::uintptr_t kActor = 0xCD070B0;
}

// Managed ARM64 methods used only as validation anchors or optional read helpers.
namespace managed_rva {
// 1.1.0 ARM64 player methods: odq.fixo (slot 50), odq.apss (position).
// ake::opmn() is the per-player update interface callback used for collection;
// ake::xdzx() is the canonical world-position getter.
inline constexpr std::uintptr_t kPlayerUpdateHook = 0x9C83D78;
inline constexpr std::uintptr_t kPlayerGetPosition = 0x9C9B170;
inline constexpr std::uintptr_t kStatsGetHp = 0x9C98DD8;
inline constexpr std::uintptr_t kStatsGetMaxHp = 0x9C9A434;
inline constexpr std::uintptr_t kStatsGetArmor = 0x9C83428;
inline constexpr std::uintptr_t kStatsGetMaxArmor = 0x9C89764;
inline constexpr std::uintptr_t kStatsGetHelmet = 0x9C8A4F8;
inline constexpr std::uintptr_t kWeaponPartGetContext = 0x9C82FE0;
inline constexpr std::uintptr_t kWeaponPartGetActiveWeapon = 0x9C81184;
inline constexpr std::uintptr_t kWeaponGetId = 0x9CB3480;
inline constexpr std::uintptr_t kWeaponGetMagazineAmmo = 0x9CB2E14;
inline constexpr std::uintptr_t kWeaponGetReserveAmmo = 0x9CB22C8;
inline constexpr std::uintptr_t kNetObjectGetParts = 0x7E2EB38;
}

// Dump-confirmed physical object offsets. Value-type offsets are already
// converted from boxed dump offsets to their embedded physical positions.
namespace field {
// NetObject world path confirmed from the matching ARM64 dump and native code:
// njh(ClientWorld) -> gat(registry) -> gzx(store) -> yjk<krm> -> krm[] objects.
namespace njh {
inline constexpr std::size_t kRegistry = 0x40;
}
namespace gat {
inline constexpr std::size_t kWorld = 0x20;
inline constexpr std::size_t kStore = 0x30;
inline constexpr std::size_t kTypeConfiguration = 0x48;
}
namespace gzx {
inline constexpr std::size_t kObjects = 0x18;
}
namespace yjk {
inline constexpr std::size_t kItems = 0x10;
inline constexpr std::size_t kCount = 0x18;
}
namespace mfc {
inline constexpr std::size_t kNetObjectId = 0x10;
inline constexpr std::size_t kOwnerId = 0x14;
inline constexpr std::size_t kWorld = 0x18;
inline constexpr std::size_t kComponentsContainer = 0x20;
inline constexpr std::size_t kPartsContainer = 0x28;
inline constexpr std::size_t kLocalOwned = 0x44;
}
namespace part_container {
// rct::vsog is a managed List<eur>, not an array.
inline constexpr std::size_t kPartList = 0x10;
}
namespace managed_list {
inline constexpr std::size_t kItems = 0x10;
inline constexpr std::size_t kSize = 0x18;
}
namespace ake {
inline constexpr std::size_t kStats = 0x70;
inline constexpr std::size_t kVisibility = 0x88;
inline constexpr std::size_t kTeam = 0x90;
}
namespace ylw {
inline constexpr std::size_t kHp = 0x10;
inline constexpr std::size_t kUnknown12 = 0x12;
inline constexpr std::size_t kMaxHp = 0x14;
inline constexpr std::size_t kArmor = 0x16;
inline constexpr std::size_t kUnknown18 = 0x18;
inline constexpr std::size_t kMaxArmor = 0x1A;
inline constexpr std::size_t kHelmet = 0x1C;
inline constexpr std::size_t kFloatValue = 0x20;
}
namespace nrk {
inline constexpr std::size_t kWeaponContext = 0x30;
inline constexpr std::size_t kTeam = 0x40;
inline constexpr std::size_t kHitTransform = 0x48;
inline constexpr std::size_t kNavigationContext = 0x50;
inline constexpr std::size_t kAccuracyContext = 0x70;
inline constexpr std::size_t kRecoilContext = 0x78;
inline constexpr std::size_t kActiveWeapon = 0x80;
inline constexpr std::size_t kWeaponState = 0x88;
inline constexpr std::size_t kAimState = 0x94;
inline constexpr std::size_t kRecoilState = 0xa0;
inline constexpr std::size_t kReloadState = 0xc0;
inline constexpr std::size_t kCurrentTime = 0xc4;
inline constexpr std::size_t kWasHit = 0xcc;
inline constexpr std::size_t kClearWeaponSignal = 0xd0;
}
namespace qjf {
inline constexpr std::size_t kWeaponId = 0x10;
inline constexpr std::size_t kWeaponMetaData = 0x18;
inline constexpr std::size_t kWeaponMetaId = 0x50;
inline constexpr std::size_t kMagazineAmmo = 0x52;
inline constexpr std::size_t kReserveAmmo = 0x54;
inline constexpr std::size_t kFloatValue = 0x58;
inline constexpr std::size_t kDefinitionLike = 0x60;
}
namespace dwc {
inline constexpr std::size_t kTransformA = 0x28;
inline constexpr std::size_t kTransformB = 0x30;
inline constexpr std::size_t kPlayerBindings = 0x38;
}
namespace jgi {
inline constexpr std::size_t kHitBoxColliders = 0x18;
inline constexpr std::size_t kTransform = 0x20;
inline constexpr std::size_t kAnimator = 0x28;
inline constexpr std::size_t kCharacterBindings = 0x30;
}
namespace player_bindings {
inline constexpr std::size_t kCharacterController = 0x20;
inline constexpr std::size_t kCapsuleCollider = 0x28;
inline constexpr std::size_t kCameraPlaceholder = 0x30;
inline constexpr std::size_t kTrigger = 0x38;
}
namespace character_bindings {
inline constexpr std::size_t kBones = 0x20;
inline constexpr std::size_t kTool = 0x28;
inline constexpr std::size_t kAnimator = 0x30;
inline constexpr std::size_t kCharacterRenderer = 0x38;
inline constexpr std::size_t kArmsRenderer = 0x40;
inline constexpr std::size_t kGlovesRenderer = 0x48;
}
namespace hitbox_collider {
inline constexpr std::size_t kHitBox = 0x20;
inline constexpr std::size_t kCollider = 0x28;
}
}

// Compatibility aliases for older source files. These values are no longer candidates.
namespace field_candidate {
inline constexpr std::size_t kStatsHp = field::ylw::kHp;
inline constexpr std::size_t kStatsMaxHp = field::ylw::kMaxHp;
inline constexpr std::size_t kStatsArmor = field::ylw::kArmor;
inline constexpr std::size_t kStatsMaxArmor = field::ylw::kMaxArmor;
inline constexpr std::size_t kWeaponContextInPart = field::nrk::kWeaponContext;
inline constexpr std::size_t kTeamInWeaponPart = field::nrk::kTeam;
inline constexpr std::size_t kActiveWeaponInPart = field::nrk::kActiveWeapon;
inline constexpr std::size_t kWeaponIdInInstance = field::qjf::kWeaponId;
inline constexpr std::size_t kAmmoInInstance = field::qjf::kReserveAmmo;
}

namespace managed {
// UnityEngine.Object::m_CachedPtr (confirmed by dump and Camera managed wrapper).
inline constexpr std::size_t kUnityObjectCachedPtr = 0x10;
// Native Camera cached matrices; confirmed from 0x5A7C2B8/0x5A7C31C.
inline constexpr std::size_t kNativeCameraWorldToCameraMatrix = 0x70;
inline constexpr std::size_t kNativeCameraProjectionMatrix = 0xB0;
inline constexpr std::size_t kObjectFirstField = 0x10;
inline constexpr std::size_t kArrayLength = 0x18;
inline constexpr std::size_t kArrayData = 0x20;
inline constexpr std::size_t kListItems = 0x10;
inline constexpr std::size_t kListSize = 0x18;
inline constexpr float kFallbackPlayerHeight = 1.78f;
}

}  // namespace astra::offsets

// Visuals bindings for 1.1.0. Unreferenced legacy entries remain outside this port.
namespace astra::offsets {
namespace il2cpp_api {
inline constexpr std::uintptr_t kClassGetMethods = 0x6782024;
inline constexpr std::uintptr_t kClassGetMethodFromName = 0x67891C4;
inline constexpr std::uintptr_t kMethodGetName = 0x6782240;
inline constexpr std::uintptr_t kMethodGetParamCount = 0x6782250;
inline constexpr std::uintptr_t kMethodGetParam = 0x6782254;
inline constexpr std::uintptr_t kMethodGetParamName = 0x679FA78;
inline constexpr std::uintptr_t kStringNew = 0x67822AC;
inline constexpr std::uintptr_t kStringChars = 0x67822A8;
inline constexpr std::uintptr_t kObjectNew = 0x6782268;
}
namespace method_info {
inline constexpr std::size_t kSlot = 0x00;               // uint16 vtable slot
inline constexpr std::size_t kName = 0x30;               // const char*
inline constexpr std::size_t kInvokerMethod = 0x20;
inline constexpr std::size_t kMethodPointer = 0x68;      // Il2CppMethodPointer
inline constexpr std::size_t kVirtualMethodPointer = 0x68;
inline constexpr std::size_t kKlass = 0x48;              // Il2CppClass*
}
namespace class_layout {
inline constexpr std::size_t kStaticFields = 0xf8;
inline constexpr std::size_t kParent = 0x148;
inline constexpr std::size_t kName = 0x188;
inline constexpr std::size_t kNamespace = 0x90;
inline constexpr std::size_t kVtable = 0x1b0;            // VirtualInvokeData[]
}
namespace visuals_rva {
namespace shader {
inline constexpr std::uintptr_t kFind = 0x72FEB98;          // static Shader Find(string)
inline constexpr std::uintptr_t kFindBuiltin = 0x7305CE8;   // internal static Shader FindBuiltin(string)
}
namespace material {
// NOTE: only the *string* overloads are used by the natives. The int
// overloads live at 0x75E89C0 (GetColor(int)) / 0x75FE688 (GetFloat(int)) /
// 0x760C250 (SetFloat(int,float)) / 0x75FC688 (SetColor(int,Color)) and must
// NOT be swapped in: the callers pass il2cpp System.String pointers.
inline constexpr std::uintptr_t kGetShader = 0x7307B1C;       // Shader get_shader()
inline constexpr std::uintptr_t kSetShader = 0x72DE678;       // void set_shader(Shader)
// Property forms (no string arguments) - used on cheat-owned materials whose
// shader is known to carry _Color / _MainTex:
inline constexpr std::uintptr_t kCtorShader = 0x7301DAC;      // void .ctor(Shader)
inline constexpr std::uintptr_t kGetColorProp = 0x72E7404;    // Color get_color()
inline constexpr std::uintptr_t kSetColorProp = 0x72FC59C;    // void set_color(Color)
inline constexpr std::uintptr_t kGetMainTexture = 0x72E5370;  // Texture get_mainTexture()
inline constexpr std::uintptr_t kSetMainTexture = 0x72E1034;  // void set_mainTexture(Texture)
inline constexpr std::uintptr_t kSetColor = 0x72F3478;        // void SetColor(string, Color)
inline constexpr std::uintptr_t kGetColor = 0x72FCA24;        // Color GetColor(string)
inline constexpr std::uintptr_t kHasProperty = 0x72E23FC;     // bool HasProperty(string)
inline constexpr std::uintptr_t kHasFloat = 0x7301D50;        // bool HasFloat(string)
inline constexpr std::uintptr_t kSetFloat = 0x72EFCC0;        // void SetFloat(string, float)
inline constexpr std::uintptr_t kGetFloat = 0x72F3F70;        // float GetFloat(string)
inline constexpr std::uintptr_t kGetTexture = 0x72F6650;      // Texture GetTexture(string)
inline constexpr std::uintptr_t kSetTexture = 0x72FA778;      // void SetTexture(string, Texture)
inline constexpr std::uintptr_t kGetRenderQueue = 0x72F5050;  // int get_renderQueue()
inline constexpr std::uintptr_t kSetRenderQueue = 0x72E6430;  // void set_renderQueue(int)
}
namespace renderer {
inline constexpr std::uintptr_t kGetMaterial = 0x7305428;      // private Material GetMaterial()
inline constexpr std::uintptr_t kSetMaterial = 0x72F3B64;      // void set_material(Material)
inline constexpr std::uintptr_t kGetSharedMaterial = 0x72FC504;  // Material get_sharedMaterial()
inline constexpr std::uintptr_t kGetSharedMaterials = 0x7307524; // Material[] get_sharedMaterials()
inline constexpr std::uintptr_t kSetSharedMaterials = 0x72FF268; // void set_sharedMaterials(Material[])
}
namespace reflection {
inline constexpr std::uintptr_t kTypeGetType = 0x681BAD4;          // static Type Type.GetType(string)
inline constexpr std::uintptr_t kFindObjectsOfTypeAll = 0x7920CB8; // static Object[] Object.FindObjectsOfTypeAll(Type)
}
namespace render_settings {
inline constexpr std::uintptr_t kGetFog = 0x72DC548;               // static bool get_fog()
inline constexpr std::uintptr_t kSetFog = 0x72E6AD8;               // static void set_fog(bool)
inline constexpr std::uintptr_t kGetFogColor = 0x72D55D0;          // static Color get_fogColor()
inline constexpr std::uintptr_t kSetFogColor = 0x7304D8C;          // static void set_fogColor(Color)
inline constexpr std::uintptr_t kGetAmbientSky = 0x72F4CF0;        // static Color get_ambientSkyColor()
inline constexpr std::uintptr_t kSetAmbientSky = 0x72E9E50;        // static void set_ambientSkyColor(Color)
inline constexpr std::uintptr_t kGetAmbientEquator = 0x7302674;    // static Color get_ambientEquatorColor()
inline constexpr std::uintptr_t kSetAmbientEquator = 0x72FE9D8;    // static void set_ambientEquatorColor(Color)
inline constexpr std::uintptr_t kGetAmbientGround = 0x72F6688;     // static Color get_ambientGroundColor()
inline constexpr std::uintptr_t kSetAmbientGround = 0x72E4B60;     // static void set_ambientGroundColor(Color)
inline constexpr std::uintptr_t kGetAmbientLight = 0x72D8708;      // static Color get_ambientLight()
inline constexpr std::uintptr_t kSetAmbientLight = 0x72F6B70;      // static void set_ambientLight(Color)
inline constexpr std::uintptr_t kGetAmbientIntensity = 0x72F3E20;  // static float get_ambientIntensity()
inline constexpr std::uintptr_t kSetAmbientIntensity = 0x72EEF68;  // static void set_ambientIntensity(float)
inline constexpr std::uintptr_t kGetAmbientMode = 0x73065F4;       // static AmbientMode get_ambientMode()
inline constexpr std::uintptr_t kSetAmbientMode = 0x73061C0;       // static void set_ambientMode(AmbientMode)
inline constexpr std::uintptr_t kGetFogStart = 0x72E74C0;          // static float get_fogStartDistance()
inline constexpr std::uintptr_t kSetFogStart = 0x72EF738;          // static void set_fogStartDistance(float)
inline constexpr std::uintptr_t kGetFogEnd = 0x7301B08;            // static float get_fogEndDistance()
inline constexpr std::uintptr_t kSetFogEnd = 0x72E1F64;            // static void set_fogEndDistance(float)
inline constexpr std::uintptr_t kGetFogMode = 0x73027DC;           // static FogMode get_fogMode()
inline constexpr std::uintptr_t kSetFogMode = 0x72F7A24;           // static void set_fogMode(FogMode)
inline constexpr std::uintptr_t kGetFogDensity = 0x72DC2D4;        // static float get_fogDensity()
inline constexpr std::uintptr_t kSetFogDensity = 0x7302444;        // static void set_fogDensity(float)
inline constexpr std::uintptr_t kGetSkybox = 0x72D7B70;            // static Material get_skybox()
inline constexpr std::uintptr_t kSetSkybox = 0x72DDA3C;            // static void set_skybox(Material)
inline constexpr std::uintptr_t kSetSunColor = 0;                  // set_sunColor(Color): METADATA-ONLY
                                                                   // (the dump tables available here do
                                                                   // not carry this RVA; resolveMethod
                                                                   // resolves it from live il2cpp metadata)
}
// Skeleton ESP: humanoid bone lookup through the character Animator.
namespace skeleton {
inline constexpr std::uintptr_t kAnimatorGetBoneTransform = 0x7908B18;  // Transform Animator.GetBoneTransform(HumanBodyBones)
}
// Camera-driven effects (scope suppression, view model). Transform wrappers live in
// the 0x67E..0x680 block of UnityEngine.Transform.
namespace transform_rva {
inline constexpr std::uintptr_t kGetPosition = 0x6937790;   // Vector3 Transform.get_position()
inline constexpr std::uintptr_t kSetPosition = 0x69378BC;   // void Transform.set_position(Vector3)
inline constexpr std::uintptr_t kGetForward = 0x694B74C;    // Vector3 Transform.get_forward()
// Local-position wrappers for the hand-pos view model offset.
inline constexpr std::uintptr_t kGetLocalPosition = 0x6935E24;  // Vector3 Transform.get_localPosition()
inline constexpr std::uintptr_t kSetLocalPosition = 0x6945870;  // void Transform.set_localPosition(Vector3)
}
namespace physics {
// bool Physics.Raycast(Vector3 origin, Vector3 direction, out RaycastHit hitInfo,
//                      float maxDistance, int layerMask)
// ABI (AAPCS64): origin -> s0-s2 (HFA), direction -> s3-s5, &hit -> x0,
// maxDistance -> s6, layerMask -> x1, MethodInfo* -> x2. RaycastHit is the
// managed struct: point @+0x00, normal @+0x0C, distance @+0x20 (56 bytes).
inline constexpr std::uintptr_t kRaycast = 0x92DDB4C;
}
namespace canvas_group {
inline constexpr std::uintptr_t kSetAlpha = 0x84EAB6C;      // void CanvasGroup.set_alpha(float)
}
// Camera property wrappers for the remove-scope FOV normalizer.
namespace camera {
inline constexpr std::uintptr_t kGetFieldOfView = 0x6BE9C80;  // float Camera.get_fieldOfView()
inline constexpr std::uintptr_t kSetFieldOfView = 0x6BD4080;  // void Camera.set_fieldOfView(float)
inline constexpr std::uintptr_t kSetUseOcclusionCulling = 0x6BEA084;  // void set_useOcclusionCulling(bool)
inline constexpr std::uintptr_t kGetBackgroundColor = 0x6BEA648;      // Color get_backgroundColor()
inline constexpr std::uintptr_t kSetBackgroundColor = 0x6BD6B0C;      // void set_backgroundColor(Color)
inline constexpr std::uintptr_t kGetAspect = 0x6BE084C;               // float get_aspect()
// Aspect-ratio override rides the projection matrix (this build exposes no
// Camera.set_aspect). The _Injected forms pass Matrix4x4 by reference:
// void (*)(Camera*, Matrix4x4*, MethodInfo*).
inline constexpr std::uintptr_t kGetProjectionMatrixInjected = 0x6BDEEB8;
inline constexpr std::uintptr_t kSetProjectionMatrixInjected = 0x6BE0A78;
inline constexpr std::uintptr_t kResetProjectionMatrixInjected = 0x6BDB4D8;
}
// Per-light shadows + intensity (No Shadows / Night mode).
// LightShadows: None=0, Hard=1, Soft=2.
namespace light {
inline constexpr std::uintptr_t kGetShadows = 0x72DBBD0;   // LightShadows Light.get_shadows()
inline constexpr std::uintptr_t kSetShadows = 0x73019C0;   // void Light.set_shadows(LightShadows)
inline constexpr std::uintptr_t kGetIntensity = 0x72F92FC; // float Light.get_intensity()
inline constexpr std::uintptr_t kSetIntensity = 0x72D67F0; // void Light.set_intensity(float)
}
// Animator culling (Occlusion Bypass): AlwaysAnimate keeps the bone
// transforms updating for players outside the camera frustum / behind walls,
// so the skeleton keeps reading live positions instead of a frozen pose.
namespace animator {
inline constexpr std::uintptr_t kGetCullingMode = 0x78F8024;  // AnimatorCullingMode get_cullingMode()
inline constexpr std::uintptr_t kSetCullingMode = 0x78FDBF4;  // void set_cullingMode(AnimatorCullingMode)
}
// GameObject activation (remove scope re-worked to the original cheat's
// SetActive path: the CanvasGroup alpha alone is re-driven by the game).
namespace game_object {
inline constexpr std::uintptr_t kComponentGetGameObject = 0x790DB98;  // GameObject Component.get_gameObject()
inline constexpr std::uintptr_t kGetActive = 0x7920088;               // bool GameObject.get_active()
inline constexpr std::uintptr_t kSetActive = 0x7918CFC;              // void GameObject.set_active(bool)
}
// Local-player camera component execute (netcode component of the player).
// Late in the frame - hooking it gives a Unity-main-thread callback that runs
// AFTER the game positioned the camera, which is where the third-person
// pull-back / scope suppression / arms offset must run to survive the frame.
namespace camera_component {
inline constexpr std::uintptr_t kIqkrExecute = 0xAA73E0C;  // void ios.iqkr(ExecuteTime)
}
}
}
