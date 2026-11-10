// Menu source watermark: @use_sberbank
// Non-Windows stub of the inventory model inspector.
//
// The real implementation lives in src/cheat/port/model_preview_bridge.cpp, which is only
// part of the Windows build (the cheat library itself is Windows-only). This translation
// unit keeps the menu's call sites compiling on the GLFW/OpenGL preview build, where the
// inventory keeps using the shipped pictures.

#include "menu/model_preview.h"

#if !defined(_WIN32) || defined(KAMSHAKO_PORTABLE_MENU)

namespace nl::model_preview {

void set_device(void*, void*) {}
void release() {}

bool available() { return false; }

void set_model(const char*) {}

void set_finish(const char*, const char*, const char*, const char*, const char*, const char*,
                bool) {}
void clear_finish() {}
void set_wear(float) {}
void set_chams(int, const ImVec4&, float, bool) {}

void orbit(float, float) {}
void zoom(float) {}
void reset_view() {}

Frame render(float, float) { return Frame{}; }

} // namespace nl::model_preview

#endif
