// KAZAKH_GUI_MENU_V1
// Full KazakhDLC menu shell (layout, animation, widgets, shadow, watermark,
// toast) with Astra's name and feature set wired into its two columns.
#pragma once
#include <algorithm>
#include <atomic>
#include <string>
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
#include "features/aim/silent_diagnostics.hpp"
#include "features/visuals/esp_runtime.hpp"
#include "features/visuals/astra_hitlog.hpp"
#include "features/visuals/astra_camera_test.hpp"
#include "features/visuals/astra_night.hpp"
#include "features/visuals/astra_night_diag.hpp"

#include "ui/kazakh/box_shadow.hpp"
#include "ui/kazakh/theme.hpp"
#include "ui/kazakh/widgets.hpp"
#include "ui/kazakh/fonts.hpp"

// Defined in entry.cpp right above this include.
using EspScreenBox = astra::esp::ScreenBox;
extern std::vector<EspScreenBox> g_espScreenBoxes;
extern std::mutex g_espMutex;
extern std::atomic<bool> g_espBoxesEnabled;
extern std::atomic<bool> g_offsetProfileReady;

// ── session state (KazakhDLC naming kept where it is part of the shell) ──────
static ImVec4 g_espBoxCol{.94f, .95f, .97f, 1.f};
static ImVec4 g_espHpCol{.36f, .87f, .65f, 1.f};
static ImVec4 g_espWepCol{.91f, .92f, .95f, 1.f};
static ImVec4 g_espAmmoCol{.68f, .72f, .79f, 1.f};
static float g_S = 1.0f;

static bool menuOpen() { return g_menuOpen.load(std::memory_order_relaxed); }
static void setMenuOpen(bool open) { g_menuOpen.store(open, std::memory_order_relaxed); }

static int g_tab = 0;
static bool previewEnabled = true;
static ImVec2 g_menuPos{-1.0f, -1.0f};
static ImVec2 g_menuScreen{0.0f, 0.0f};
static bool g_boels = false;
static float g_lerped = 75.0f; // starts collapsed because Astra starts closed
static constexpr float kOpenedS = 450.0f;
static constexpr float kClosedS = 75.0f;
static constexpr int g_openStyle = 1; // Always use the watermark toggle.
static float g_dragDistance = 0.0f;
static bool g_dragMoved = false;


struct KwB {
    bool esp_enabled = false, esp_box = true, esp_hp = true, esp_armor = true;
    bool esp_wep = true, esp_ammo = true;
    bool watermark = true;
};
static KwB g_b;

static const char* kTabs[] = {"visuals", "silent", "config"};
static constexpr int kTabCount = sizeof(kTabs) / sizeof(kTabs[0]);

#include "ui/premium_esp_extra.hpp"
#include "ui/premium_arrows.hpp"

// ── aim projection cache (Astra) ────────────────────────────────────────────
namespace astra_kazakh {
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
    auto config = astra::aim::configuration();
    if (!config.normal.showFov && !config.silent.showFov) {
        astra_kazakh::clearAimProjection();
        return;
    }
    astra::esp::CameraMatrixCache m{};
    std::uintptr_t camera = 0;
    if (!astra::esp::detail::readCameraForPresent(m, camera)) {
        astra_kazakh::clearAimProjection();
        return;
    }
    const auto& p = m.projection.value;
    float scale = -p[11];
    if (!std::isfinite(scale) || scale <= .0001f) {
        astra_kazakh::clearAimProjection();
        return;
    }
    for (int index : {1, 3, 4, 7, 12, 13, 15})
        if (std::fabs(p[index] / scale) > .001f) {
            astra_kazakh::clearAimProjection();
            return;
        }
    auto size = ImGui::GetIO().DisplaySize;
    float p00 = std::fabs(p[0] / scale), p11 = std::fabs(p[5] / scale);
    float sx = p[8] / scale, sy = p[9] / scale;
    auto g = astra::aim::assists::fovGeometry(
        std::max(config.normal.fov, config.silent.fov), size.x, size.y, p00, p11, sx, sy);
    if (!g.valid) {
        astra_kazakh::clearAimProjection();
        return;
    }
    astra_kazakh::setAimProjection(size.x, size.y, p00, p11, sx, sy);
}

