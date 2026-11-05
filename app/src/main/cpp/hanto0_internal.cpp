// Language: C++20 | File: hanto0_internal.cpp | Runtime: Android NDK ARM64 | Target: libunity.so (Standoff 2 1.1.0)
#include <jni.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <vector>
#include <string>
#include "offsets.hpp"

struct Vec3 {
    float x, y, z;
};

struct Quat {
    float x, y, z, w;
};

struct ConfigState {
    std::atomic<bool> aimbot{false};
    std::atomic<bool> silentaim{false};
    std::atomic<bool> triggerbot{false};
    std::atomic<bool> prediction{false};
    std::atomic<bool> esp{false};
    std::atomic<bool> skeleton{false};
    std::atomic<bool> healthbar{false};
    std::atomic<bool> nametag{false};
    std::atomic<bool> distance{false};
    std::atomic<bool> radar{false};
    std::atomic<bool> bunny{false};
    std::atomic<bool> speed{false};
    std::atomic<bool> norecoil{false};
    std::atomic<bool> nospread{false};
    std::atomic<bool> rapidfire{false};
    std::atomic<bool> nofog{false};
    std::atomic<float> fov{45.0f};
    std::atomic<float> smooth{5.0f};
    std::atomic<int> bone{0};
    std::atomic<float> speed_mult{1.0f};
    std::atomic<float> radar_size{160.0f};
} g_cfg;

static std::uintptr_t g_libunity_base = 0;
static std::uintptr_t g_client_world = 0;

static std::uintptr_t find_module_base(const char* module_name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    std::uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            base = strtoull(line, nullptr, 16);
            break;
        }
    }
    fclose(fp);
    return base;
}

template <typename T>
static bool safe_read(std::uintptr_t addr, T& out) {
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFFULL) return false;
    out = *reinterpret_cast<T*>(addr);
    return true;
}

template <typename T>
static bool safe_write(std::uintptr_t addr, const T& val) {
    if (addr < 0x10000 || addr > 0x7FFFFFFFFFFFULL) return false;
    *reinterpret_cast<T*>(addr) = val;
    return true;
}

