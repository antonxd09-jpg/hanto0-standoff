#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "offsets.hpp"
#include "dobby.h"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "HantoO", __VA_ARGS__)
bool esp_on = true;
uintptr_t libunity_base = 0;
struct Vector3 { float x, y, z; };
struct Vector2 { float x, y; };
struct Matrix4x4 { float m[4][4]; };
Matrix4x4 viewMatrix;
template <typename T> T Read(uintptr_t addr) { return *(T*)addr; }
uintptr_t get_module_base(const char* module_name) {
    uintptr_t base = 0; std::ifstream maps("/proc/self/maps"); std::string line;
    while (std::getline(maps, line)) {
        if (line.find(module_name) != std::string::npos && line.find("r-xp") != std::string::npos) { sscanf(line.c_str(), "%lx", &base); break; }
    }
    return base;
}
typedef void (*Camera_get_worldToCameraMatrix_t)(void* instance, Matrix4x4* out);
Camera_get_worldToCameraMatrix_t orig_get_worldToCameraMatrix;
void hook_get_worldToCameraMatrix(void* instance, Matrix4x4* out) {
    orig_get_worldToCameraMatrix(instance, out);
    viewMatrix = *out;
}
bool WorldToScreen(Vector3 pos, Vector2& screen, float sw, float sh) {
    float cx = pos.x * viewMatrix.m[0][0] + pos.y * viewMatrix.m[1][0] + pos.z * viewMatrix.m[2][0] + viewMatrix.m[3][0];
    float cy = pos.x * viewMatrix.m[0][1] + pos.y * viewMatrix.m[1][1] + pos.z * viewMatrix.m[2][1] + viewMatrix.m[3][1];
    float cw = pos.x * viewMatrix.m[0][3] + pos.y * viewMatrix.m[1][3] + pos.z * viewMatrix.m[2][3] + viewMatrix.m[3][3];
    if (cw < 0.1f) return false;
    screen.x = (sw / 2.0f) * (cx / cw + 1.0f); screen.y = (sh / 2.0f) * (1.0f - cy / cw); return true;
}
void DrawBox(float x, float y, float w, float h, float r, float g, float b) {
    GLfloat verts[] = { x, y, x + w, y, x + w, y + h, x, y + h, x, y };
    glEnableClientState(GL_VERTEX_ARRAY); glVertexPointer(2, GL_FLOAT, 0, verts); glColor4f(r, g, b, 1.0f); glLineWidth(2.0f); glDrawArrays(GL_LINE_STRIP, 0, 5); glDisableClientState(GL_VERTEX_ARRAY);
}
uintptr_t client_world = 0;
void FindClientWorld() {
    uintptr_t typeInfoPtr = Read<uintptr_t>(libunity_base + offsets::manager::ClientWorld_TypeInfo);
    for (uintptr_t addr = libunity_base + 0x1000000; addr < libunity_base + 0x3000000; addr += 8) {
        if (Read<uintptr_t>(addr) == typeInfoPtr) { client_world = addr; break; }
    }
}
void DrawESP(float sw, float sh) {
    if (!client_world) return;
    uintptr_t netObjMgr = Read<uintptr_t>(client_world + offsets::manager::ClientWorld_NetObjectManager); if (!netObjMgr) return;
    uintptr_t registry = Read<uintptr_t>(netObjMgr + offsets::manager::NetObjectManager_Registry); if (!registry) return;
    uintptr_t allObjects = Read<uintptr_t>(registry + offsets::manager::NetObjectRegistry_AllObjects); if (!allObjects) return;
    uintptr_t arrayPtr = Read<uintptr_t>(allObjects + offsets::list::cmz_Array); int count = Read<int>(allObjects + offsets::list::cmz_Count); if (!arrayPtr || count <= 0) return;
    uintptr_t odq_type = Read<uintptr_t>(libunity_base + offsets::players::odq_TypeInfo);
    for (int i = 0; i < count; i++) {
        uintptr_t player = Read<uintptr_t>(arrayPtr + offsets::list::Array_Items + (i * 8)); if (!player || Read<uintptr_t>(player) != odq_type) continue;
        if (Read<bool>(player + offsets::localplayer::Player_IsLocal)) continue;
        uintptr_t stats = Read<uintptr_t>(player + offsets::localplayer::Player_Stats); if (!stats) continue;
        int hp = Read<unsigned short>(stats + offsets::localplayer::Stats_Hp); if (hp <= 0 || hp > 100) continue;
        uintptr_t movement = Read<uintptr_t>(player + offsets::player::odq_uiav); if (!movement) continue;
        uintptr_t charCtrl = Read<uintptr_t>(movement + offsets::controllers::MovementController_CharacterController); if (!charCtrl) continue;
        uintptr_t transform = Read<uintptr_t>(charCtrl + 0x38); if (!transform) continue;
        Vector3 pos = Read<Vector3>(transform + 0x44); Vector2 screen;
        if (WorldToScreen(pos, screen, sw, sh)) DrawBox(screen.x - 30, screen.y - 60, 60, 120, 1.0f, 0.0f, 0.0f);
    }
}
typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay dpy, EGLSurface surf);
eglSwapBuffers_t orig_eglSwapBuffers = nullptr;
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    EGLint width, height; eglQuerySurface(dpy, surf, EGL_WIDTH, &width); eglQuerySurface(dpy, surf, EGL_HEIGHT, &height);
    glViewport(0, 0, width, height); glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrthof(0, width, height, 0, -1, 1); glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
    if (esp_on) DrawESP((float)width, (float)height);
    glEnable(GL_DEPTH_TEST);
    return orig_eglSwapBuffers(dpy, surf);
}
void* socket_thread(void*) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET; servaddr.sin_port = htons(8888); servaddr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    char buffer[1024];
    while (true) {
        int n = recv(sockfd, (char *)buffer, 1024, MSG_WAITALL);
        if (n > 0) {
            buffer[n] = '\0';
            if (strncmp(buffer, "esp:1", 5) == 0) esp_on = true;
            if (strncmp(buffer, "esp:0", 5) == 0) esp_on = false;
        }
    }
    return nullptr;
}
void* init_thread(void*) {
    while (!libunity_base) { libunity_base = get_module_base("libunity.so"); usleep(100000); }
    FindClientWorld();
    DobbyHook((void*)(libunity_base + offsets::unity::Camera_get_worldToCameraMatrix_rva), (void*)hook_get_worldToCameraMatrix, (void**)&orig_get_worldToCameraMatrix);
    return nullptr;
}
__attribute__((constructor))
void init() {
    pthread_t t1, t2;
    pthread_create(&t1, nullptr, init_thread, nullptr);
    pthread_create(&t2, nullptr, socket_thread, nullptr);
    void* egl = dlopen("libEGL.so", RTLD_NOW);
    if (egl) { void* sym = dlsym(egl, "eglSwapBuffers"); if (sym) { DobbyHook(sym, (void*)hook_eglSwapBuffers, (void**)&orig_eglSwapBuffers); } }
}
