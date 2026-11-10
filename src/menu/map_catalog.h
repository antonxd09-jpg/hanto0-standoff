// Menu source watermark: @use_sberbank
#pragma once

#include <cstdint>

#include "imgui.h"

// Map previews for the Ambience MAP SELECTION list (see tools/build_maps.py).
// The pictures live in assets/images/maps.pak, which is indexed exactly like the
// inventory pak: offset and length of a PNG inside the blob.
namespace nl::maps {

// A PNG in the pak holding the opaque part (x, y, w, h) of a canvas_w x canvas_h picture.
struct Image {
    std::uint32_t offset;
    std::uint32_t size;
    std::uint16_t x;
    std::uint16_t y;
    std::uint16_t w;
    std::uint16_t h;
    std::uint16_t canvas_w;
    std::uint16_t canvas_h;
};

struct Entry {
    const char* id;    // the game's map name, "de_mirage"
    const char* name;  // display name, "Mirage"
    int image;         // index into images[], -1 when the pak has no picture
};

extern const Image images[];
extern const int image_count;
extern const Entry entries[];
extern const int entry_count;

// Index into images[] for a map id, or -1 when the pak has no picture for it.
int image_of(const char* id);
// Display name for a map id, or the id itself when the list does not know it.
const char* display_name(const char* id);

} // namespace nl::maps
