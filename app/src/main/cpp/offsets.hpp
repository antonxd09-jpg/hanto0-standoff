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
namespace players {
    inline constexpr std::uintptr_t odq_TypeInfo = 0xCD32658;
    inline constexpr std::uintptr_t odq_GetPosition_rva = 0x9C9B170;
    inline constexpr std::uintptr_t odq_GetRotation_rva = 0x9C96A38;
    inline constexpr std::uintptr_t odq_SetPositionRotation_rva = 0x9C825CC;
}
namespace localplayer {
    inline constexpr std::uintptr_t Player_IsLocal = 0x44;
    inline constexpr std::uintptr_t Player_Team = 0x90;
    inline constexpr std::uintptr_t Player_Stats = 0x70;
    inline constexpr std::uintptr_t Stats_Hp = 0x10;
    inline constexpr std::uintptr_t Stats_MaxHp = 0x14;
}
namespace unity {
    inline constexpr std::uintptr_t Camera_get_main_rva = 0x6BE9604;
    inline constexpr std::uintptr_t Camera_WorldToScreenPoint_2_rva = 0x6BE52D8;
    inline constexpr std::uintptr_t Camera_get_pixelWidth_rva = 0x6BD6F1C;
    inline constexpr std::uintptr_t Camera_get_pixelHeight_rva = 0x6BE3248;
    inline constexpr std::uintptr_t RenderSettings_set_fog_rva = 0x72E6AD8;
    inline constexpr std::uintptr_t Transform_set_eulerAngles_rva = 0x694969C;
    inline constexpr std::uintptr_t Component_get_transform_rva = 0x791CF08;
}
namespace weapon {
    inline constexpr std::uintptr_t RecoilParameters_set_HorizontalRange_rva = 0x9CAC284;
    inline constexpr std::uintptr_t RecoilParameters_set_VerticalRange_rva = 0x9CB0CA4;
    inline constexpr std::uintptr_t AccuracyParameters_set_AccuracyAimMult_rva = 0x9CAA3DC;
    inline constexpr std::uintptr_t AccuracyParameters_set_AccuracyMultOnJump_rva = 0x9CA0C38;
}
namespace movement {
    inline constexpr std::uintptr_t PlayerTranslationParameters_set_SpeedMultiplayer_rva = 0xA7D158C;
    inline constexpr std::uintptr_t PlayerTranslationParameters_set_BunnyJumpMaxSpeed_rva = 0xA7D4678;
}
}
