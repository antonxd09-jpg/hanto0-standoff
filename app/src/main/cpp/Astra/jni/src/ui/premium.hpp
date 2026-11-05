// ASTRA_MULOAD_GUI_V2
// FloatingModMenu / BlackStyle shell + Astra feature set.
// Layout, fonts, colors, navigation and launcher are ported from the
// muload project; tabs and feature wiring stay Astra's.
#pragma once
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"
#include "features/aim/core.hpp"
#include "features/aim/assists.hpp"
#include "features/visuals/esp_runtime.hpp"
#include "ui/fonts/muload_fonts.hpp"
#include "ui/muload/theme.hpp"

// Defined in entry.cpp right above this include.
extern std::vector<EspScreenBox> g_espScreenBoxes;

// ── session state ────────────────────────────────────────────────────────────
static ImVec4 g_accent4{.00f, .26f, .62f, 1.f};
static ImVec4 g_espBoxCol{.94f, .95f, .97f, 1.f}, g_espHpCol{.36f, .87f, .65f, 1.f};
static ImVec4 g_espWepCol{.91f, .92f, .95f, 1.f}, g_espAmmoCol{.68f, .72f, .79f, 1.f};
static float g_S = 1.0f;
static bool menuOpen() { return g_menuOpen.load(std::memory_order_relaxed); }
static void setMenuOpen(bool open) { g_menuOpen.store(open, std::memory_order_relaxed); }
static int g_tab = 0;
static bool previewEnabled = true;
static float g_rainbowPhase = 0.0f;
static ImVec2 g_menuPos{-1.0f, -1.0f};
static ImVec2 g_menuScreen{0.0f, 0.0f};

struct KwB {
    bool esp_enabled = false, esp_box = true, esp_hp = true, esp_armor = true;
    bool esp_wep = true, esp_ammo = true;
    bool watermark = false;
};
static KwB g_b;

static const char* tabs[] = {"Aim", "Visuals", "Silent", "Anti-Aim", "Config"};

#include "premium_esp_extra.hpp"
#include "premium_arrows.hpp"

// ── aim projection cache (unchanged from Astra) ──────────────────────────────
namespace astra_premium {
struct Projection {
    float width = 0, height = 0, p00 = 0, p11 = 0, shiftX = 0, shiftY = 0;
    double captured = 0;
};
inline std::mutex projectionMutex;
inline Projection projection;
inline double monotonicSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void setAimProjection(float w, float h, float p00, float p11, float sx = 0, float sy = 0) {
    std::lock_guard<std::mutex> lock(projectionMutex);
    projection = {w, h, p00, p11, sx, sy, monotonicSeconds()};
}
inline void clearAimProjection() {
    std::lock_guard<std::mutex> lock(projectionMutex);
    projection = {};
}
}

static void updateAimProjection() {
    if (!astra::aim::configuration().normal.showFov) {
        astra_premium::clearAimProjection();
        return;
    }
    astra::esp::CameraMatrixCache m{};
    std::uintptr_t camera = 0;
    if (!astra::esp::detail::readCameraForPresent(m, camera)) {
        astra_premium::clearAimProjection();
        return;
    }
    const auto& p = m.projection.value;
    float scale = -p[11];
    if (!std::isfinite(scale) || scale <= .0001f) {
        astra_premium::clearAimProjection();
        return;
    }
    for (int index : {1, 3, 4, 7, 12, 13, 15})
        if (std::fabs(p[index] / scale) > .001f) {
            astra_premium::clearAimProjection();
            return;
        }
    auto size = ImGui::GetIO().DisplaySize;
    float p00 = std::fabs(p[0] / scale), p11 = std::fabs(p[5] / scale);
    float sx = p[8] / scale, sy = p[9] / scale;
    auto g = astra::aim::assists::fovGeometry(
        astra::aim::configuration().normal.fov, size.x, size.y, p00, p11, sx, sy);
    if (!g.valid) {
        astra_premium::clearAimProjection();
        return;
    }
    astra_premium::setAimProjection(size.x, size.y, p00, p11, sx, sy);
}

