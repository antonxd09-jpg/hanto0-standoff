from pathlib import Path

path = Path('/home/leftcode/Projects/LemmingRMT/jni/main.cpp')
s = path.read_text('utf-8')

def replace(old: str, new: str, label: str) -> None:
    global s
    count = s.count(old)
    if count != 1:
        raise SystemExit(f'{label}: expected 1 match, got {count}')
    s = s.replace(old, new, 1)

replace(
'''#include <EGL/egl.h>
#include <GLES3/gl3.h>''',
'''#include <EGL/egl.h>
#include <GLES3/gl3.h>''',
'includes-egl')

replace(
'''#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <csignal>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>''',
'''#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>''',
'includes-std')

replace(
'''#define LOG_TAG "LemmingRMT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)''',
'''#define LOG_TAG "LemmingRMT"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)''',
'logging-macros')

replace(
'''using SwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
using InputGetEventFn = int32_t (*)(AInputQueue*, AInputEvent**);
using NativeInjectEventFn = jboolean (*)(JNIEnv*, jobject, jobject, jint);
using GetCreatedJavaVMsFn = jint (*)(JavaVM**, jsize, jsize*);

SwapBuffersFn g_realSwapBuffers = nullptr;
InputGetEventFn g_realInputGetEvent = nullptr;
NativeInjectEventFn g_realNativeInjectEvent = nullptr;''',
'''using SwapBuffersFn = EGLBoolean (*)(EGLDisplay, EGLSurface);
using SwapBuffersWithDamageFn = EGLBoolean (*)(EGLDisplay, EGLSurface, const EGLint*, EGLint);
using InputGetEventFn = int32_t (*)(AInputQueue*, AInputEvent**);
using NativeInjectEventFn = jboolean (*)(JNIEnv*, jobject, jobject, jint);
using GetCreatedJavaVMsFn = jint (*)(JavaVM**, jsize, jsize*);

SwapBuffersFn g_realSwapBuffers = nullptr;
SwapBuffersWithDamageFn g_realSwapBuffersWithDamageKHR = nullptr;
SwapBuffersWithDamageFn g_realSwapBuffersWithDamageEXT = nullptr;
InputGetEventFn g_realInputGetEvent = nullptr;
NativeInjectEventFn g_realNativeInjectEvent = nullptr;''',
'function-types')

replace(
'''std::atomic<int> g_swapSlots{0};
std::atomic<int> g_inputSlots{0};
std::atomic<bool> g_javaInputHookReady{false};
std::atomic<uint64_t> g_javaInputEvents{0};
std::atomic<uintptr_t> g_unityBase{0};
std::atomic<bool> g_offsetProfileReady{false};
volatile sig_atomic_t g_toggleMenuSignal = 0;

EGLContext g_imguiContext = EGL_NO_CONTEXT;
bool g_imguiBackendReady = false;
std::chrono::steady_clock::time_point g_lastImGuiFrame{};
std::atomic<bool> g_espBoxesEnabled{false};
std::atomic<bool> g_espResolverReady{false};''',
'''std::atomic<int> g_swapSlots{0};
std::atomic<int> g_swapDamageSlots{0};
std::atomic<int> g_inputSlots{0};
std::atomic<bool> g_javaInputHookReady{false};
std::atomic<uint64_t> g_javaInputEvents{0};
std::atomic<uint64_t> g_presentFrames{0};
std::atomic<uint64_t> g_renderedFrames{0};
std::atomic<uint64_t> g_renderFailures{0};
std::atomic<int> g_glClientVersion{0};
std::atomic<uintptr_t> g_lastEglContext{0};
std::atomic<uintptr_t> g_lastEglSurface{0};
std::atomic<uintptr_t> g_unityBase{0};
std::atomic<bool> g_offsetProfileReady{false};
volatile sig_atomic_t g_toggleMenuSignal = 0;
volatile sig_atomic_t g_dumpDiagnosticsSignal = 0;

EGLContext g_imguiContext = EGL_NO_CONTEXT;
EGLDisplay g_imguiDisplay = EGL_NO_DISPLAY;
EGLSurface g_imguiSurface = EGL_NO_SURFACE;
bool g_imguiBackendReady = false;
std::chrono::steady_clock::time_point g_lastImGuiFrame{};
std::atomic<bool> g_espBoxesEnabled{true};
std::atomic<bool> g_espResolverReady{false};''',
'globals')

