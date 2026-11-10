// Menu source watermark: @use_sberbank
#include "menu/blur.h"

namespace nl::blur {
namespace {

Backend g_backend;

} // namespace

void set_backend(const Backend& backend) { g_backend = backend; }

bool available() { return g_backend.capture && g_backend.texture != ImTextureID_Invalid; }

void capture(ImDrawList* dl)
{
    if (!available())
        return;
    dl->AddCallback(g_backend.capture, nullptr);
    if (ImDrawCallback reset = ImGui::GetPlatformIO().DrawCallback_ResetRenderState)
        dl->AddCallback(reset, nullptr);
}

void image(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImDrawFlags flags)
{
    if (!available())
        return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    if (display.x <= 0.0f || display.y <= 0.0f)
        return;
    const ImVec2 uv0(min.x / display.x, min.y / display.y), uv1(max.x / display.x, max.y / display.y);
    dl->AddImageRounded(ImTextureRef(g_backend.texture), min, max, uv0, uv1, IM_COL32_WHITE, rounding, flags);
}

void backdrop(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, ImDrawFlags flags)
{
    capture(dl);
    image(dl, min, max, rounding, flags);
}

} // namespace nl::blur