static astra::aim::assists::FovGeometry currentFovGeometry(float degrees) {
    astra_premium::Projection p;
    {
        std::lock_guard<std::mutex> lock(astra_premium::projectionMutex);
        p = astra_premium::projection;
    }
    double age = astra_premium::monotonicSeconds() - p.captured;
    auto display = ImGui::GetIO().DisplaySize;
    if (age < 0 || age >= .25 || std::fabs(p.width - display.x) > 1 ||
        std::fabs(p.height - display.y) > 1)
        return {};
    return astra::aim::assists::fovGeometry(degrees, p.width, p.height, p.p00, p.p11, p.shiftX, p.shiftY);
}

static void renderFov(const astra::aim::Settings& settings) {
    if (!settings.showFov) return;
    auto g = currentFovGeometry(settings.fov);
    if (!g.valid || g.coversScreen) return;
    auto* d = ImGui::GetBackgroundDrawList();
    d->PushClipRect(ImVec2(0, 0), ImGui::GetIO().DisplaySize, true);
    d->PathClear();
    for (int i = 0; i <= 256; ++i) {
        float a = float(i) * 6.28318530718f / 256;
        d->PathLineTo(ImVec2(g.centerX + std::cos(a) * g.radiusX, g.centerY + std::sin(a) * g.radiusY));
    }
    d->PathStroke(ImGui::ColorConvertFloat4ToU32(g_accent4), 0, 2.0f);
    d->PopClipRect();
}

// ── ESP overlay (Astra rendering, unchanged) ─────────────────────────────────
static void renderEsp() {
    if (!g_b.esp_enabled || !g_espBoxesEnabled.load()) return;
    std::vector<EspScreenBox> boxes;
    {
        std::lock_guard<std::mutex> lock(g_espMutex);
        boxes = g_espScreenBoxes;
    }
    auto* d = ImGui::GetBackgroundDrawList();
    d->PushClipRect(ImVec2(0, 0), ImGui::GetIO().DisplaySize, true);
    for (const auto& b : boxes) astra_esp_extra::draw(d, b);
    d->PopClipRect();
}

// ── preview (Astra tuning helper) ────────────────────────────────────────────
static void preview() {
    const float w = std::max(1.f, ImGui::GetContentRegionAvail().x);
    const auto p = ImGui::GetCursorScreenPos();
    const float h = 260.0f, cx = p.x + w * .5f;
    ImGui::Dummy(ImVec2(w, h));
    auto* d = ImGui::GetWindowDrawList();
    d->PushClipRect(p, ImVec2(p.x + w, p.y + h), true);
    d->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(10, 10, 10, 255), 3.f);
    for (float x = p.x + 20; x < p.x + w; x += 24) d->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + h), IM_COL32(255, 255, 255, 5));
    for (float y = p.y + 20; y < p.y + h; y += 24) d->AddLine(ImVec2(p.x, y), ImVec2(p.x + w, y), IM_COL32(255, 255, 255, 5));
    const float top = p.y + 25, bottom = p.y + h - 65, bw = std::min(w * .42f, 80.f);
    const float bodyHeight = bottom - top;
    auto body = IM_COL32(62, 66, 76, 255);
    d->AddCircleFilled(ImVec2(cx, top + bodyHeight * .09f), bodyHeight * .065f, body, 24);
    d->AddRectFilled(ImVec2(cx - bw * .17f, top + bodyHeight * .18f),
                     ImVec2(cx + bw * .17f, top + bodyHeight * .59f), body, 5);
    for (int side : {-1, 1}) {
        d->AddLine(ImVec2(cx + side * bw * .22f, top + bodyHeight * .22f),
                   ImVec2(cx + side * bw * .33f, top + bodyHeight * .62f), body, 8);
        d->AddLine(ImVec2(cx + side * bw * .10f, top + bodyHeight * .56f),
                   ImVec2(cx + side * bw * .20f, bottom - 4), body, 9);
    }
    if (g_b.esp_enabled) {
        EspScreenBox b{};
        b.left = cx - bw * .5f;
        b.right = cx + bw * .5f;
        b.top = top;
        b.bottom = bottom;
        b.enemy = true;
        b.statsValid = true;
        b.weaponValid = true;
        b.hp = 74;
        b.maxHp = 100;
        b.armor = 60;
        b.maxArmor = 100;
        if (astra_esp_extra::settings.lowHealth) b.hp = 20;
        b.weaponId = 7;
        b.magazineAmmo = astra_esp_extra::settings.emptyMagazine ? 0 : 24;
        b.reserveAmmo = 90;
        b.distanceUnits = 24.f;
        b.distanceValid = true;
        astra_esp_extra::draw(d, b);
    }
    d->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(40, 40, 40, 255), 3.f);
    d->PopClipRect();
}

