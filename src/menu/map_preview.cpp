// Menu source watermark: @use_sberbank
// Desktop harness: the game archive is not reachable, so every preview reports empty and
// the Ambience list falls back to its flat rows.
#include "menu/map_preview.h"

#if !defined(_WIN32) || defined(KAMSHAKO_PORTABLE_MENU)

namespace nl::maps {

bool available() { return false; }
void request(const char*) {}
int pending() { return 0; }
ImTextureID texture(const char*) { return 0; }
bool texture_size(const char*, int*, int*) { return false; }
bool draw(ImDrawList*, const char*, const ImVec2&, const ImVec2&, ImU32, float, ImDrawFlags, float) { return false; }

} // namespace nl::maps

#endif