static std::uintptr_t scan_client_world(std::uintptr_t target_klass) {
    if (!target_klass) return 0;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        std::uintptr_t start = 0, end = 0;
        char perms[8] = {0};
        if (sscanf(line, "%lx-%lx %4s", &start, &end, perms) == 3) {
            if (perms[0] == 'r' && perms[1] == 'w' && perms[3] == 'p' && (end - start) <= 0x4000000) {
                for (std::uintptr_t p = start; p + 0x50 <= end; p += 8) {
                    if (*reinterpret_cast<std::uintptr_t*>(p) == target_klass) {
                        std::uintptr_t mgr = *reinterpret_cast<std::uintptr_t*>(p + offsets::manager::ClientWorld_NetObjectManager);
                        if (mgr > 0x10000 && mgr < 0x7FFFFFFFFFFFULL) {
                            fclose(fp);
                            return p;
                        }
                    }
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

static std::string build_esp_frame() {
    if (!g_libunity_base) return "{\"players\":[]}";

    std::uintptr_t cw_typeinfo_ptr = 0, odq_typeinfo_ptr = 0;
    safe_read(g_libunity_base + offsets::manager::ClientWorld_TypeInfo, cw_typeinfo_ptr);
    safe_read(g_libunity_base + offsets::players::odq_TypeInfo, odq_typeinfo_ptr);

    if (!g_client_world && cw_typeinfo_ptr) {
        g_client_world = scan_client_world(cw_typeinfo_ptr);
    }
    if (!g_client_world || !odq_typeinfo_ptr) return "{\"players\":[]}";

    std::uintptr_t net_mgr = 0, registry = 0, all_objs = 0, arr = 0;
    int32_t count = 0;
    if (!safe_read(g_client_world + offsets::manager::ClientWorld_NetObjectManager, net_mgr) || !net_mgr) {
        g_client_world = 0;
        return "{\"players\":[]}";
    }
    if (!safe_read(net_mgr + offsets::manager::NetObjectManager_Registry, registry) || !registry) return "{\"players\":[]}";
    if (!safe_read(registry + offsets::manager::NetObjectRegistry_AllObjects, all_objs) || !all_objs) return "{\"players\":[]}";
    if (!safe_read(all_objs + offsets::list::cmz_Array, arr) || !arr) return "{\"players\":[]}";
    if (!safe_read(all_objs + offsets::list::cmz_Count, count) || count <= 0 || count > 128) return "{\"players\":[]}";

    using GetMainCamFn = std::uintptr_t(*)();
    using W2SFn = Vec3(*)(std::uintptr_t, Vec3);
    using GetPosFn = Vec3(*)(std::uintptr_t);
    using SetFogFn = void(*)(bool);

    auto get_main_cam = reinterpret_cast<GetMainCamFn>(g_libunity_base + offsets::unity::Camera_get_main_rva);
    auto world_to_screen = reinterpret_cast<W2SFn>(g_libunity_base + offsets::unity::Camera_WorldToScreenPoint_2_rva);
    auto get_pos = reinterpret_cast<GetPosFn>(g_libunity_base + offsets::players::odq_GetPosition_rva);
    auto set_fog = reinterpret_cast<SetFogFn>(g_libunity_base + offsets::unity::RenderSettings_set_fog_rva);

    if (g_cfg.nofog.load()) {
        set_fog(false);
    }

    std::uintptr_t cam = get_main_cam();
    if (!cam) return "{\"players\":[]}";

    std::uintptr_t local_player = 0;
    uint8_t local_team = 0;
    Vec3 local_pos{0, 0, 0};

    for (int32_t i = 0; i < count; ++i) {
        std::uintptr_t obj = 0, klass = 0;
        if (!safe_read(arr + offsets::list::Array_Items + i * 8, obj) || !obj) continue;
        if (!safe_read(obj, klass) || klass != odq_typeinfo_ptr) continue;
        uint8_t is_local = 0;
        safe_read(obj + offsets::localplayer::Player_IsLocal, is_local);
        if (is_local != 0) {
            local_player = obj;
            safe_read(obj + offsets::localplayer::Player_Team, local_team);
            local_pos = get_pos(obj);
            break;
        }
    }

    std::string json = "{\"players\":[";
    bool first = true;

    for (int32_t i = 0; i < count; ++i) {
        std::uintptr_t obj = 0, klass = 0;
        if (!safe_read(arr + offsets::list::Array_Items + i * 8, obj) || !obj || obj == local_player) continue;
        if (!safe_read(obj, klass) || klass != odq_typeinfo_ptr) continue;

        uint8_t team = 0;
        safe_read(obj + offsets::localplayer::Player_Team, team);
        if (local_team != 0 && team == local_team) continue;

        std::uintptr_t stats = 0;
        uint16_t hp = 100;
        if (safe_read(obj + offsets::localplayer::Player_Stats, stats) && stats) {
            safe_read(stats + offsets::localplayer::Stats_Hp, hp);
        }
        if (hp == 0 || hp > 1000) continue;

        Vec3 feet = get_pos(obj);
        Vec3 head = {feet.x, feet.y + 1.72f, feet.z};
        Vec3 s_feet = world_to_screen(cam, feet);
        Vec3 s_head = world_to_screen(cam, head);

        if (s_feet.z <= 0.1f || s_head.z <= 0.1f) continue;

        float dx = feet.x - local_pos.x;
        float dy = feet.y - local_pos.y;
        float dz = feet.z - local_pos.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        char buf[256];
        snprintf(buf, sizeof(buf),
                 "%s{\"x1\":%.1f,\"y1\":%.1f,\"x2\":%.1f,\"y2\":%.1f,\"hp\":%u,\"dist\":%.1f,\"rx\":%.1f,\"rz\":%.1f}",
                 first ? "" : ",", s_head.x, s_head.y, s_feet.x, s_feet.y, hp, dist, dx, dz);
        json += buf;
        first = false;
    }

    json += "]}";
    return json;
}

static void apply_command(const char* cmd) {
    char key[64] = {0};
    float val = 0.0f;
    if (sscanf(cmd, "%63[^=]=%f", key, &val) == 2) {
        bool b = (val > 0.5f);
        if (!strcmp(key, "aimbot")) g_cfg.aimbot = b;
        else if (!strcmp(key, "silentaim")) g_cfg.silentaim = b;
        else if (!strcmp(key, "triggerbot")) g_cfg.triggerbot = b;
        else if (!strcmp(key, "prediction")) g_cfg.prediction = b;
        else if (!strcmp(key, "esp")) g_cfg.esp = b;
        else if (!strcmp(key, "skeleton")) g_cfg.skeleton = b;
        else if (!strcmp(key, "healthbar")) g_cfg.healthbar = b;
        else if (!strcmp(key, "nametag")) g_cfg.nametag = b;
        else if (!strcmp(key, "distance")) g_cfg.distance = b;
        else if (!strcmp(key, "radar")) g_cfg.radar = b;
        else if (!strcmp(key, "bunny")) g_cfg.bunny = b;
        else if (!strcmp(key, "speed")) g_cfg.speed = b;
        else if (!strcmp(key, "norecoil")) g_cfg.norecoil = b;
        else if (!strcmp(key, "nospread")) g_cfg.nospread = b;
        else if (!strcmp(key, "rapidfire")) g_cfg.rapidfire = b;
        else if (!strcmp(key, "nofog")) g_cfg.nofog = b;
        else if (!strcmp(key, "fov")) g_cfg.fov = val;
        else if (!strcmp(key, "smooth")) g_cfg.smooth = val;
        else if (!strcmp(key, "bone")) g_cfg.bone = static_cast<int>(val);
        else if (!strcmp(key, "speed_val")) g_cfg.speed_mult = val;
        else if (!strcmp(key, "radar_val")) g_cfg.radar_size = val;
    }
}

static void* ipc_server_thread(void*) {
    while (!g_libunity_base) {
        g_libunity_base = find_module_base("libunity.so");
        usleep(250000);
    }

    int srv = socket(AF_UNIX, SOCK_STREAM, 0);
    if (srv < 0) return nullptr;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const char* sock_name = "hanto0_so2_ipc";
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, sock_name, sizeof(addr.sun_path) - 2);
    socklen_t len = offsetof(sockaddr_un, sun_path) + 1 + strlen(sock_name);

    bind(srv, reinterpret_cast<sockaddr*>(&addr), len);
    listen(srv, 4);

    while (true) {
        int client = accept(srv, nullptr, nullptr);
        if (client < 0) continue;
        char buf[256] = {0};
        ssize_t n = read(client, buf, sizeof(buf) - 1);
        if (n > 0) {
            if (strncmp(buf, "GET_ESP", 7) == 0) {
                std::string frame = build_esp_frame();
                write(client, frame.c_str(), frame.size());
            } else {
                apply_command(buf);
                write(client, "OK", 2);
            }
        }
        close(client);
    }
    return nullptr;
}

__attribute__((constructor))
static void on_load() {
    pthread_t tid;
    pthread_create(&tid, nullptr, ipc_server_thread, nullptr);
    pthread_detach(tid);
}