// ── tab content ──────────────────────────────────────────────────────────────
static astra::aim::Configuration edit;
static bool dirty = false, resetRequested = false;

static void mark(bool changed) {
    if (changed) dirty = true;
}

static void aimTab() {
    if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen)) {
        mark(ImGui::Checkbox("Enabled##aim", &edit.normal.enabled));
        mark(ImGui::Checkbox("Visible check##aim", &edit.normal.visibleCheck));
        mark(ImGui::SliderFloat("FOV##aim", &edit.normal.fov, .1f, 180.f, "%.1f deg"));
        mark(ImGui::Checkbox("Show FOV##aim", &edit.normal.showFov));
        mark(ImGui::SliderFloat("Smoothing##aim", &edit.normal.smoothing, 0.f, 1.f, "%.3f s"));
        ImGui::TextDisabled("0 = instant; higher = smoother.");
    }
    if (ImGui::CollapsingHeader("Recoil control")) {
        mark(ImGui::Checkbox("RCS##aim", &edit.normal.rcs));
        mark(ImGui::SliderFloat("RCS strength##aim", &edit.normal.rcsStrength, 0.f, 100.f, "%.0f%%"));
    }
    if (ImGui::CollapsingHeader("Target selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (unsigned i = 0; i < unsigned(astra::aim::Body::Count); ++i) {
            bool value = (edit.normal.bodies & (1u << i)) != 0;
            if (ImGui::Checkbox(astra::aim::kBodyNames[i], &value)) {
                if (value)
                    edit.normal.bodies |= 1u << i;
                else
                    edit.normal.bodies &= ~(1u << i);
                dirty = true;
            }
        }
        if (!edit.normal.bodies) ImGui::TextDisabled("Select at least one body region.");

        ImGui::Separator();
        if (!edit.normal.enabled)
            ImGui::TextDisabled("Status: disabled");
        else if (edit.pauseInMenu && menuOpen())
            ImGui::TextDisabled("Status: paused while menu is open");
        else if (!astra::aim::g_normalHookReady.load())
            ImGui::TextDisabled("Status: waiting for angle runtime hook");
        else
            ImGui::Text("Status: %s", astra::aim::stateText(astra::aim::g_normalState.load()));
        if (edit.normal.visibleCheck && !astra::aim::g_visibilityReady.load())
            ImGui::TextDisabled("Visibility profile not ready; targeting is blocked.");
        if (edit.normal.rcs && !astra::aim::g_rcsReady.load())
            ImGui::TextDisabled("RCS profile not ready");

        ImGui::Spacing();
        ImGui::Text("RCS: enabled=%d ready=%d strength=%.0f", int(edit.normal.rcs),
                    int(astra::aim::g_rcsReady.load()), double(edit.normal.rcsStrength));
        ImGui::Text("Reads: %llu | Rejected: %llu | Applied: %llu",
                    (unsigned long long)astra::aim::debug::rcsReads.load(),
                    (unsigned long long)astra::aim::debug::rcsRejected.load(),
                    (unsigned long long)astra::aim::debug::rcsApplied.load());
        ImGui::Text("Last recoil: pitch=%.4f yaw=%.4f", double(astra::aim::debug::recoilPitch.load()),
                    double(astra::aim::debug::recoilYaw.load()));
        ImGui::Text("Attack raw: %d", astra::aim::debug::attackRaw.load());
        if (edit.normal.showFov) {
            auto g = currentFovGeometry(edit.normal.fov);
            if (!g.valid)
                ImGui::TextDisabled("FOV overlay: waiting for fresh camera projection");
            else if (g.coversScreen)
                ImGui::TextDisabled("FOV covers the screen; outline is outside the view.");
        }
    }
}

