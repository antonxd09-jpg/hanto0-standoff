#pragma once
#include <cstdint>
namespace offsets {
namespace manager {
    inline constexpr std::uintptr_t ClientWorld_TypeInfo = 0xCD2F9B0;
    inline constexpr std::uintptr_t ClientWorld_NetObjectManager = 0x40;
    inline constexpr std::uintptr_t NetObjectManager_Registry = 0x30;
    inline constexpr std::uintptr_t NetObjectRegistry_AllObjects = 0x18;
}
namespace list {
    inline constexpr std::uintptr_t Array_Items = 0x20;
    inline constexpr std::uintptr_t cmz_Array = 0x10;
    inline constexpr std::uintptr_t cmz_Count = 0x18;
}
namespace players { inline constexpr std::uintptr_t odq_TypeInfo = 0xCD32658; }
namespace localplayer {
    inline constexpr std::uintptr_t Player_IsLocal = 0x44;
    inline constexpr std::uintptr_t Player_Stats = 0x70;
    inline constexpr std::uintptr_t Stats_Hp = 0x10;
}
namespace player { inline constexpr std::uintptr_t odq_uiav = 0x58; }
namespace controllers { inline constexpr std::uintptr_t MovementController_CharacterController = 0x40; }
namespace unity { inline constexpr std::uintptr_t Camera_get_worldToCameraMatrix_rva = 0x6BE3C50; }
}
