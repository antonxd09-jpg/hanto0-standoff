#pragma once
#include <cstdint>
namespace astra::silent_offsets {
// Supplied 1.1.0 ARM64 binary; Build ID gated by entry.cpp.
inline constexpr std::uintptr_t kShotEmit=0x9CA6A0C;
inline constexpr std::uintptr_t kShotCallsite=0x9CA6638;
inline constexpr std::uintptr_t kColliderBounds=0x92DE468;
inline constexpr std::uintptr_t kNativeRaycast=0x92DE930;
inline constexpr std::uintptr_t kDefaultPhysicsScene=0x92D6BCC;
inline constexpr std::uintptr_t kObjectEntityId=0x6951FFC;
inline constexpr std::uintptr_t kCommandLook=0x7FF899C;
inline constexpr std::uintptr_t kCameraLookWrite=0xAA73FE4;
inline constexpr std::uintptr_t kCommandGate=0x9C83188;
inline constexpr std::uintptr_t kCommandGateSite=0x9C82F28;
inline constexpr std::uintptr_t kCommandEnd=0x9C8F244;
inline constexpr std::uintptr_t kCommandEndSite=0x9C90E98;
inline constexpr std::uint32_t kShotCallsiteWord=0x940000f5;
inline constexpr std::uint32_t kCameraLookWriteWord=0x9756126e;
inline constexpr std::uint32_t kCommandGateSiteWord=0x94000098;
inline constexpr std::uint32_t kCommandEndSiteWord=0x97fff8eb;
}