old_ensure = '''bool ensureImGui() {
    const EGLContext current = eglGetCurrentContext();
    if (current == EGL_NO_CONTEXT) return false;
    if (g_imguiBackendReady && current == g_imguiContext && ImGui::GetCurrentContext()) return true;
    if (ImGui::GetCurrentContext()) {
        if (g_imguiBackendReady) ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendPlatformName = "LemmingRMT_UnityJNI";
    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
        static_cast<int>(lemming::fonts::kMozillaTextSize), 21.0f, &cfg,
        io.Fonts->GetGlyphRangesCyrillic());
    if (!font) io.Fonts->AddFontDefault();
    configureImGuiStyle();
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        LOGE("ImGui OpenGL3 initialization failed");
        ImGui::DestroyContext();
        g_imguiBackendReady = false;
        return false;
    }
    g_imguiContext = current;
    g_imguiBackendReady = true;
    g_lastImGuiFrame = std::chrono::steady_clock::now();
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOGI("ImGui ready: Mozilla Text, version=%s renderer=%s",
         version ? version : "unknown", renderer ? renderer : "unknown");
    return true;
}'''
new_ensure = '''bool ensureImGui(EGLDisplay display, EGLSurface surface) {
    const EGLContext current = eglGetCurrentContext();
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || current == EGL_NO_CONTEXT) {
        return false;
    }
    g_lastEglContext.store(reinterpret_cast<uintptr_t>(current), std::memory_order_relaxed);
    g_lastEglSurface.store(reinterpret_cast<uintptr_t>(surface), std::memory_order_relaxed);

    if (g_imguiBackendReady && current == g_imguiContext && display == g_imguiDisplay &&
        surface == g_imguiSurface && ImGui::GetCurrentContext()) {
        return true;
    }

    EGLint clientVersion = 0;
    if (eglQueryContext(display, current, EGL_CONTEXT_CLIENT_VERSION, &clientVersion) != EGL_TRUE) {
        clientVersion = 0;
    }
    g_glClientVersion.store(clientVersion, std::memory_order_relaxed);
    if (clientVersion < 3) {
        static EGLContext lastRejected = EGL_NO_CONTEXT;
        if (lastRejected != current) {
            lastRejected = current;
            LOGE("renderer rejected EGL context=%p: GLES client version=%d; this build requires GLES3",
                 current, clientVersion);
        }
        return false;
    }

    if (ImGui::GetCurrentContext()) {
        if (g_imguiBackendReady && current == g_imguiContext) ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }
    g_imguiBackendReady = false;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.BackendPlatformName = "LemmingRMT_EGL";
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImFontConfig cfg{};
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = false;
    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        const_cast<std::uint8_t*>(lemming::fonts::kMozillaText),
        static_cast<int>(lemming::fonts::kMozillaTextSize), 21.0f, &cfg,
        io.Fonts->GetGlyphRangesCyrillic());
    if (!font) {
        LOGW("Mozilla Text font failed; using default ImGui font");
        io.Fonts->AddFontDefault();
    }
    configureImGuiStyle();

    if (!ImGui_ImplOpenGL3_Init("#version 300 es") || !ImGui_ImplOpenGL3_CreateDeviceObjects()) {
        LOGE("ImGui GLES3 backend/device initialization failed: context=%p", current);
        if (ImGui::GetCurrentContext() && ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplOpenGL3_Shutdown();
        }
        ImGui::DestroyContext();
        return false;
    }

    g_imguiContext = current;
    g_imguiDisplay = display;
    g_imguiSurface = surface;
    g_imguiBackendReady = true;
    g_lastImGuiFrame = std::chrono::steady_clock::now();

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glsl = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    LOGI("renderer ready: context=%p GLES=%d GL=%s GLSL=%s GPU=%s",
         current, clientVersion, version ? version : "unknown", glsl ? glsl : "unknown",
         renderer ? renderer : "unknown");
    return true;
}'''
replace(old_ensure, new_ensure, 'ensure-imgui')

