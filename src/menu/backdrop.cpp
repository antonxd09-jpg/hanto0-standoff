// Menu source watermark: @use_sberbank
#include "menu/backdrop.h"

#include <cmath>
#include <cstdio>

#include "menu/draw.h"
#include "menu/fonts.h"
#include "menu/style.h"

namespace nl {

void draw_backdrop(ImDrawList* dl, const ImVec2& size, bool menu_visible)
{
    using namespace nl::style;
    dl->AddRectFilledMultiColor(ImVec2(0.0f, 0.0f), size, rgb(24, 26, 33), rgb(20, 22, 29), rgb(9, 10, 13), rgb(11, 12, 16));

    // Faint grid, like the floor of an empty map.
    const ImU32 grid = rgb(255, 255, 255, 6);
    const float step = draw::snap(px(40.0f));
    for (float x = step; x < size.x; x += step)
        draw::vline(dl, x, 0.0f, size.y, grid);
    for (float y = step; y < size.y; y += step)
        draw::hline(dl, 0.0f, size.x, y, grid);

    char hint[128];
    if (menu_visible)
        std::snprintf(hint, sizeof(hint), "INSERT  hide menu      CTRL + WHEEL  zoom %d%%      drag to move",
                      static_cast<int>(std::lround(ui_scale * 100.0f)));
    else
        std::snprintf(hint, sizeof(hint), "INSERT  show menu");
    draw::text(dl, fonts().medium, font_size::header, ImVec2(px(14.0f), size.y - px(20.0f)), rgb(120, 123, 133, 160),
               hint);
}

} // namespace nl
