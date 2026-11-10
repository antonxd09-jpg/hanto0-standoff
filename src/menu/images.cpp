// Menu source watermark: @use_sberbank
#include "menu/images.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "assets.h"
#include "imgui_internal.h"
#include "menu/draw.h"
#include "menu/inventory_catalog.h"

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace nl::images {
namespace {

using Clock = std::chrono::steady_clock;

constexpr int k_max_level = 3;
constexpr double k_decode_budget_ms = 6.0;
constexpr float k_fade_time = 0.18f;

struct Texture {
    ImTextureData* tex = nullptr;
    double ready_time = 0.0;
};

std::unordered_map<int, Texture> g_cache; // key: image * 8 + level
Clock::time_point g_frame_start;
bool g_decoded_this_frame = false;

int key_of(int image, int level) { return image * 8 + level; }

// Halves a premultiplied RGBA image with a 2x2 box filter (odd edges repeat the last pixel).
void halve(std::vector<unsigned short>& px, int& w, int& h)
{
    const int nw = (w + 1) / 2, nh = (h + 1) / 2;
    std::vector<unsigned short> out(static_cast<size_t>(nw) * nh * 4);
    for (int y = 0; y < nh; ++y) {
        const int y0 = y * 2, y1 = ImMin(y * 2 + 1, h - 1);
        for (int x = 0; x < nw; ++x) {
            const int x0 = x * 2, x1 = ImMin(x * 2 + 1, w - 1);
            for (int c = 0; c < 4; ++c) {
                const unsigned sum = px[(static_cast<size_t>(y0) * w + x0) * 4 + c] +
                                     px[(static_cast<size_t>(y0) * w + x1) * 4 + c] +
                                     px[(static_cast<size_t>(y1) * w + x0) * 4 + c] +
                                     px[(static_cast<size_t>(y1) * w + x1) * 4 + c];
                out[(static_cast<size_t>(y) * nw + x) * 4 + c] = static_cast<unsigned short>((sum + 2) / 4);
            }
        }
    }
    px.swap(out);
    w = nw;
    h = nh;
}

ImTextureData* decode(int image, int level)
{
    const inv::Image& info = inv::images[image];
    int w = 0, h = 0, n = 0;
    unsigned char* rgba = stbi_load_from_memory(assets::inventory_pak.data + info.offset, static_cast<int>(info.size),
                                                &w, &h, &n, 4);
    if (!rgba)
        return nullptr;

    // Premultiplied, 8.8 fixed point, so that halving doesn't bleed the color of transparent pixels.
    std::vector<unsigned short> px(static_cast<size_t>(w) * h * 4);
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
        const unsigned a = rgba[i * 4 + 3];
        for (int c = 0; c < 3; ++c)
            px[i * 4 + c] = static_cast<unsigned short>((rgba[i * 4 + c] * a * 256u + 127u) / 255u);
        px[i * 4 + 3] = static_cast<unsigned short>(a * 256u);
    }
    stbi_image_free(rgba);
    for (int l = 0; l < level && (w > 1 || h > 1); ++l)
        halve(px, w, h);

    ImTextureData* tex = IM_NEW(ImTextureData)();
    tex->Create(ImTextureFormat_RGBA32, w, h);
    tex->UseColors = true;
    auto* out = static_cast<unsigned char*>(tex->GetPixelsAt(0, 0));
    for (size_t i = 0; i < static_cast<size_t>(w) * h; ++i) {
        const unsigned a = px[i * 4 + 3];
        for (int c = 0; c < 3; ++c)
            out[i * 4 + c] = a ? static_cast<unsigned char>(ImMin(255u, (px[i * 4 + c] * 255u + a / 2) / a)) : 0;
        out[i * 4 + 3] = static_cast<unsigned char>((a + 128u) / 256u);
    }
    ImGui::RegisterUserTexture(tex);
    return tex;
}

// The texture for `image` at `level`, or null while it is waiting for its turn to be decoded.
const Texture* get(int image, int level)
{
    const int key = key_of(image, level);
    auto it = g_cache.find(key);
    if (it != g_cache.end())
        return it->second.tex ? &it->second : nullptr;

    const double spent = std::chrono::duration<double, std::milli>(Clock::now() - g_frame_start).count();
    if (g_decoded_this_frame && spent > k_decode_budget_ms)
        return nullptr;
    g_decoded_this_frame = true;
    Texture t;
    t.tex = decode(image, level);
    t.ready_time = ImGui::GetTime();
    const Texture& slot = g_cache.emplace(key, t).first->second;
    return slot.tex ? &slot : nullptr;
}

bool valid(int image) { return image >= 0 && image < inv::image_count; }

} // namespace

void new_frame()
{
    g_frame_start = Clock::now();
    g_decoded_this_frame = false;
}

void shutdown()
{
    for (auto& [key, t] : g_cache) {
        if (!t.tex)
            continue;
        ImGui::UnregisterUserTexture(t.tex);
        IM_DELETE(t.tex);
    }
    g_cache.clear();
}

void fit(int image, const ImVec2& min, const ImVec2& max, bool crop, ImVec2* out_min, ImVec2* out_max, float max_scale)
{
    if (!valid(image)) {
        *out_min = min;
        *out_max = min;
        return;
    }
    const inv::Image& info = inv::images[image];
    const float fw = crop ? info.w : info.canvas_w, fh = crop ? info.h : info.canvas_h;
    float s = ImMin((max.x - min.x) / fw, (max.y - min.y) / fh);
    if (max_scale > 0.0f)
        s = ImMin(s, max_scale);
    const ImVec2 origin(min.x + ((max.x - min.x) - fw * s) * 0.5f, min.y + ((max.y - min.y) - fh * s) * 0.5f);
    const float ox = crop ? 0.0f : info.x, oy = crop ? 0.0f : info.y;
    *out_min = ImVec2(draw::snap(origin.x + ox * s), draw::snap(origin.y + oy * s));
    *out_max = ImVec2(draw::snap(origin.x + (ox + info.w) * s), draw::snap(origin.y + (oy + info.h) * s));
}

void draw_rect(ImDrawList* dl, int image, const ImVec2& min, const ImVec2& max, ImU32 tint)
{
    if (!valid(image) || max.x <= min.x || max.y <= min.y)
        return;
    const ImVec2 clip_min = dl->GetClipRectMin(), clip_max = dl->GetClipRectMax();
    if (max.x < clip_min.x || min.x > clip_max.x || max.y < clip_min.y || min.y > clip_max.y)
        return;
    const inv::Image& info = inv::images[image];
    const float ratio = static_cast<float>(info.w) / ImMax(max.x - min.x, 1.0f);
    const int level = ImClamp(static_cast<int>(std::floor(std::log2(ImMax(ratio, 1.0f)) + 0.15f)), 0, k_max_level);
    const Texture* t = get(image, level);
    if (!t)
        return;
    const float fade = ImSaturate(static_cast<float>(ImGui::GetTime() - t->ready_time) / k_fade_time);
    dl->AddImage(t->tex->GetTexRef(), min, max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
                 draw::with_alpha(tint, fade * (2.0f - fade)));
}

void draw(ImDrawList* dl, int image, const ImVec2& min, const ImVec2& max, ImU32 tint, bool crop, float max_scale)
{
    ImVec2 a, b;
    fit(image, min, max, crop, &a, &b, max_scale);
    draw_rect(dl, image, a, b, tint);
}

} // namespace nl::images