old_render = '''void renderEspBoxes() {
    if (!g_espBoxesEnabled.load(std::memory_order_relaxed) || !g_espResolverReady.load(std::memory_order_relaxed)) return;
    std::vector<EspScreenBox> boxes;
    { std::lock_guard<std::mutex> lock(g_espMutex); boxes=g_espScreenBoxes; }
    ImDrawList* d=ImGui::GetBackgroundDrawList();
    for (const auto& b: boxes) {
        const ImU32 col=b.enemy?IM_COL32(255,84,102,245):IM_COL32(82,182,255,235);
        d->AddRect(ImVec2(b.left,b.top),ImVec2(b.right,b.bottom),IM_COL32(0,0,0,190),3,0,4);
        d->AddRect(ImVec2(b.left,b.top),ImVec2(b.right,b.bottom),col,3,0,1.8f);
    }
}

void renderMinimalMenu() {
    renderWatermark(); renderEspBoxes();
    if (!g_menuOpen.load(std::memory_order_relaxed)) return;
    ImGui::SetNextWindowPos(ImVec2(24,72),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430,264),ImGuiCond_Always);
    ImGui::Begin("##lemming_menu",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);
    ImDrawList* d=ImGui::GetWindowDrawList();
    const ImVec2 wp=ImGui::GetWindowPos(), ws=ImGui::GetWindowSize();
    d->AddRectFilled(wp,ImVec2(wp.x+4,wp.y+ws.y),IM_COL32(74,164,255,255),15,ImDrawFlags_RoundCornersLeft);
    ImGui::SetCursorPos(ImVec2(22,18)); ImGui::TextUnformatted("LemmingRMT");
    ImGui::SameLine(); ImGui::SetCursorPosY(21); ImGui::TextColored(ImVec4(.43f,.49f,.59f,1),"internal overlay");
    ImGui::SetCursorPos(ImVec2(ws.x-50,12));
    if(ImGui::Button("x",ImVec2(32,32))) g_menuOpen.store(false,std::memory_order_relaxed);
    ImGui::SetCursorPos(ImVec2(22,62));
    const ImVec2 sm=ImGui::GetCursorScreenPos(), sx(sm.x+ws.x-44,sm.y+48);
    d->AddRectFilled(sm,sx,IM_COL32(20,27,38,255),11);
    d->AddCircleFilled(ImVec2(sm.x+18,sm.y+24),5,IM_COL32(77,221,142,255));
    d->AddText(ImVec2(sm.x+32,sm.y+8),IM_COL32(232,238,247,255),"Overlay online");
    d->AddText(ImVec2(sm.x+32,sm.y+27),IM_COL32(119,132,153,255),"Unity JNI input  •  OpenGL ES 3");
    ImGui::SetCursorPos(ImVec2(22,132));
    bool esp=g_espBoxesEnabled.load(std::memory_order_relaxed);
    const bool ready=g_espResolverReady.load(std::memory_order_relaxed);
    ImGui::BeginDisabled(!ready);
    if(toggleSwitch("ESP-боксы",&esp)) g_espBoxesEnabled.store(esp,std::memory_order_relaxed);
    ImGui::EndDisabled();
    ImGui::SetCursorPos(ImVec2(78,164));
    const char* espStatus=lemming::esp::statusText();
    if(ready) ImGui::TextColored(ImVec4(.42f,.84f,.61f,1),"%s  •  игроков: %d",espStatus,lemming::esp::playerCount());
    else ImGui::TextColored(ImVec4(.95f,.55f,.34f,1),"%s",espStatus);
    ImGui::SetCursorPos(ImVec2(22,ws.y-39)); ImGui::Separator(); ImGui::SetCursorPosY(ws.y-27);
    ImGui::TextColored(ImVec4(.42f,.47f,.56f,1),"Mozilla Text  •  Dear ImGui  •  %s",kArchitectureLabel);
    ImGui::End();
}

void drawOverlay(int width,int height) {
    if(width<160||height<120||!ensureImGui()) return;
    std::vector<EspScreenBox> nextEspBoxes;
    {
        std::lock_guard<std::mutex> lock(g_espMutex);
        nextEspBoxes=g_espScreenBoxes;
    }
    const bool espReady=lemming::esp::update(g_unityBase.load(std::memory_order_acquire),width,height,nextEspBoxes);
    g_espResolverReady.store(espReady&&lemming::esp::ready(),std::memory_order_release);
    if(espReady){
        std::lock_guard<std::mutex> lock(g_espMutex);
        g_espScreenBoxes.swap(nextEspBoxes);
    }
    ImGuiIO& io=ImGui::GetIO(); io.DisplaySize=ImVec2((float)width,(float)height);
    const auto now=std::chrono::steady_clock::now();
    const float dt=std::chrono::duration<float>(now-g_lastImGuiFrame).count();
    io.DeltaTime=(dt>0&&dt<.25f)?dt:(1.0f/60.0f); g_lastImGuiFrame=now;
    std::vector<PointerEvent> events;
    { std::lock_guard<std::mutex> lock(g_pointerMutex); events.swap(g_pointerEvents); }
    ImGui_ImplOpenGL3_NewFrame();
    for(const auto& e:events){ io.AddMousePosEvent(e.x,e.y); if(e.kind==1)io.AddMouseButtonEvent(0,true); else if(e.kind==2)io.AddMouseButtonEvent(0,false); }
    ImGui::NewFrame(); renderMinimalMenu(); ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}'''
