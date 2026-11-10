// Menu source watermark: @use_sberbank
#include "menu/fonts.h"

#include "assets.h"
#include "menu/style.h"

namespace nl {
namespace {

Fonts g_fonts;

ImFont* add_font(const assets::Blob& blob, float size, const ImWchar* ranges = nullptr)
{
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 3;
    cfg.OversampleV = 1;
    return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(blob.data),
                                                      static_cast<int>(blob.size), size, &cfg, ranges);
}

} // namespace

void load_fonts()
{
    static const ImWchar icon_ranges[] = { 0xE000, 0xF8FF, 0 };

    g_fonts.medium = add_font(assets::noto_sans_medium, style::font_size::text);
    g_fonts.semibold = add_font(assets::noto_sans_semibold, style::font_size::title);
    g_fonts.black = add_font(assets::noto_sans_black, style::font_size::logo);
    g_fonts.icons = add_font(assets::fa_solid, style::font_size::icon, icon_ranges);
    ImGui::GetIO().FontDefault = g_fonts.medium;
}

void adopt_fonts(ImFont* medium,ImFont* semibold,ImFont* black,ImFont* icons){g_fonts.medium=medium;g_fonts.semibold=semibold;g_fonts.black=black;g_fonts.icons=icons;}

const Fonts& fonts() { return g_fonts; }

} // namespace nl
