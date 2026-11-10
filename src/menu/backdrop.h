// Menu source watermark: @use_sberbank
#pragma once

#include "imgui.h"

namespace nl {

// Dark scene behind the menu in the standalone viewer, plus the key hint in the corner.
void draw_backdrop(ImDrawList* dl, const ImVec2& size, bool menu_visible);

} // namespace nl