new_render = '''void renderEspBoxes() {
    if (!g_espBoxesEnabled.load(std::memory_order_relaxed)) return;
    std::vector<EspScreenBox> boxes;
    { std::lock_guard<std::mutex> lock(g_espMutex); boxes = g_espScreenBoxes; }
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    for (const auto& box : boxes) {
        const ImU32 color = box.enemy ? IM_COL32(255, 82, 98, 245) : IM_COL32(70, 184, 255, 245);
        const ImVec2 topLeft(box.left, box.top);
        const ImVec2 bottomRight(box.right, box.bottom);
        draw->AddRect(topLeft, bottomRight, IM_COL32(0, 0, 0, 210), 3.0f, 0, 4.0f);
        draw->AddRect(topLeft, bottomRight, color, 3.0f, 0, 1.8f);
        draw->AddText(ImVec2(box.left + 3.0f, box.top - 21.0f), color, "PLAYER");
    }
}

void renderMinimalMenu() {
    renderEspBoxes();
    renderWatermark();
    if (!g_menuOpen.load(std::memory_order_relaxed)) return;

    ImGuiIO& io = ImGui::GetIO();
    const float scale = std::clamp(io.DisplaySize.y / 1080.0f, 0.82f, 1.28f);
    const float width = std::min(500.0f * scale, std::max(320.0f, io.DisplaySize.x - 24.0f));
    const float height = 330.0f * scale;
    ImGui::SetNextWindowPos(ImVec2(18.0f, 70.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.97f);

    bool open = true;
    if (ImGui::Begin("LemmingRMT##main", &open,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(0.35f, 0.68f, 1.0f, 1.0f), "ARM64 1.0.0 / read-only base ESP");
        ImGui::TextDisabled("Profile: %s", lemming::offsets::kProfileName);
        ImGui::Separator();

        bool esp = g_espBoxesEnabled.load(std::memory_order_relaxed);
        if (ImGui::Checkbox("Basic ESP boxes", &esp)) {
            g_espBoxesEnabled.store(esp, std::memory_order_relaxed);
            LOGI("basic ESP toggled: enabled=%d", esp ? 1 : 0);
        }
        ImGui::TextWrapped("Status: %s", lemming::esp::statusText());
        ImGui::Text("Players: %d", lemming::esp::playerCount());
        ImGui::TextDisabled("Team/HP labels remain disabled until FieldInfo validation.");

        ImGui::Separator();
        ImGui::Text("Renderer diagnostics");
        ImGui::Text("GLES: %d | present: %llu | rendered: %llu | failures: %llu",
                    g_glClientVersion.load(std::memory_order_relaxed),
                    static_cast<unsigned long long>(g_presentFrames.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(g_renderedFrames.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(g_renderFailures.load(std::memory_order_relaxed)));
        ImGui::Text("Hooks: swap=%d damage=%d input=%d",
                    g_swapSlots.load(std::memory_order_relaxed),
                    g_swapDamageSlots.load(std::memory_order_relaxed),
                    g_inputSlots.load(std::memory_order_relaxed));
        ImGui::TextDisabled("ADB: SIGUSR1 menu toggle, SIGUSR2 diagnostic dump");

        ImGui::Separator();
        if (ImGui::Button("Dump diagnostics to logcat")) {
            lemming::esp::logDiagnostics("menu-button");
            LOGI("renderer diagnostics requested from menu");
        }
        ImGui::SameLine();
        if (ImGui::Button("Close menu")) open = false;
        ImGui::TextDisabled("Mozilla Text / Dear ImGui / %s", kArchitectureLabel);
    }
    ImGui::End();
    if (!open) g_menuOpen.store(false, std::memory_order_relaxed);
}

bool drawOverlay(EGLDisplay display, EGLSurface surface, int width, int height) {
    if (width < 160 || height < 120 || !ensureImGui(display, surface)) {
        g_renderFailures.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    std::vector<EspScreenBox> nextEspBoxes;
    { std::lock_guard<std::mutex> lock(g_espMutex); nextEspBoxes = g_espScreenBoxes; }
    const bool espUpdated = lemming::esp::update(
        g_unityBase.load(std::memory_order_acquire), width, height, nextEspBoxes);
    g_espResolverReady.store(espUpdated && lemming::esp::ready(), std::memory_order_release);
    if (espUpdated) {
        std::lock_guard<std::mutex> lock(g_espMutex);
        g_espScreenBoxes.swap(nextEspBoxes);
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - g_lastImGuiFrame).count();
    io.DeltaTime = (dt > 0.0f && dt < 0.25f) ? dt : (1.0f / 60.0f);
    g_lastImGuiFrame = now;

    std::vector<PointerEvent> events;
    { std::lock_guard<std::mutex> lock(g_pointerMutex); events.swap(g_pointerEvents); }

    ImGui_ImplOpenGL3_NewFrame();
    for (const auto& event : events) {
        io.AddMousePosEvent(event.x, event.y);
        if (event.kind == 1) io.AddMouseButtonEvent(0, true);
        if (event.kind == 2) io.AddMouseButtonEvent(0, false);
    }
    ImGui::NewFrame();
    renderMinimalMenu();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        static auto lastGlErrorLog = std::chrono::steady_clock::time_point{};
        if (now - lastGlErrorLog >= std::chrono::seconds(2)) {
            lastGlErrorLog = now;
            LOGW("renderer GL error after frame: 0x%x", static_cast<unsigned>(error));
        }
    }
    g_renderedFrames.fetch_add(1, std::memory_order_relaxed);
    return true;
}'''
replace(old_render, new_render, 'render-block')

old_swap = '''EGLBoolean hookedSwapBuffers(EGLDisplay display, EGLSurface surface) {
    EGLint width = 0;
    EGLint height = 0;
    if (surface != EGL_NO_SURFACE &&
        eglQuerySurface(display, surface, EGL_WIDTH, &width) == EGL_TRUE &&
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) == EGL_TRUE) {
        g_screenWidth.store(width, std::memory_order_relaxed);
        g_screenHeight.store(height, std::memory_order_relaxed);
        drawOverlay(width, height);
    }
    return g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE;
}'''
new_swap = '''void renderBeforePresent(EGLDisplay display, EGLSurface surface, const char* path) {
    const auto frame = g_presentFrames.fetch_add(1, std::memory_order_relaxed) + 1;
    EGLint width = 0;
    EGLint height = 0;
    const EGLContext context = eglGetCurrentContext();
    if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT ||
        eglQuerySurface(display, surface, EGL_WIDTH, &width) != EGL_TRUE ||
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) != EGL_TRUE || width <= 0 || height <= 0) {
        g_renderFailures.fetch_add(1, std::memory_order_relaxed);
        if (frame <= 5 || frame % 300 == 0) {
            LOGW("present skipped: path=%s display=%p surface=%p context=%p size=%dx%d eglError=0x%x",
                 path, display, surface, context, width, height, static_cast<unsigned>(eglGetError()));
        }
        return;
    }

    g_screenWidth.store(width, std::memory_order_relaxed);
    g_screenHeight.store(height, std::memory_order_relaxed);
    drawOverlay(display, surface, width, height);

    if (frame <= 3 || frame % 300 == 0) {
        LOGI("render heartbeat: path=%s frame=%llu size=%dx%d context=%p menu=%d esp=%d status=%s",
             path, static_cast<unsigned long long>(frame), width, height, context,
             g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0,
             g_espBoxesEnabled.load(std::memory_order_relaxed) ? 1 : 0,
             lemming::esp::statusText());
    }
}

EGLBoolean hookedSwapBuffers(EGLDisplay display, EGLSurface surface) {
    renderBeforePresent(display, surface, "eglSwapBuffers");
    return g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE;
}

EGLBoolean hookedSwapBuffersWithDamageKHR(EGLDisplay display, EGLSurface surface,
                                          const EGLint* rects, EGLint count) {
    renderBeforePresent(display, surface, "eglSwapBuffersWithDamageKHR");
    return g_realSwapBuffersWithDamageKHR
        ? g_realSwapBuffersWithDamageKHR(display, surface, rects, count)
        : (g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE);
}

EGLBoolean hookedSwapBuffersWithDamageEXT(EGLDisplay display, EGLSurface surface,
                                          const EGLint* rects, EGLint count) {
    renderBeforePresent(display, surface, "eglSwapBuffersWithDamageEXT");
    return g_realSwapBuffersWithDamageEXT
        ? g_realSwapBuffersWithDamageEXT(display, surface, rects, count)
        : (g_realSwapBuffers ? g_realSwapBuffers(display, surface) : EGL_FALSE);
}'''
replace(old_swap, new_swap, 'swap-hooks')