static astra::aim::assists::FovGeometry currentFovGeometry(float degrees) {
    astra_kazakh::Projection p;
    {
        std::lock_guard<std::mutex> lock(astra_kazakh::projectionMutex);
        p = astra_kazakh::projection;
    }
    double age = astra_kazakh::monotonicSeconds() - p.captured;
    auto display = ImGui::GetIO().DisplaySize;
    if (age < 0 || age >= .25 || std::fabs(p.width - display.x) > 1 ||
        std::fabs(p.height - display.y) > 1)
        return {};
    return astra::aim::assists::fovGeometry(degrees, p.width, p.height, p.p00, p.p11, p.shiftX, p.shiftY);
}

static void renderFov(const astra::aim::Settings& settings, ImU32 color) {
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
    d->PathStroke(color, 0, 2.0f);
    d->PopClipRect();
}

// ── ESP overlay (Astra rendering) ───────────────────────────────────────────
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

// ── preview ─────────────────────────────────────────────────────────────────
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

// ── tab content: Astra functions, Kazakh two-column layout ─────────────────
static astra::aim::Configuration edit;
static bool dirty = false, resetRequested = false;

static void mark(bool changed) {
    if (changed) dirty = true;
}

static void aimLeft() {
    if (ImGui::CollapsingHeader("Aimbot", ImGuiTreeNodeFlags_DefaultOpen)) {
        mark(ImGui::Checkbox("Enabled##aim", &edit.normal.enabled));
        mark(ImGui::Checkbox("Visible check##aim", &edit.normal.visibleCheck));
        mark(ImGui::SliderFloat("FOV##aim", &edit.normal.fov, .1f, 540.f, "%.1f deg"));
        mark(ImGui::Checkbox("Show FOV##aim", &edit.normal.showFov));
        mark(ImGui::SliderFloat("Smoothing##aim", &edit.normal.smoothing, 0.f, 1.f, "%.3f s"));
        ImGui::TextDisabled("0 = instant; higher = smoother.");
    }
    if (ImGui::CollapsingHeader("Recoil control")) {
        mark(ImGui::Checkbox("RCS##aim", &edit.normal.rcs));
        mark(ImGui::SliderFloat("RCS strength##aim", &edit.normal.rcsStrength, 0.f, 100.f, "%.0f%%"));
    }
}

static void aimRight() {
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

static void visualsLeft() {
    if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled##esp", &g_b.esp_enabled);
        ImGui::Checkbox("Bounding box##esp", &g_b.esp_box);
        ImGui::SameLine();
        kazakh::ColorPicker("Box color", g_espBoxCol);
        ImGui::Checkbox("Health bar##esp", &g_b.esp_hp);
        ImGui::SameLine();
        kazakh::ColorPicker("Health color", g_espHpCol);
        ImGui::Checkbox("Armor bar##esp", &g_b.esp_armor);
        ImGui::SameLine();
        kazakh::ColorPicker("Armor color", astra_esp_extra::settings.armorColor);
        ImGui::Checkbox("Weapon info##esp", &g_b.esp_wep);
        ImGui::SameLine();
        kazakh::ColorPicker("Weapon color", g_espWepCol);
        ImGui::Checkbox("Ammo##esp", &g_b.esp_ammo);
        ImGui::SameLine();
        kazakh::ColorPicker("Ammo color", g_espAmmoCol);
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
        ImGui::SameLine();
        kazakh::ColorPicker("Arrow color", astra_arrows::settings.color);
        ImGui::Checkbox("Arrow halo##esp", &astra_arrows::settings.glow);
        ImGui::SliderFloat("Arrow size##esp", &astra_arrows::settings.size, 7.f, 24.f, "%.0f px");
        ImGui::SliderFloat("Arrow inset##esp", &astra_arrows::settings.inset, 30.f, 180.f, "%.0f px");
    }
}

