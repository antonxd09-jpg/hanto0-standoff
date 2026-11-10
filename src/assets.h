// Menu source watermark: @use_sberbank
#pragma once

#include <cstddef>

// Binary assets compiled into the executable (see cmake/EmbedAssets.cmake).
namespace nl::assets {

struct Blob {
    const unsigned char* data;
    std::size_t size;
};

extern const Blob noto_sans_medium;
extern const Blob noto_sans_semibold;
extern const Blob noto_sans_black;
extern const Blob fa_solid;
extern const Blob avatar_rgba; // 120x120, RGBA8
extern const Blob inventory_pak; // PNGs indexed by nl::inv::images

inline constexpr int avatar_size = 120;

} // namespace nl::assets