old_input = '''int32_t hookedInputGetEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!g_realInputGetEvent) return -1;
    const int32_t result = g_realInputGetEvent(queue, outEvent);
    if (result >= 0 && outEvent && *outEvent && AInputEvent_getType(*outEvent) == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t action = AMotionEvent_getAction(*outEvent);
        const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
        const size_t index = static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                                  AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            const size_t count = AMotionEvent_getPointerCount(*outEvent);
            if (index < count) {
                handleTouch(AMotionEvent_getX(*outEvent, index), AMotionEvent_getY(*outEvent, index));
            }
        }
    }
    return result;
}'''
new_input = '''int32_t hookedInputGetEvent(AInputQueue* queue, AInputEvent** outEvent) {
    if (!g_realInputGetEvent) return -1;
    const int32_t result = g_realInputGetEvent(queue, outEvent);
    if (result >= 0 && outEvent && *outEvent && AInputEvent_getType(*outEvent) == AINPUT_EVENT_TYPE_MOTION) {
        const int32_t action = AMotionEvent_getAction(*outEvent);
        const int32_t masked = action & AMOTION_EVENT_ACTION_MASK;
        const size_t actionIndex = static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                                       AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        const size_t count = AMotionEvent_getPointerCount(*outEvent);
        const size_t index = masked == AMOTION_EVENT_ACTION_MOVE ? 0 : actionIndex;
        if (index < count) {
            const float x = AMotionEvent_getX(*outEvent, index);
            const float y = AMotionEvent_getY(*outEvent, index);
            int kind = 0;
            if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) kind = 1;
            if (masked == AMOTION_EVENT_ACTION_UP || masked == AMOTION_EVENT_ACTION_POINTER_UP ||
                masked == AMOTION_EVENT_ACTION_CANCEL) kind = 2;
            queuePointerEvent(x, y, kind);
            if (kind == 1) handleTouch(x, y);
        }
    }
    return result;
}'''
replace(old_input, new_input, 'native-input')

old_patchslot = '''bool patchSlot(void** slot, void* replacement, void** original) {
    if (!slot || !replacement) return false;
    void* current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (current == replacement) return false;
    if (original && !*original && current) *original = current;

    const long pageSize = sysconf(_SC_PAGESIZE);
    const uintptr_t page = reinterpret_cast<uintptr_t>(slot) & ~(static_cast<uintptr_t>(pageSize) - 1u);
    if (mprotect(reinterpret_cast<void*>(page), static_cast<size_t>(pageSize), PROT_READ | PROT_WRITE) != 0) {
        return false;
    }
    __atomic_store_n(slot, replacement, __ATOMIC_RELEASE);
    __builtin___clear_cache(reinterpret_cast<char*>(slot), reinterpret_cast<char*>(slot) + sizeof(void*));
    mprotect(reinterpret_cast<void*>(page), static_cast<size_t>(pageSize), PROT_READ);
    return true;
}'''
new_patchslot = '''int protectionForAddress(const void* address) {
    FILE* file = std::fopen("/proc/self/maps", "r");
    if (!file) return PROT_READ;
    char line[512];
    const auto target = reinterpret_cast<uintptr_t>(address);
    int result = PROT_READ;
    while (std::fgets(line, sizeof(line), file)) {
        unsigned long long begin = 0;
        unsigned long long end = 0;
        char perms[5]{};
        if (std::sscanf(line, "%llx-%llx %4s", &begin, &end, perms) != 3) continue;
        if (target < begin || target >= end) continue;
        result = 0;
        if (perms[0] == 'r') result |= PROT_READ;
        if (perms[1] == 'w') result |= PROT_WRITE;
        if (perms[2] == 'x') result |= PROT_EXEC;
        break;
    }
    std::fclose(file);
    return result;
}

bool patchSlot(void** slot, void* replacement, void** original) {
    if (!slot || !replacement) return false;
    void* current = __atomic_load_n(slot, __ATOMIC_ACQUIRE);
    if (current == replacement) return false;
    if (original && !*original && current) *original = current;

    const long pageSizeLong = sysconf(_SC_PAGESIZE);
    if (pageSizeLong <= 0) return false;
    const auto pageSize = static_cast<size_t>(pageSizeLong);
    const uintptr_t page = reinterpret_cast<uintptr_t>(slot) & ~(static_cast<uintptr_t>(pageSize) - 1u);
    const int originalProtection = protectionForAddress(slot);
    if (mprotect(reinterpret_cast<void*>(page), pageSize, originalProtection | PROT_WRITE) != 0) {
        LOGE("mprotect RW failed for GOT slot=%p: errno=%d %s", slot, errno, std::strerror(errno));
        return false;
    }
    __atomic_store_n(slot, replacement, __ATOMIC_RELEASE);
    __builtin___clear_cache(reinterpret_cast<char*>(slot), reinterpret_cast<char*>(slot) + sizeof(void*));
    if (mprotect(reinterpret_cast<void*>(page), pageSize, originalProtection) != 0) {
        LOGW("mprotect restore failed for GOT slot=%p: errno=%d %s", slot, errno, std::strerror(errno));
    }
    return true;
}'''
replace(old_patchslot, new_patchslot, 'patch-slot')