static void visualsRight() {
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

static void silentLeft() {
    mark(ImGui::Checkbox("Silent Aim##silent", &edit.silent.enabled));
    ImGui::TextDisabled("Rays are redirected at shot emission.");
    if (ImGui::CollapsingHeader("Target", ImGuiTreeNodeFlags_DefaultOpen)) {
        mark(ImGui::SliderFloat("FOV##silent", &edit.silent.fov, 1.f, 540.f, "%.1f deg"));
        mark(ImGui::Checkbox("Show FOV##silent", &edit.silent.showFov));
        mark(ImGui::Checkbox("Body aim (safe)##silent", &edit.silent.safeAim));
        mark(ImGui::SliderFloat("Max distance##silent", &edit.silent.maxDistance, 10.f, 300.f, "%.0f"));
        mark(ImGui::Checkbox("Visible check##silent", &edit.silent.visibleCheck));
        mark(ImGui::Checkbox("Lock target##silent", &edit.silent.lockTarget));
        ImGui::TextDisabled("Recoil options await a separate 1.1.0 port.");
    }
    if (ImGui::CollapsingHeader("Accuracy", ImGuiTreeNodeFlags_DefaultOpen)) {
        mark(ImGui::Checkbox("Multipoints##silent", &edit.silent.multipoints));
        if (edit.silent.multipoints)
            mark(ImGui::SliderFloat("Point scale##silent", &edit.silent.pointScale, 0.f, .9f, "%.2f"));
        mark(ImGui::Checkbox("Prediction##silent", &edit.silent.prediction));
        if (edit.silent.prediction)
            mark(ImGui::SliderFloat("Lead/rewind ms##silent", &edit.silent.predictionMs, -150.f, 150.f, "%.0f"));
    }
}

static void silentRight() {
    if (ImGui::CollapsingHeader("Hitboxes", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (unsigned i = 0; i < unsigned(astra::aim::Body::Count); ++i) {
            bool value = (edit.silent.bodies & (1u << i)) != 0;
            if (ImGui::Checkbox(astra::aim::kBodyNames[i], &value)) {
                if (value) edit.silent.bodies |= 1u << i;
                else edit.silent.bodies &= ~(1u << i);
                dirty = true;
            }
        }
        if (!edit.silent.bodies) ImGui::TextDisabled("Select at least one body region.");
    }
    if (ImGui::CollapsingHeader("Status", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int state = astra::aim::g_silentState.load();
        ImGui::Text("Silent: %s", astra::aim::stateText(state));
        ImGui::Text("Hook: %s", astra::aim::g_silentHookReady.load() ? "ready" : "waiting");
        ImGui::Text("Applications: %llu", (unsigned long long)astra::aim::g_silentApplications.load());
        ImGui::Text("Commands: %llu (rejected %llu)",
                    (unsigned long long)astra::aim::debug::silentCommands.load(),
                    (unsigned long long)astra::aim::debug::silentCommandRejected.load());
        ImGui::TextDisabled("Last reason: %s", astra::aim::debug::reason(astra::aim::debug::silentReason.load()));
    }
    ImGui::Separator();
    if (ImGui::Button("Save silent diagnostics", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        silent_diag::save();
    ImGui::TextWrapped("%s", silent_diag::status);
}

struct AntiAimState {
    bool enabled = false;
    int pitch = 0, yaw = 0;
    bool random_pitch = false, jitter = false;
    float jitter_range = 35.f, jitter_speed = 1.f;
    bool spin = false;
    float spin_speed = 8.f;
    int spin_dir = 0;
    bool fakelag = false;
    float fakelag_ticks = 12.f;
    bool fakelag_var = false;
    float fakelag_var_range = 4.f;
    bool lby = false, slide = false, edge = false, micro_move = false;
    float edge_range = 48.f, micro_move_range = 6.f;
    int on_shot_mode = 0;
    float manual_angle = 90.f;
    bool manual_left = false, manual_right = false;
};
static AntiAimState g_aa;

static void antiAimLeft() {
    ImGui::Checkbox("Anti-Aim##aa", &g_aa.enabled);
    if (ImGui::CollapsingHeader("Pitch", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Mode##aa-pitch", &g_aa.pitch, "Off\0Down\0Up\0Zero\0Random\0");
        ImGui::Checkbox("Random pitch##aa", &g_aa.random_pitch);
    }
    if (ImGui::CollapsingHeader("Yaw", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Mode##aa-yaw", &g_aa.yaw,
                     "Off\0Backward\0Sideways L\0Sideways R\0Spin\0Jitter\0Manual\0At target\0");
        if (g_aa.yaw == 4) {
            ImGui::Checkbox("Spin##aa", &g_aa.spin);
            ImGui::SliderFloat("Spin speed##aa", &g_aa.spin_speed, 1.f, 30.f, "%.0f deg/tick");
            ImGui::Combo("Direction##aa", &g_aa.spin_dir, "CW\0CCW\0Random\0");
        }
        if (g_aa.yaw == 5) {
            ImGui::Checkbox("Jitter##aa", &g_aa.jitter);
            ImGui::SliderFloat("Jitter range##aa", &g_aa.jitter_range, 5.f, 180.f, "%.0f deg");
            ImGui::SliderFloat("Jitter speed##aa", &g_aa.jitter_speed, .5f, 5.f, "%.1f");
        }
        if (g_aa.yaw == 6) {
            ImGui::SliderFloat("Manual angle##aa", &g_aa.manual_angle, -180.f, 180.f, "%.0f deg");
            ImGui::Checkbox("Manual left##aa", &g_aa.manual_left);
            ImGui::Checkbox("Manual right##aa", &g_aa.manual_right);
        }
    }
}

static void antiAimRight() {
    if (ImGui::CollapsingHeader("On shot", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Combo("Mode##aa-shot", &g_aa.on_shot_mode,
                     "Off\0Real on shot\0Jitter on shot\0");
    }
    if (ImGui::CollapsingHeader("Fake lag")) {
        ImGui::Checkbox("Fake lag##aa", &g_aa.fakelag);
        if (g_aa.fakelag) {
            ImGui::SliderFloat("Ticks##aa", &g_aa.fakelag_ticks, 1.f, 22.f, "%.0f");
            ImGui::Checkbox("Variance##aa", &g_aa.fakelag_var);
            if (g_aa.fakelag_var)
                ImGui::SliderFloat("Variance range##aa", &g_aa.fakelag_var_range, 1.f, 8.f, "%.0f");
        }
    }
    if (ImGui::CollapsingHeader("Extra")) {
        ImGui::Checkbox("LBY breaker##aa", &g_aa.lby);
        ImGui::Checkbox("Slide##aa", &g_aa.slide);
        ImGui::Checkbox("Edge detection##aa", &g_aa.edge);
        if (g_aa.edge) ImGui::SliderFloat("Edge range##aa", &g_aa.edge_range, 16.f, 128.f, "%.0f");
        ImGui::Checkbox("Micro movement##aa", &g_aa.micro_move);
        if (g_aa.micro_move)
            ImGui::SliderFloat("Micro range##aa", &g_aa.micro_move_range, 1.f, 20.f, "%.1f");
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
    g_aa = AntiAimState{};
}

static void configLeft() {
    ImGui::TextDisabled("Tap the watermark to open or close the menu.");
    ImGui::TextUnformatted("Menu accent");
    ImGui::SameLine();
    kazakh::ColorPicker("Menu accent", kazakh::accentColor);
    ImGui::Checkbox("Show ESP preview##cfg", &previewEnabled);
    if (ImGui::Button("unload modification", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        ImGui::OpenPopup("Unload unavailable");
    if (ImGui::BeginPopupModal("Unload unavailable", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("This build cannot safely unload the library.");
        ImGui::TextUnformatted("Active hooks and worker threads need a shutdown lifecycle.");
        ImGui::TextUnformatted("No unload was performed. Restart the game to remove it.");
        if (ImGui::Button("OK##unload")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::Separator();
    if (ImGui::Button("Reset settings", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        ImGui::OpenPopup("Reset settings?");
    if (ImGui::BeginPopupModal("Reset settings?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Restore all controls to defaults?");
        if (ImGui::Button("Reset##confirm", ImVec2(100, 0))) {
            resetAll();
            kazakh::Toast("Defaults restored", 1);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel##confirm", ImVec2(100, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void configRight() {
    ImGui::TextDisabled("HantoO / Free");
    const auto unityBase = g_unityBase.load(std::memory_order_acquire);
    ImGui::Text("Unity: %s", unityBase ? "found" : "waiting libunity.so");
    ImGui::Text("Unity base: %p", reinterpret_cast<void*>(unityBase));
    ImGui::Text("Profile: %s", astra::offsets::kProfileName);
    ImGui::Text("Binary profile: %s", !unityBase ? "waiting for Unity" :
        g_offsetProfileReady.load() ? "verified 1.1.0 ARM64" : "build ID mismatch / blocked");
    ImGui::Text("Camera cache: %s", astra::esp::g_cameraSnapshotReady.load() ? "ready" : "waiting");
    ImGui::Text("Camera captures: %llu | rejected: %llu",
        static_cast<unsigned long long>(astra::esp::g_cameraSnapshotCaptures.load()),
        static_cast<unsigned long long>(astra::esp::g_cameraSnapshotFailures.load()));
    ImGui::Text("ESP: %s", astra::esp::statusText());
    ImGui::Text("Players: %d | projected: %d", astra::esp::playerCount(), astra::esp::projectedCount());
    ImGui::Text("Aim hook: %s", astra::aim::g_normalHookReady.load() ? "ready" : "waiting");
    ImGui::Text("ESP hook: %s", astra::esp::g_playerUpdateHookReady.load() ? "ready" : "waiting");
    ImGui::Text("Hitlog hook: %d", astra_hitlog::state.load());
    ImGui::Separator();
    ImGui::TextDisabled("Silent / 1.1.0 ARM64");
    ImGui::Text("State: %s", astra::aim::stateText(astra::aim::g_silentState.load()));
    ImGui::Text("Shot hook: %s", astra::aim::debug::installReason(astra::aim::debug::silentInstallReason.load()));
    ImGui::Text("Command hook: %s", astra::aim::input_init::reasonText(astra::aim::g_inputInstallReason.load()));
    ImGui::Text("Look hook: %s", astra::aim::debug::commandLookHookReady.load() ? "ready" : "waiting");
    ImGui::Text("Visibility: %s", astra::aim::g_visibilityReady.load() ? "ready" : "waiting");
    ImGui::Text("Last result: %s", astra::aim::debug::reason(astra::aim::debug::silentReason.load()));
    ImGui::Text("Calls: %llu | rays: %llu | commands: %llu",
        (unsigned long long)astra::aim::debug::silentCalls.load(),
        (unsigned long long)astra::aim::g_silentApplications.load(),
        (unsigned long long)astra::aim::debug::silentCommands.load());
    ImGui::Text("Targets: %d | hitboxes: %d | profile failures: %llu",
        astra::aim::debug::freshActors.load(), astra::aim::debug::freshVolumes.load(),
        (unsigned long long)astra::aim::debug::validationFailures.load());
    ImGui::Text("Sync: %llu | missing/stale: %llu | stale look: %llu",
        (unsigned long long)astra::aim::debug::silentPlanMatched.load(),
        (unsigned long long)astra::aim::debug::silentPlanRejected.load(),
        (unsigned long long)astra::aim::debug::silentStaleLookRejected.load());
    ImGui::Text("Origin mismatch: %llu | moved target: %llu | look write failure: %llu",
        (unsigned long long)astra::aim::debug::silentOriginMismatch.load(),
        (unsigned long long)astra::aim::debug::silentPlanTargetRejected.load(),
        (unsigned long long)astra::aim::debug::silentLookReadbackFailed.load());
    if (ImGui::Button("Save silent diagnostics##config")) silent_diag::save();
    ImGui::TextWrapped("%s", silent_diag::status);
    ImGui::Text("Frame: %.1f ms", 1000.0f / std::max(1.0f, ImGui::GetIO().Framerate));
    ImGui::Separator();
    ImGui::TextDisabled("GUI: KazakhDLC shell");
    ImGui::TextDisabled("Layout: two columns, animated");
}

static void astraPage(int tab, int column) {
    switch (tab) {
        case 0: column == 1 ? visualsLeft() : visualsRight(); break;
        case 1: column == 1 ? silentLeft() : silentRight(); break;
        default: column == 1 ? configLeft() : configRight(); break;
    }
}

// ── KazakhDLC shell ────────────────────────────────────────────────────────
static void menuDragHandle(bool clickOpens) {
    if (ImGui::IsItemActivated()) {
        g_dragDistance = 0.0f;
        g_dragMoved = false;
    } else if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const ImVec2 d = ImGui::GetIO().MouseDelta;
        g_dragDistance += std::fabs(d.x) + std::fabs(d.y);
        if (g_dragDistance > 4.0f) g_dragMoved = true;
        g_menuPos.x += d.x;
        g_menuPos.y += d.y;
        const ImVec2 screen = ImGui::GetIO().DisplaySize;
        const ImVec2 cur = ImGui::GetWindowSize();
        g_menuPos.x = kazakh::clampv(g_menuPos.x, 0.0f, std::max(0.0f, screen.x - cur.x));
        g_menuPos.y = kazakh::clampv(g_menuPos.y, 0.0f, std::max(0.0f, screen.y - cur.y));
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    } else if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    if (ImGui::IsItemDeactivated() && clickOpens && !g_dragMoved)
        setMenuOpen(true);
}

static void drawKazakhMenu() {
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 screen = io.DisplaySize;

    // KazakhDLC re-applies the accent every frame so the color picker is live.
    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_CheckMark] = kazakh::accentColor;
    style->Colors[ImGuiCol_SliderGrab] = kazakh::accentColor;
    style->Colors[ImGuiCol_SliderGrabActive] = kazakh::accentColor;
    style->Colors[ImGuiCol_SeparatorHovered] = kazakh::accentColor;
    style->Colors[ImGuiCol_ResizeGripHovered] = kazakh::accentColor;
    style->Colors[ImGuiCol_TabHovered] = kazakh::accentColor;
    const bool closed = !menuOpen();

    g_lerped = kazakh::lerp(g_lerped, closed ? kClosedS : kOpenedS,
                            0.2f * io.DeltaTime * 120.0f);
    const float elo = kazakh::clampv((g_lerped - 75.0f) / 375.0f, 0.001f, 1.0f);

    // Watermark mode has no title bar: when closed, only the watermark remains.
    if (closed && g_openStyle == 1) return;

    ImGui::GetStyle().WindowMinSize = ImVec2(600, g_lerped) * kazakh::kDpi;

    const float fullW = 600.0f * kazakh::kDpi;
    const float fullH = kOpenedS * kazakh::kDpi;
    if (g_menuPos.x < 0.0f || screen.x != g_menuScreen.x || screen.y != g_menuScreen.y) {
        // KazakhDLC first-use position, clamped to the current display.
        g_menuPos = ImVec2(400.0f, 400.0f);
        g_menuScreen = screen;
    }
    const float curH = g_lerped * kazakh::kDpi;
    g_menuPos.x = std::min(std::max(g_menuPos.x, 0.0f), std::max(0.0f, screen.x - fullW));
    g_menuPos.y = std::min(std::max(g_menuPos.y, 0.0f), std::max(0.0f, screen.y - curH));

    ImGui::SetNextWindowPos(g_menuPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(fullW, g_lerped * kazakh::kDpi), ImGuiCond_FirstUseEver);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("##menuuuuuuu", &g_boels, flags);

    const float menuWidth = ImGui::GetWindowWidth();
    const float windowHeight = ImGui::GetWindowHeight();
    if (std::fabs(windowHeight - g_lerped * kazakh::kDpi) > 0.5f)
        ImGui::SetWindowSize(ImVec2(menuWidth, g_lerped * kazakh::kDpi));

    // Header / collapsed-bar drag. Movement is applied on the next frame via
    // SetNextWindowPos so nothing already drawn can shear.
    if (closed) {
        ImGui::SetCursorPos(ImVec2(6, 6) * kazakh::kDpi);
        ImGui::InvisibleButton("##kz-bar-drag",
                               ImVec2(menuWidth - 12 * kazakh::kDpi,
                                      windowHeight - 12 * kazakh::kDpi));
        menuDragHandle(true);
    } else {
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##kz-header-drag",
                               ImVec2(menuWidth, 55 * kazakh::kDpi));
        menuDragHandle(false);
    }

    if (!closed && g_openStyle == 0) {
        ImGui::SetCursorPos(ImVec2(menuWidth / 2, windowHeight) - ImVec2(25, 40) * kazakh::kDpi);
        if (ImGui::Button("/\\", ImVec2(50, 30) * kazakh::kDpi))
            setMenuOpen(false);
    }

    auto* wd = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();

    wd->AddRectFilled(pos, ImVec2(pos.x + menuWidth, pos.y + windowHeight), ImColor(60, 60, 60, 180));
    wd->AddRect(pos, ImVec2(pos.x + menuWidth, pos.y + windowHeight), ImColor(10, 10, 10, 255), 0, 0, 2);
    wd->AddRect(pos + ImVec2(1, 1) * kazakh::kDpi,
                ImVec2(pos.x + menuWidth, pos.y + windowHeight) - ImVec2(1, 1) * kazakh::kDpi,
                ImColor(80, 80, 80, 180), 0, 0, 2 * kazakh::kDpi);
    wd->AddRect(pos + ImVec2(6, 6) * kazakh::kDpi - ImVec2(1, 1) * kazakh::kDpi,
                pos + size - ImVec2(6, 6) * kazakh::kDpi + ImVec2(1, 1) * kazakh::kDpi,
                ImColor(80, 80, 80, 180), 0, 0, 2 * kazakh::kDpi);
    wd->AddRectFilled(pos + ImVec2(6, 6) * kazakh::kDpi,
                      pos + size - ImVec2(6, 6) * kazakh::kDpi, ImColor(15, 15, 15));
    wd->AddRectFilled(pos + ImVec2(30, 45) * kazakh::kDpi,
                      pos + ImVec2(size.x - 30 * kazakh::kDpi, 50 * kazakh::kDpi),
                      ImColor(kazakh::accentColor.x, kazakh::accentColor.y,
                              kazakh::accentColor.z, kazakh::accentColor.w));

    const char* title = "HantoO";
    const float titlePx = 26 * kazakh::kDpi;
    const float titleW = ImGui::CalcTextSize(title).x * (titlePx / ImGui::GetFontSize());
    wd->AddText(ImGui::GetFont(), titlePx,
                ImVec2(pos.x + (size.x - titleW) * .5f,
                       pos.y + 25 * kazakh::kDpi -
                           (ImGui::CalcTextSize(title).y / (22 * kazakh::kDpi) * titlePx) / 2),
                ImColor(255, 255, 255), title);

    const float step = (menuWidth - 60 * kazakh::kDpi) / static_cast<float>(kTabCount);
    for (int i = 0; i < kTabCount; ++i) {
        ImGui::SetCursorPos(ImVec2(30 * kazakh::kDpi + step * i + (i ? 5 * kazakh::kDpi : 0.0f),
                                   60 * kazakh::kDpi));
        if (ImGui::Button(kTabs[i], ImVec2(i ? step - 5 * kazakh::kDpi : step,
                                           30 * kazakh::kDpi * elo)))
            g_tab = i;
    }

    const float panelTop = pos.y + 100 * kazakh::kDpi;
    const float panelBottom = pos.y + kazakh::clampv(windowHeight - 50 * kazakh::kDpi,
                                                     100 * kazakh::kDpi, windowHeight - 50 * kazakh::kDpi);
    wd->AddRect(ImVec2(pos.x + 30 * kazakh::kDpi, panelTop),
                ImVec2(pos.x + menuWidth / 2 - 5 * kazakh::kDpi, panelBottom),
                ImColor(0, 0, 0), 0, 0, 4);
    wd->AddRectFilled(ImVec2(pos.x + 30 * kazakh::kDpi, panelTop),
                      ImVec2(pos.x + menuWidth / 2 - 5 * kazakh::kDpi, panelBottom),
                      ImColor(17, 17, 17));
    wd->AddRect(ImVec2(pos.x + 30 * kazakh::kDpi, panelTop),
                ImVec2(pos.x + menuWidth / 2 - 5 * kazakh::kDpi, panelBottom),
                ImColor(60, 60, 60), 0, 0, 2);

    wd->AddRect(ImVec2(pos.x + menuWidth / 2 + 5 * kazakh::kDpi, panelTop),
                ImVec2(pos.x + menuWidth - 30 * kazakh::kDpi, panelBottom),
                ImColor(0, 0, 0), 0, 0, 4);
    wd->AddRectFilled(ImVec2(pos.x + menuWidth / 2 + 5 * kazakh::kDpi, panelTop),
                      ImVec2(pos.x + menuWidth - 30 * kazakh::kDpi, panelBottom),
                      ImColor(17, 17, 17));
    wd->AddRect(ImVec2(pos.x + menuWidth / 2 + 5 * kazakh::kDpi, panelTop),
                ImVec2(pos.x + menuWidth - 30 * kazakh::kDpi, panelBottom),
                ImColor(60, 60, 60), 0, 0, 2);

    if (windowHeight > 150 * kazakh::kDpi + 8.0f) {
        const float listH = kazakh::clampv(windowHeight - 150 * kazakh::kDpi,
                                           1.0f, windowHeight - 150 * kazakh::kDpi);
        const float listW = menuWidth / 2 - 35 * kazakh::kDpi;

        ImGui::SetCursorPos(ImVec2(30, 100) * kazakh::kDpi);
        if (ImGui::BeginListBox("##wf", ImVec2(listW, listH))) {
            astraPage(g_tab, 1);
            ImGui::EndListBox();
        }

        ImGui::SetCursorPos(ImVec2(menuWidth / 2 + 5 * kazakh::kDpi, 100 * kazakh::kDpi));
        if (ImGui::BeginListBox("##ws", ImVec2(listW, listH))) {
            astraPage(g_tab, 2);
            ImGui::EndListBox();
        }
    }

    if (g_openStyle == 0) {
        const char* arrow = closed ? "\\/" : "/\\";
        const ImVec2 arrowSize = ImGui::CalcTextSize(arrow);
        wd->AddText(ImGui::GetFont(), 26 * kazakh::kDpi,
                    ImVec2(pos.x + menuWidth / 2 - (arrowSize.x / (22 * kazakh::kDpi) * (26 * kazakh::kDpi)) / 2,
                           pos.y + windowHeight - 30 * kazakh::kDpi -
                               (arrowSize.y / (22 * kazakh::kDpi) * (26 * kazakh::kDpi)) / 2),
                    ImColor(255, 255, 255), arrow);
    }

    RectangleShadowSettings shadowSettings;
    shadowSettings.rectPos = ImVec2(0, 0);
    shadowSettings.rectSize = ImGui::GetWindowSize();
    shadowSettings.sigma = 20;
    shadowSettings.rings = 4;
    shadowSettings.spacingBetweenRings = 7;
    shadowSettings.samplesPerCornerSide = 2;
    shadowSettings.spacingBetweenSamples = 20;
    shadowSettings.shadowColor = ImColor(0, 0, 0, 255);
    shadowSettings.shadowSize = ImVec2(0, 0);
    drawRectangleShadowVerticesAdaptive(shadowSettings);

    ImGui::End();
}

// ── watermark ───────────────────────────────────────────────────────────────
static void drawWatermark() {
    constexpr bool watermarkMode = true;

    std::string t1 = ("HantoO ");
    std::string t2 = ("| internal ");
    std::string t3 = ("| ");
    time_t now = time(0);
    tm* local_time = localtime(&now);
    std::string t5 = std::to_string(local_time->tm_hour);
    std::string t6 = ":";
    std::string t7 = std::to_string(local_time->tm_min);
    std::string water_text = t1 + t2 + t3 + t5 + t6 + t7;
    auto text_size = ImGui::CalcTextSize(water_text.c_str());
    auto padding = ImVec2(24, 24);
    auto textpadding = 42;
    auto size = ImVec2(text_size.x + padding.x, text_size.y + padding.y);
    auto pos = ImVec2(ImGui::GetIO().DisplaySize.x - size.x - padding.x * 2 - textpadding + padding.x / 2,
                      padding.y * 2);

    // Watermark mode: the box itself is the only menu open/close control.
    if (watermarkMode) {
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(size.x + textpadding, size.y), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoNav;
        if (ImGui::Begin("##astra-watermark-toggle", nullptr, flags)) {
            ImGui::SetCursorPos(ImVec2(0, 0));
            if (ImGui::InvisibleButton("##wm-toggle", ImGui::GetContentRegionAvail()))
                setMenuOpen(!menuOpen());
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    auto* draw = ImGui::GetForegroundDrawList();
    auto textoffset = 1.f;

    draw->AddRectFilled(pos, {pos.x + size.x + textpadding, pos.y + size.y}, ImColor(0, 0, 0, 200), 4);
    draw->AddRect(pos, {pos.x + size.x + textpadding, pos.y + size.y}, ImColor(0, 0, 0), 3, 0, 7);
    draw->AddRect(pos, {pos.x + size.x + textpadding, pos.y + size.y}, ImColor(50, 50, 50), 3, 0, 2.5f);
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2, pos.y + padding.y / 2 - textoffset},
                  ImColor(255, 255, 255), t1.c_str());
    int offset = ImGui::CalcTextSize(t1.c_str()).x;
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2 + offset, pos.y + padding.y / 2 - textoffset},
                  ImColor(128, 128, 128), t2.c_str());
    offset += ImGui::CalcTextSize(t2.c_str()).x;
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2 + offset, pos.y + padding.y / 2 - textoffset},
                  ImColor(255, 255, 255), t3.c_str());
    offset += ImGui::CalcTextSize(t3.c_str()).x;
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2 + offset, pos.y + padding.y / 2 - textoffset},
                  ImColor(255, 255, 255), t5.c_str());
    offset += ImGui::CalcTextSize(t5.c_str()).x;
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2 + offset, pos.y + padding.y / 2 - textoffset},
                  ImColor(255, 255, 255), t6.c_str());
    offset += ImGui::CalcTextSize(t6.c_str()).x;
    draw->AddText({pos.x + padding.x / 2 + textpadding / 2 + offset, pos.y + padding.y / 2 - textoffset},
                  ImColor(255, 255, 255), t7.c_str());
    draw->AddRectFilled({pos.x + size.x + textpadding - 12 - 2, pos.y + 12},
                        {pos.x + size.x + textpadding - 12 - 4 - 2, pos.y - 12 + size.y},
                        ImColor(255, 255, 255));
    draw->AddRectFilled({pos.x + 12 + 2, pos.y + 12},
                        {pos.x + 12 + 4 + 2, pos.y - 12 + size.y},
                        ImColor(255, 255, 255));
}

// ── commit ──────────────────────────────────────────────────────────────────
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
    c.silent = edit.silent;
    c.silent.rcs = false; c.silent.noRecoil = false;
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
    renderFov(edit.normal, ImGui::ColorConvertFloat4ToU32(kazakh::accentColor));
    renderFov(edit.silent, IM_COL32(255, 92, 92, 220));
    drawKazakhMenu();
    drawWatermark();
    kazakh::RenderToast();

    commit();
    astra::aim::g_menuOpen.store(menuOpen(), std::memory_order_release);
}