static void visualsTab() {
    if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled##esp", &g_b.esp_enabled);
        ImGui::Checkbox("Bounding box##esp", &g_b.esp_box);
        ImGui::ColorEdit4("Box color##esp", &g_espBoxCol.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Health bar##esp", &g_b.esp_hp);
        ImGui::ColorEdit4("Health color##esp", &g_espHpCol.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Armor bar##esp", &g_b.esp_armor);
        ImGui::ColorEdit4("Armor color##esp", &astra_esp_extra::settings.armorColor.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Weapon info##esp", &g_b.esp_wep);
        ImGui::ColorEdit4("Weapon color##esp", &g_espWepCol.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Ammo##esp", &g_b.esp_ammo);
        ImGui::ColorEdit4("Ammo color##esp", &g_espAmmoCol.x, ImGuiColorEditFlags_NoInputs);
    }
    if (ImGui::CollapsingHeader("ESP style")) {
        auto& a = astra_esp_extra::settings;
        ImGui::Combo("Frame style##esp", &astra_esp_extra::frameStyle,
                     "Classic\0Corners\0Rounded\0Dashed\0Side brackets\0");
        ImGui::Checkbox("Box fill##esp", &a.fill);
        ImGui::SliderFloat("Fill opacity##esp", &a.fillAlpha, 0.f, .35f, "%.2f");
        ImGui::Checkbox("Snaplines##esp", &a.snaplines);
        ImGui::Checkbox("Box head marker##esp", &a.headMarker);
        ImGui::SliderFloat("Line width##esp", &a.thickness, 1.f, 4.f, "%.1f");
        ImGui::SliderFloat("ESP text scale##esp", &a.textScale, .5f, 2.f, "%.2f");
        ImGui::Checkbox("Text shadow##esp", &a.shadow);
        ImGui::Checkbox("Text outline##esp", &a.outline);
        ImGui::Checkbox("HP / armor divisions##esp", &astra_esp_extra::barDivisions);
        ImGui::Checkbox("Soft frame halo##esp", &astra_esp_extra::softHalo);
        ImGui::SliderFloat("Halo opacity##esp", &astra_esp_extra::haloOpacity, 0.f, .25f, "%.2f");
        ImGui::Checkbox("Arrows##esp", &astra_arrows::settings.enabled);
        ImGui::ColorEdit4("Arrow color##esp", &astra_arrows::settings.color.x, ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Arrow halo##esp", &astra_arrows::settings.glow);
        ImGui::SliderFloat("Arrow size##esp", &astra_arrows::settings.size, 7.f, 24.f, "%.0f px");
        ImGui::SliderFloat("Arrow inset##esp", &astra_arrows::settings.inset, 30.f, 180.f, "%.0f px");
    }
    if (ImGui::CollapsingHeader("Information")) {
        auto& info = astra_esp_extra::settings;
        ImGui::Checkbox("Distance##info", &info.distance);
        ImGui::SliderFloat("Meters per world unit##info", &info.metersPerUnit, .01f, 10.f, "%.2f");
        ImGui::Checkbox("HP number##info", &info.hpNumber);
        ImGui::Checkbox("Armor number##info", &info.armorNumber);
        ImGui::Checkbox("Low health marker##info", &info.lowHealth);
        ImGui::SliderFloat("Low HP threshold##info", &info.lowHealthPercent, 5.f, 50.f, "%.0f%%");
        ImGui::Checkbox("Empty magazine marker##info", &info.emptyMagazine);
        ImGui::TextDisabled("Distance requires fresh local and enemy positions.");
    }
    if (ImGui::CollapsingHeader("Third person / Camera test V2")) {
        ImGui::PushID("astra-camera-panel");
        astra_camera_test::controls();
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Hitlog / Hitsound")) {
        bool enabled = astra_hitlog::enabled.load();
        if (ImGui::Checkbox("Hitlog##hitlog", &enabled)) {
            astra_hitlog::enabled.store(enabled);
            if (!enabled) {
                std::lock_guard<std::mutex> lock(astra_hitlog::mutex);
                astra_hitlog::lines.clear();
                astra_hitlog::localWorld = nullptr;
            }
        }
        const int state = astra_hitlog::state.load();
        ImGui::TextDisabled("%s", state == 2 ? "Hook installed"
                               : state == -1 ? "Code mismatch"
                               : state == -2 ? "Hook installation failed"
                               : state == 1 ? "Installing..."
                                            : "Waiting for enabled / local player");
        ImGui::Text("Events: %llu | Local hits: %llu", astra_hitlog::received.load(),
                    astra_hitlog::accepted.load());
        astra_hitlog::deathDiagnostics();
    }
    if (ImGui::CollapsingHeader("GPU Night mode")) {
        astra_night_diag::controls();
    }
    if (ImGui::CollapsingHeader("Preview")) {
        ImGui::Checkbox("Show ESP preview##preview", &previewEnabled);
        if (previewEnabled) preview();
    }
}

static void silentTab() {
    static bool sa_enabled = false, sa_vischeck = false, sa_backtrack = false, sa_prediction = false;
    static float sa_fov = 15.f, sa_smooth = 0.05f, sa_backtrack_ms = 200.f;
    static int sa_hitbox = 0;
    static const char* sa_hitboxes = "Head\0Neck\0Chest\0Pelvis\0";
    ImGui::Checkbox("Silent Aim##silent", &sa_enabled);
    ImGui::TextDisabled("Fires at target position without moving crosshair.");
    if (ImGui::CollapsingHeader("Target", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("FOV##silent", &sa_fov, 1.f, 180.f, "%.1f deg");
        ImGui::SliderFloat("Smoothing##silent", &sa_smooth, 0.f, 1.f, "%.3f s");
        ImGui::Combo("Hitbox##silent", &sa_hitbox, sa_hitboxes);
    }
    if (ImGui::CollapsingHeader("Options")) {
        ImGui::Checkbox("Visible check##silent", &sa_vischeck);
        ImGui::Checkbox("Backtrack##silent", &sa_backtrack);
        if (sa_backtrack) ImGui::SliderFloat("Backtrack window##silent", &sa_backtrack_ms, 0.f, 400.f, "%.0f ms");
        ImGui::Checkbox("Movement prediction##silent", &sa_prediction);
    }
    ImGui::Separator();
    ImGui::TextDisabled("Runtime is present; this build keeps it disabled.");
}

static void antiAimTab() {
    static bool aa_enabled = false;
    static int aa_pitch = 0, aa_yaw = 0;
    static bool aa_random_pitch = false, aa_jitter = false;
    static float aa_jitter_range = 35.f, aa_jitter_speed = 1.f;
    static bool aa_spin = false;
    static float aa_spin_speed = 8.f;
    static int aa_spin_dir = 0;
    static bool aa_fakelag = false;
    static float aa_fakelag_ticks = 12.f;
    static bool aa_fakelag_var = false;
    static float aa_fakelag_var_range = 4.f;
    static bool aa_lby = false, aa_slide = false, aa_edge = false, aa_micro_move = false;
    static float aa_edge_range = 48.f, aa_micro_move_range = 6.f;
    static int aa_on_shot_mode = 0;
    static float aa_manual_angle = 90.f;
    static bool aa_manual_left = false, aa_manual_right = false;
    static const char* aa_pitch_modes = "Off\0Down\0Up\0Zero\0Random\0";
    static const char* aa_yaw_modes = "Off\0Backward\0Sideways L\0Sideways R\0Spin\0Jitter\0Manual\0At target\0";
    static const char* aa_spin_dirs = "CW\0CCW\0Random\0";
    static const char* aa_shot_modes = "Off\0Real on shot\0Jitter on shot\0";

    ImGui::Checkbox("Anti-Aim##aa", &aa_enabled);
    if (ImGui::CollapsingHeader("Pitch", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Mode##aa-pitch", &aa_pitch, aa_pitch_modes);
        ImGui::Checkbox("Random pitch##aa", &aa_random_pitch);
    }
    if (ImGui::CollapsingHeader("Yaw", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Mode##aa-yaw", &aa_yaw, aa_yaw_modes);
        if (aa_yaw == 4) {
            ImGui::Checkbox("Spin##aa", &aa_spin);
            ImGui::SliderFloat("Spin speed##aa", &aa_spin_speed, 1.f, 30.f, "%.0f deg/tick");
            ImGui::Combo("Direction##aa", &aa_spin_dir, aa_spin_dirs);
        }
        if (aa_yaw == 5) {
            ImGui::Checkbox("Jitter##aa", &aa_jitter);
            ImGui::SliderFloat("Jitter range##aa", &aa_jitter_range, 5.f, 180.f, "%.0f deg");
            ImGui::SliderFloat("Jitter speed##aa", &aa_jitter_speed, .5f, 5.f, "%.1f");
        }
        if (aa_yaw == 6) {
            ImGui::SliderFloat("Manual angle##aa", &aa_manual_angle, -180.f, 180.f, "%.0f deg");
            ImGui::Checkbox("Manual left##aa", &aa_manual_left);
            ImGui::Checkbox("Manual right##aa", &aa_manual_right);
        }
    }
    if (ImGui::CollapsingHeader("On shot")) {
        ImGui::Combo("Mode##aa-shot", &aa_on_shot_mode, aa_shot_modes);
    }
    if (ImGui::CollapsingHeader("Fake lag")) {
        ImGui::Checkbox("Fake lag##aa", &aa_fakelag);
        if (aa_fakelag) {
            ImGui::SliderFloat("Ticks##aa", &aa_fakelag_ticks, 1.f, 22.f, "%.0f");
            ImGui::Checkbox("Variance##aa", &aa_fakelag_var);
            if (aa_fakelag_var) ImGui::SliderFloat("Variance range##aa", &aa_fakelag_var_range, 1.f, 8.f, "%.0f");
        }
    }
    if (ImGui::CollapsingHeader("Extra")) {
        ImGui::Checkbox("LBY breaker##aa", &aa_lby);
        ImGui::Checkbox("Slide##aa", &aa_slide);
        ImGui::Checkbox("Edge detection##aa", &aa_edge);
        if (aa_edge) ImGui::SliderFloat("Edge range##aa", &aa_edge_range, 16.f, 128.f, "%.0f");
        ImGui::Checkbox("Micro movement##aa", &aa_micro_move);
        if (aa_micro_move) ImGui::SliderFloat("Micro range##aa", &aa_micro_move_range, 1.f, 20.f, "%.1f");
    }
    ImGui::Separator();
    ImGui::TextDisabled("Runtime is present; this build keeps it disabled.");
}

static void resetAll() {
    astra_camera_test::reset();
    astra_arrows::reset();
    astra_esp_extra::reset();
    astra_night::publish(astra_night::Settings{});
    edit = astra::aim::Configuration{};
    dirty = true;
    resetRequested = true;
    g_b = KwB{};
    previewEnabled = true;
    g_espBoxCol = ImVec4(.94f, .95f, .97f, 1.f);
    g_espHpCol = ImVec4(.36f, .87f, .65f, 1.f);
    g_espWepCol = ImVec4(.91f, .92f, .95f, 1.f);
    g_espAmmoCol = ImVec4(.68f, .72f, .79f, 1.f);
}

static void configTab() {
    ImGui::Checkbox("Watermark##cfg", &g_b.watermark);
    ImGui::Checkbox("Show ESP preview##cfg", &previewEnabled);
    if (ImGui::Button("Close menu", ImVec2(ImGui::GetContentRegionAvail().x, 0))) setMenuOpen(false);
    ImGui::Separator();
    ImGui::TextDisabled("Astra / Free");
    ImGui::Text("Profile: %s", g_offsetProfileReady.load() ? "ready" : "not matched");
    ImGui::Text("Aim hook: %s", astra::aim::g_normalHookReady.load() ? "ready" : "waiting");
    ImGui::Text("ESP hook: %s", astra::esp::g_playerUpdateHookReady.load() ? "ready" : "waiting");
    ImGui::Text("Hitlog hook: %d", astra_hitlog::state.load());
    ImGui::Text("Frame: %.1f ms", 1000.0f / std::max(1.0f, ImGui::GetIO().Framerate));
    ImGui::Separator();
    if (ImGui::Button("Reset settings", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        ImGui::OpenPopup("Reset settings?");
    if (ImGui::BeginPopupModal("Reset settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Restore all controls to defaults?");
        if (ImGui::Button("Reset##confirm", ImVec2(100, 0))) {
            resetAll();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##confirm", ImVec2(100, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void tabContent() {
    switch (g_tab) {
        case 0: aimTab(); break;
        case 1: visualsTab(); break;
        case 2: silentTab(); break;
        case 3: antiAimTab(); break;
        default: configTab(); break;
    }
}

// ── shell ────────────────────────────────────────────────────────────────────
static void drawLauncher() {
    ImGui::SetNextWindowPos(ImVec2(18.f, 42.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(150.f, 150.f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("##astra-open", nullptr, flags)) {
        ImGui::SetCursorPos(ImVec2(25.f, 25.f));
        if (ImGui::Button("OPEN", ImVec2(100.f, 100.f))) setMenuOpen(!menuOpen());
    }
    ImGui::End();
    ImGui::PopStyleVar(3);
}

static void drawWatermark() {
    if (!g_b.watermark) return;
    auto* d = ImGui::GetBackgroundDrawList();
    ImFont* font = astra::fonts::base ? astra::fonts::base : ImGui::GetFont();
    d->AddText(font, 30.0f, ImVec2(100.f, 30.f), IM_COL32(255, 255, 255, 255), "ASTRA | APK");
}

static void drawMenu() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 screen = io.DisplaySize;
    const float w = std::min(1200.f, screen.x - 12.f);
    const float h = std::min(760.f, screen.y - 24.f);
    if (w < 400.f || h < 300.f) return;
    if (g_menuPos.x < 0.0f || screen.x != g_menuScreen.x || screen.y != g_menuScreen.y) {
        g_menuPos = ImVec2((screen.x - w) * .5f, (screen.y - h) * .5f);
        g_menuScreen = screen;
    }
    g_menuPos.x = std::clamp(g_menuPos.x, 0.0f, std::max(0.0f, screen.x - w));
    g_menuPos.y = std::clamp(g_menuPos.y, 0.0f, std::max(0.0f, screen.y - h));
    ImGui::SetNextWindowPos(g_menuPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("##astra-menu", nullptr, flags)) {
        const ImVec2 pos = ImGui::GetWindowPos();
        const ImGuiStyle& shellStyle = ImGui::GetStyle();
        astra::gui::rainbowLine(ImGui::GetWindowDrawList(), pos.x + 4.f,
                                pos.y + shellStyle.WindowBorderSize, w - 8.f, 8.0f,
                                g_rainbowPhase, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        if (astra::fonts::big) ImGui::PushFont(astra::fonts::big);
        ImGui::TextUnformatted("Astra");
        if (astra::fonts::big) ImGui::PopFont();
        if (astra::fonts::med) ImGui::PushFont(astra::fonts::med);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        ImGui::TextUnformatted("APK");
        ImGui::PopStyleColor();
        if (astra::fonts::med) ImGui::PopFont();
        ImGui::PopStyleVar(5);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Drag by the header: invisible handle covering title/description.
        const ImVec2 afterHeader = ImGui::GetCursorScreenPos();
        const float headerH = std::max(1.0f, afterHeader.y - pos.y);
        ImGui::SetCursorScreenPos(ImVec2(pos.x + shellStyle.WindowBorderSize,
                                        pos.y + shellStyle.WindowBorderSize));
        ImGui::InvisibleButton("##menu-drag", ImVec2(std::max(1.0f, w - shellStyle.WindowBorderSize * 2.0f),
                                                    std::max(1.0f, headerH - shellStyle.WindowBorderSize)));
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImVec2 moved = g_menuPos;
            moved.x += ImGui::GetIO().MouseDelta.x;
            moved.y += ImGui::GetIO().MouseDelta.y;
            moved.x = std::clamp(moved.x, 0.0f, std::max(0.0f, screen.x - w));
            moved.y = std::clamp(moved.y, 0.0f, std::max(0.0f, screen.y - h));
            // Applied through SetNextWindowPos on the next frame so the
            // already-submitted title/rainbow do not shear against the body.
            g_menuPos = moved;
        }
        ImGui::SetCursorScreenPos(afterHeader);

        const float navW = std::max(110.f, w / 9.916f);
        const float contentH = ImGui::GetContentRegionAvail().y;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
        ImGui::BeginChild("##astra-nav", ImVec2(navW, contentH), false, ImGuiWindowFlags_NoScrollbar);
        float btn = std::min(100.f, (contentH - 4.f * 8.f) / 5.f);
        btn = std::max(40.f, btn);
        const char* icons[] = {ICON_FA_CROSSHAIRS, ICON_FA_EYE, ICON_FA_CIRCLE_NOTCH,
                               ICON_FA_SHIELD, ICON_FA_GEAR};
        for (int i = 0; i < 5; ++i) {
            ImGui::SetCursorPosX((navW - btn) * .5f);
            ImGui::PushID(i);
            ImFont* navIcon = astra::fonts::iconLarge ? astra::fonts::iconLarge : astra::fonts::icon;
            if (navIcon) ImGui::PushFont(navIcon);
            const bool pressed = ImGui::Button(icons[i], ImVec2(btn, btn));
            const bool hovered = ImGui::IsItemHovered();
            if (navIcon) ImGui::PopFont();
            if (pressed) g_tab = i;
            if (hovered) ImGui::SetTooltip("%s", tabs[i]);
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::SameLine(0, 12.f);
        ImGui::BeginChild("##astra-content", ImVec2(0, contentH), false,
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
        tabContent();
        ImGui::EndChild();
    }
    ImGui::End();
}

static void commit() {
    if (!dirty) return;
    auto c = resetRequested ? astra::aim::Configuration{} : astra::aim::configuration();
    c.normal.enabled = edit.normal.enabled;
    c.normal.visibleCheck = edit.normal.visibleCheck;
    c.normal.fov = edit.normal.fov;
    c.normal.showFov = edit.normal.showFov;
    c.normal.bodies = edit.normal.bodies;
    c.normal.smoothing = edit.normal.smoothing;
    c.normal.rcs = edit.normal.rcs;
    c.normal.rcsStrength = edit.normal.rcsStrength;
    c.silent.enabled = false;
    c.autoFire.enabled = false;
    astra::aim::publish(c);
    dirty = false;
    resetRequested = false;
}

static void renderMinimalMenu() {
    edit = astra::aim::configuration();
    dirty = false;
    resetRequested = false;
    astra::aim::g_menuOpen.store(menuOpen(), std::memory_order_release);

    updateAimProjection();
    renderEsp();
    astra_arrows::render();
    renderFov(edit.normal);
    if (menuOpen()) drawMenu();
    drawLauncher();
    drawWatermark();

    commit();
    astra::aim::g_menuOpen.store(menuOpen(), std::memory_order_release);
}