replace(
'''    if (!std::strstr(moduleName, "libunity.so") &&
        !std::strstr(moduleName, "libil2cpp.so") &&
        !std::strstr(moduleName, "libmain.so")) {''',
'''    if (!std::strstr(moduleName, "libunity.so") &&
        !std::strstr(moduleName, "libil2cpp.so") &&
        !std::strstr(moduleName, "libmain.so") &&
        !std::strstr(moduleName, "libGfxDevice")) {''',
'hook-module-filter')

replace(
'''#if defined(__aarch64__)
        const uintptr_t probe = base + lemming::offsets::method_rva::kWeaponryOnPreInitialize;
        g_offsetProfileReady.store(probe >= minAddress && probe < maxAddress, std::memory_order_release);
#else''',
'''#if defined(__aarch64__)
        const uintptr_t codeProbe = base + lemming::offsets::unity::kCameraWorldToScreen;
        const uintptr_t typeProbe = base + lemming::offsets::typeinfo::kPlayer;
        const bool ready = codeProbe >= minAddress && codeProbe < maxAddress &&
                           typeProbe >= minAddress && typeProbe < maxAddress;
        g_offsetProfileReady.store(ready, std::memory_order_release);
#else''',
'profile-probe')

old_targets = '''    HookTargets targets[] = {
        {"eglSwapBuffers", reinterpret_cast<void*>(hookedSwapBuffers), reinterpret_cast<void**>(&g_realSwapBuffers), &g_swapSlots},
        {"AInputQueue_getEvent", reinterpret_cast<void*>(hookedInputGetEvent), reinterpret_cast<void**>(&g_realInputGetEvent), &g_inputSlots},
    };

    if (pltRelocations && pltRelSize) {
        if (pltRelType == DT_REL) {
            const auto* relocs = reinterpret_cast<const ElfW(Rel)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, 2);
        } else if (pltRelType == DT_RELA) {
            const auto* relocs = reinterpret_cast<const ElfW(Rela)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rela)), symbols, strings, targets, 2);
        }
    }
    if (regularRel && regularRelSize) {
        patchRelocations(base, regularRel, regularRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, 2);
    }
    if (regularRela && regularRelaSize) {
        patchRelocations(base, regularRela, regularRelaSize / sizeof(ElfW(Rela)), symbols, strings, targets, 2);
    }'''
new_targets = '''    HookTargets targets[] = {
        {"eglSwapBuffers", reinterpret_cast<void*>(hookedSwapBuffers),
         reinterpret_cast<void**>(&g_realSwapBuffers), &g_swapSlots},
        {"eglSwapBuffersWithDamageKHR", reinterpret_cast<void*>(hookedSwapBuffersWithDamageKHR),
         reinterpret_cast<void**>(&g_realSwapBuffersWithDamageKHR), &g_swapDamageSlots},
        {"eglSwapBuffersWithDamageEXT", reinterpret_cast<void*>(hookedSwapBuffersWithDamageEXT),
         reinterpret_cast<void**>(&g_realSwapBuffersWithDamageEXT), &g_swapDamageSlots},
        {"AInputQueue_getEvent", reinterpret_cast<void*>(hookedInputGetEvent),
         reinterpret_cast<void**>(&g_realInputGetEvent), &g_inputSlots},
    };
    const size_t targetCount = sizeof(targets) / sizeof(targets[0]);

    if (pltRelocations && pltRelSize) {
        if (pltRelType == DT_REL) {
            const auto* relocs = reinterpret_cast<const ElfW(Rel)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, targetCount);
        } else if (pltRelType == DT_RELA) {
            const auto* relocs = reinterpret_cast<const ElfW(Rela)*>(pltRelocations);
            patchRelocations(base, relocs, pltRelSize / sizeof(ElfW(Rela)), symbols, strings, targets, targetCount);
        }
    }
    if (regularRel && regularRelSize) {
        patchRelocations(base, regularRel, regularRelSize / sizeof(ElfW(Rel)), symbols, strings, targets, targetCount);
    }
    if (regularRela && regularRelaSize) {
        patchRelocations(base, regularRela, regularRelaSize / sizeof(ElfW(Rela)), symbols, strings, targets, targetCount);
    }'''
