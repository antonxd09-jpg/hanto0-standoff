#pragma once

#include <cstdint>

namespace lemming::offsets {

inline constexpr char kProfileName[] = "Standoff2-1.0.0-dump64-v1";
inline constexpr char kSourceSha256[] = "ecb17199f3219949dd0884ab0977dd96894ba81daa719bfd3d3de9a3dde7296a";

namespace player_controller {
inline constexpr std::uintptr_t kMainCameraHolder = 0x28;
inline constexpr std::uintptr_t kFpsCameraHolder = 0x30;
inline constexpr std::uintptr_t kFpsDirective = 0x38;
inline constexpr std::uintptr_t kAimController = 0x80;
inline constexpr std::uintptr_t kWeaponryController = 0x88;
inline constexpr std::uintptr_t kMecanimController = 0x90;
inline constexpr std::uintptr_t kMovementController = 0x98;
inline constexpr std::uintptr_t kArmsAnimationController = 0xA0;
inline constexpr std::uintptr_t kPlayerHitController = 0xA8;
inline constexpr std::uintptr_t kNetworkController = 0xC0;
inline constexpr std::uintptr_t kPlayerMainCamera = 0xE8;
inline constexpr std::uintptr_t kPlayerFpsCamera = 0xF0;
inline constexpr std::uintptr_t kCharacterController = 0x118;
inline constexpr std::uintptr_t kPhotonView = 0x150;
inline constexpr std::uintptr_t kPhotonPlayer = 0x160;
}

namespace weaponry_controller {
inline constexpr std::uintptr_t kPlayerController = 0x78;
inline constexpr std::uintptr_t kMecanimController = 0x80;
inline constexpr std::uintptr_t kWeaponPickupController = 0x90;
inline constexpr std::uintptr_t kKitController = 0x98;
inline constexpr std::uintptr_t kCurrentWeaponController = 0xA0;
inline constexpr std::uintptr_t kWeaponManager = 0xB8;
}

namespace gun_controller {
inline constexpr std::uintptr_t kGunParameters = 0x168;
inline constexpr std::uintptr_t kAccuracyData = 0x228;
}

namespace gun_parameters {
inline constexpr std::uintptr_t kAmmunition = 0x130;
inline constexpr std::uintptr_t kDamage = 0x140;
inline constexpr std::uintptr_t kHitRange = 0x148;
inline constexpr std::uintptr_t kFireRate = 0x14C;
inline constexpr std::uintptr_t kRecoilControl = 0x150;
inline constexpr std::uintptr_t kRecoilParameters = 0x158;
inline constexpr std::uintptr_t kAccuracyMultOnJump = 0x168;
inline constexpr std::uintptr_t kRecoilMultOnCrouch = 0x178;
inline constexpr std::uintptr_t kAccuracyMultOnCrouch = 0x17C;
inline constexpr std::uintptr_t kRecoilAimMult = 0x180;
inline constexpr std::uintptr_t kAccuracyAimMult = 0x184;
inline constexpr std::uintptr_t kRecoilAimMultOnCrouch = 0x188;
inline constexpr std::uintptr_t kAccuracyAimMultOnCrouch = 0x18C;
inline constexpr std::uintptr_t kArmorPenetration = 0x1A0;
inline constexpr std::uintptr_t kPenetrationPower = 0x1A4;
inline constexpr std::uintptr_t kReloadDuration = 0x1A8;
inline constexpr std::uintptr_t kTacticalReloadDuration = 0x1AC;
inline constexpr std::uintptr_t kFireRateSafe = 0x1D4;
inline constexpr std::uintptr_t kRecoilControlSafe = 0x240;
inline constexpr std::uintptr_t kArmorPenetrationSafe = 0x258;
inline constexpr std::uintptr_t kPenetrationPowerSafe = 0x264;
inline constexpr std::uintptr_t kReloadDurationSafe = 0x270;
inline constexpr std::uintptr_t kHitRangeSafe = 0x2DC;
}

namespace ammunition {
inline constexpr std::uintptr_t kMagazineCapacity = 0x10;
inline constexpr std::uintptr_t kCapacity = 0x12;
}

namespace recoil_parameters {
inline constexpr std::uintptr_t kHorizontalRange = 0x10;
inline constexpr std::uintptr_t kVerticalRange = 0x14;
inline constexpr std::uintptr_t kCameraDeviationCoeff = 0x48;
inline constexpr std::uintptr_t kMaxApproachSpeed = 0x4C;
inline constexpr std::uintptr_t kRecoilAccelDuration = 0x50;
inline constexpr std::uintptr_t kRecoilAccelStep = 0x54;
}

namespace game_mode_team_settings {
inline constexpr std::uintptr_t kSpeedMultiplier = 0x14;
inline constexpr std::uintptr_t kHighJump = 0x18;
inline constexpr std::uintptr_t kInfinityAmmoAbsolutely = 0x1B;
inline constexpr std::uintptr_t kInfinityMagazines = 0x1C;
inline constexpr std::uintptr_t kInfinityGrenades = 0x1D;
inline constexpr std::uintptr_t kConfiguredHealth = 0x30;
inline constexpr std::uintptr_t kConfiguredArmor = 0x34;
}

namespace photon_player {
inline constexpr std::uintptr_t kActorId = 0x18;
inline constexpr std::uintptr_t kName = 0x20;
inline constexpr std::uintptr_t kUserId = 0x28;
inline constexpr std::uintptr_t kIsLocal = 0x30;
inline constexpr std::uintptr_t kCustomProperties = 0x38;
}

namespace method_rva {
inline constexpr std::uintptr_t kWeaponryOnPreInitialize = 0x9147D18;
inline constexpr std::uintptr_t kWeaponryPrepareArmsBipedMap = 0x91584AC;
inline constexpr std::uintptr_t kWeaponryConstructor = 0x914F354;
inline constexpr std::uintptr_t kBombWeaponryOnPreInitialize = 0x9149214;
inline constexpr std::uintptr_t kBombWeaponryConstructor = 0x9155EB8;
}

}  // namespace lemming::offsets
