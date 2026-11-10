// Menu source watermark: @use_sberbank
// Desktop harness: no game archive, so the list only holds Default.
#include "menu/sky_list.h"

#if !defined(_WIN32) || defined(KAMSHAKO_PORTABLE_MENU)

namespace nl::skies {

namespace {
const std::vector<Entry>& defaults()
{
    static const std::vector<Entry> list{ Entry{ "Default", "", false } };
    return list;
}
} // namespace

void set_device(void*, void*) {}
void release() {}
const std::vector<Entry>& list() { return defaults(); }
bool ready() { return true; }
void refresh() {}
void open_folder() {}
void warm() {}
int pending() { return 0; }
bool draw(ImDrawList*, int, const ImVec2&, const ImVec2&, ImU32, float, ImDrawFlags, float) { return false; }
int find(const char*) { return 0; }

} // namespace nl::skies

#endif