replace(old_targets, new_targets, 'hook-targets')

old_resolve = '''void resolveOriginals() {
    void* egl = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (egl) g_realSwapBuffers = reinterpret_cast<SwapBuffersFn>(dlsym(egl, "eglSwapBuffers"));
    void* android = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (android) g_realInputGetEvent = reinterpret_cast<InputGetEventFn>(dlsym(android, "AInputQueue_getEvent"));
}

void handleToggleSignal(int) {
    g_toggleMenuSignal = 1;
}'''
new_resolve = '''void resolveOriginals() {
    void* egl = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (egl) {
        g_realSwapBuffers = reinterpret_cast<SwapBuffersFn>(dlsym(egl, "eglSwapBuffers"));
        g_realSwapBuffersWithDamageKHR = reinterpret_cast<SwapBuffersWithDamageFn>(
            dlsym(egl, "eglSwapBuffersWithDamageKHR"));
        g_realSwapBuffersWithDamageEXT = reinterpret_cast<SwapBuffersWithDamageFn>(
            dlsym(egl, "eglSwapBuffersWithDamageEXT"));
    }
    if (!g_realSwapBuffersWithDamageKHR) {
        g_realSwapBuffersWithDamageKHR = reinterpret_cast<SwapBuffersWithDamageFn>(
            eglGetProcAddress("eglSwapBuffersWithDamageKHR"));
    }
    if (!g_realSwapBuffersWithDamageEXT) {
        g_realSwapBuffersWithDamageEXT = reinterpret_cast<SwapBuffersWithDamageFn>(
            eglGetProcAddress("eglSwapBuffersWithDamageEXT"));
    }
    void* android = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (android) g_realInputGetEvent = reinterpret_cast<InputGetEventFn>(dlsym(android, "AInputQueue_getEvent"));
    LOGI("resolved originals: swap=%p damageKHR=%p damageEXT=%p input=%p",
         reinterpret_cast<void*>(g_realSwapBuffers),
         reinterpret_cast<void*>(g_realSwapBuffersWithDamageKHR),
         reinterpret_cast<void*>(g_realSwapBuffersWithDamageEXT),
         reinterpret_cast<void*>(g_realInputGetEvent));
}

void handleToggleSignal(int) {
    g_toggleMenuSignal = 1;
}

void handleDumpSignal(int) {
    g_dumpDiagnosticsSignal = 1;
}

void logDiagnostics(const char* reason) {
    LOGI("diagnostics[%s]: profile=%s ready=%d base=%p hooks(swap=%d damage=%d input=%d java=%d) frames(present=%llu rendered=%llu failures=%llu) EGL(context=%p surface=%p GLES=%d) menu=%d esp=%d",
         reason ? reason : "manual", lemming::offsets::kProfileName,
         g_offsetProfileReady.load(std::memory_order_relaxed) ? 1 : 0,
         reinterpret_cast<void*>(g_unityBase.load(std::memory_order_relaxed)),
         g_swapSlots.load(std::memory_order_relaxed),
         g_swapDamageSlots.load(std::memory_order_relaxed),
         g_inputSlots.load(std::memory_order_relaxed),
         g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0,
         static_cast<unsigned long long>(g_presentFrames.load(std::memory_order_relaxed)),
         static_cast<unsigned long long>(g_renderedFrames.load(std::memory_order_relaxed)),
         static_cast<unsigned long long>(g_renderFailures.load(std::memory_order_relaxed)),
         reinterpret_cast<void*>(g_lastEglContext.load(std::memory_order_relaxed)),
         reinterpret_cast<void*>(g_lastEglSurface.load(std::memory_order_relaxed)),
         g_glClientVersion.load(std::memory_order_relaxed),
         g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0,
         g_espBoxesEnabled.load(std::memory_order_relaxed) ? 1 : 0);
    lemming::esp::logDiagnostics(reason);
}'''
replace(old_resolve, new_resolve, 'resolve-and-diagnostics')

old_worker = '''void hookWorker() {
    resolveOriginals();
    int lastSwap = -1;
    int lastInput = -1;
    uintptr_t lastUnityBase = 0;
    bool lastProfileReady = false;
    for (;;) {
        if (g_toggleMenuSignal) {
            g_toggleMenuSignal = 0;
            g_menuOpen.store(!g_menuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
            LOGI("menu toggled by SIGUSR1: open=%d", g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0);
        }
        dl_iterate_phdr(hookModule, nullptr);
        if (!g_javaInputHookReady.load(std::memory_order_acquire)) {
            installJavaInputHook();
        }
        const int swaps = g_swapSlots.load(std::memory_order_relaxed);
        const int inputs = g_inputSlots.load(std::memory_order_relaxed);
        if (swaps != lastSwap || inputs != lastInput) {
            LOGI("hook status: swap=%d nativeInput=%d javaInput=%d signalToggle=ready", swaps, inputs,
                 g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0);
            lastSwap = swaps;
            lastInput = inputs;
        }
        const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
        const bool profileReady = g_offsetProfileReady.load(std::memory_order_acquire);
        if (unityBase != lastUnityBase || profileReady != lastProfileReady) {
            LOGI("dump64 profile: base=%p ready=%d source=%s", reinterpret_cast<void*>(unityBase),
                 profileReady ? 1 : 0, lemming::offsets::kProfileName);
            lastUnityBase = unityBase;
            lastProfileReady = profileReady;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void start() {
    bool expected = false;
    if (!g_started.compare_exchange_strong(expected, true)) return;
    struct sigaction action{};
    action.sa_handler = handleToggleSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &action, nullptr);
    std::thread(hookWorker).detach();
    LOGI("bootstrap started");
}'''
new_worker = '''void hookWorker() {
    resolveOriginals();
    int lastSwap = -1;
    int lastDamage = -1;
    int lastInput = -1;
    uintptr_t lastUnityBase = 0;
    bool lastProfileReady = false;
    std::uint64_t lastPresent = 0;
    int stagnantChecks = 0;
    for (;;) {
        if (g_toggleMenuSignal) {
            g_toggleMenuSignal = 0;
            g_menuOpen.store(!g_menuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
            LOGI("menu toggled by SIGUSR1: open=%d", g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0);
        }
        if (g_dumpDiagnosticsSignal) {
            g_dumpDiagnosticsSignal = 0;
            logDiagnostics("SIGUSR2");
        }

        dl_iterate_phdr(hookModule, nullptr);
#if defined(LEMMING_ENABLE_UNVERIFIED_JAVA_INPUT_HOOK)
        if (!g_javaInputHookReady.load(std::memory_order_acquire)) installJavaInputHook();
#endif

        const int swaps = g_swapSlots.load(std::memory_order_relaxed);
        const int damage = g_swapDamageSlots.load(std::memory_order_relaxed);
        const int inputs = g_inputSlots.load(std::memory_order_relaxed);
        if (swaps != lastSwap || damage != lastDamage || inputs != lastInput) {
            LOGI("hook status: swap=%d damage=%d nativeInput=%d javaInput=%d signals=USR1/USR2",
                 swaps, damage, inputs, g_javaInputHookReady.load(std::memory_order_relaxed) ? 1 : 0);
            lastSwap = swaps;
            lastDamage = damage;
            lastInput = inputs;
        }

        const uintptr_t unityBase = g_unityBase.load(std::memory_order_acquire);
        const bool profileReady = g_offsetProfileReady.load(std::memory_order_acquire);
        if (unityBase != lastUnityBase || profileReady != lastProfileReady) {
            LOGI("ARM64 profile: base=%p ready=%d profile=%s source=%s",
                 reinterpret_cast<void*>(unityBase), profileReady ? 1 : 0,
                 lemming::offsets::kProfileName, lemming::offsets::kSourceSha256);
            lastUnityBase = unityBase;
            lastProfileReady = profileReady;
        }

        const std::uint64_t presents = g_presentFrames.load(std::memory_order_relaxed);
        if (presents == lastPresent) {
            ++stagnantChecks;
            if (stagnantChecks == 5) {
                LOGW("no intercepted EGL present calls for 10s: swapSlots=%d damageSlots=%d; check injection timing or Vulkan renderer",
                     swaps, damage);
                logDiagnostics("no-present-watchdog");
                stagnantChecks = 0;
            }
        } else {
            stagnantChecks = 0;
            lastPresent = presents;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void start() {
    bool expected = false;
    if (!g_started.compare_exchange_strong(expected, true)) return;

    struct sigaction toggleAction{};
    toggleAction.sa_handler = handleToggleSignal;
    sigemptyset(&toggleAction.sa_mask);
    toggleAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &toggleAction, nullptr);

    struct sigaction dumpAction{};
    dumpAction.sa_handler = handleDumpSignal;
    sigemptyset(&dumpAction.sa_mask);
    dumpAction.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &dumpAction, nullptr);

    std::thread(hookWorker).detach();
    LOGI("bootstrap started: build=%s arch=%s SIGUSR1=toggle SIGUSR2=diagnostics",
         lemming::offsets::kProfileName, kArchitectureLabel);
}'''
replace(old_worker, new_worker, 'worker-start')

old_exports = '''extern "C" __attribute__((visibility("default"))) void LemmingRMT_ToggleMenu() {
    g_menuOpen.store(!g_menuOpen.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetMenuOpen(int open) {
    g_menuOpen.store(open != 0, std::memory_order_relaxed);
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsMenuOpen() {
    return g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0;
}'''
new_exports = '''extern "C" __attribute__((visibility("default"))) void LemmingRMT_ToggleMenu() {
    const bool next = !g_menuOpen.load(std::memory_order_relaxed);
    g_menuOpen.store(next, std::memory_order_relaxed);
    LOGI("menu toggled by export: open=%d", next ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetMenuOpen(int open) {
    g_menuOpen.store(open != 0, std::memory_order_relaxed);
    LOGI("menu set by export: open=%d", open != 0 ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) int LemmingRMT_IsMenuOpen() {
    return g_menuOpen.load(std::memory_order_relaxed) ? 1 : 0;
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_SetEspEnabled(int enabled) {
    g_espBoxesEnabled.store(enabled != 0, std::memory_order_relaxed);
    LOGI("basic ESP set by export: enabled=%d", enabled != 0 ? 1 : 0);
}

extern "C" __attribute__((visibility("default"))) void LemmingRMT_DumpDiagnostics() {
    logDiagnostics("export");
}'''
replace(old_exports, new_exports, 'exports')

path.write_text(s, encoding='utf-8')
print(f'updated {path}: {len(s.splitlines())} lines, {len(s.encode())} bytes')
