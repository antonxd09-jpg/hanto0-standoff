#include <jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <dlfcn.h>
#include <math.h>
#include <vector>
#include <string>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "And64InlineHook/And64InlineHook.hpp"

#define LOG_TAG "HantoO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// --- IL2CPP API ---
typedef void* (*il2cpp_domain_get_t)();
typedef void** (*il2cpp_domain_get_assemblies_t)(void* domain, size_t* size);
typedef void* (*il2cpp_assembly_get_image_t)(void* assembly);
typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);
typedef void* (*il2cpp_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);
typedef void* (*il2cpp_class_get_type_t)(void* klass);
typedef void* (*il2cpp_type_get_object_t)(void* type);

il2cpp_domain_get_t domain_get;
il2cpp_domain_get_assemblies_t domain_get_assemblies;
il2cpp_assembly_get_image_t assembly_get_image;
il2cpp_class_from_name_t class_from_name;
il2cpp_class_get_method_from_name_t class_get_method_from_name;
il2cpp_runtime_invoke_t runtime_invoke;
il2cpp_class_get_type_t class_get_type;
il2cpp_type_get_object_t type_get_object;

// Указатели на классы
void* player_class = nullptr;
void* camera_class = nullptr;
void* transform_class = nullptr;
void* object_class = nullptr;

// Методы
void* get_transform_method = nullptr;
void* get_position_method = nullptr;
void* get_main_cam_method = nullptr;
void* w2s_method = nullptr;
void* find_objects_method = nullptr;

uintptr_t base_address = 0;
bool g_ShowMenu = true;
bool g_ESP = true;
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surf) = nullptr;

void InitIL2CPP() {
    void* il2cpp = dlopen("libil2cpp.so", RTLD_LAZY);
    if (!il2cpp) return;
    base_address = (uintptr_t)il2cpp;

    domain_get = (il2cpp_domain_get_t)dlsym(il2cpp, "il2cpp_domain_get");
    domain_get_assemblies = (il2cpp_domain_get_assemblies_t)dlsym(il2cpp, "il2cpp_domain_get_assemblies");
    assembly_get_image = (il2cpp_assembly_get_image_t)dlsym(il2cpp, "il2cpp_assembly_get_image");
    class_from_name = (il2cpp_class_from_name_t)dlsym(il2cpp, "il2cpp_class_from_name");
    class_get_method_from_name = (il2cpp_class_get_method_from_name_t)dlsym(il2cpp, "il2cpp_class_get_method_from_name");
    runtime_invoke = (il2cpp_runtime_invoke_t)dlsym(il2cpp, "il2cpp_runtime_invoke");
    class_get_type = (il2cpp_class_get_type_t)dlsym(il2cpp, "il2cpp_class_get_type");
    type_get_object = (il2cpp_type_get_object_t)dlsym(il2cpp, "il2cpp_type_get_object");

    void* domain = domain_get();
    size_t count = 0;
    void** assemblies = domain_get_assemblies(domain, &count);

    for (size_t i = 0; i < count; ++i) {
        void* image = assembly_get_image(assemblies[i]);
        
        if (!player_class) {
            player_class = class_from_name(image, "Axlebolt.Standoff.Player", "PlayerController");
            if (player_class) {
                get_transform_method = class_get_method_from_name(player_class, "get_transform", 0);
            }
        }
        if (!camera_class) {
            camera_class = class_from_name(image, "UnityEngine", "Camera");
            if (camera_class) {
                w2s_method = class_get_method_from_name(camera_class, "WorldToScreenPoint", 1);
                get_main_cam_method = class_get_method_from_name(camera_class, "get_main", 0);
            }
        }
        if (!transform_class) {
            transform_class = class_from_name(image, "UnityEngine", "Transform");
            if (transform_class) {
                get_position_method = class_get_method_from_name(transform_class, "get_position", 0);
            }
        }
        if (!object_class) {
            object_class = class_from_name(image, "UnityEngine", "Object");
            if (object_class) {
                find_objects_method = class_get_method_from_name(object_class, "FindObjectsOfType", 1);
            }
        }
    }
}

// Вектор3 (Unity) - 3 float подряд
struct Vector3 { float x, y, z; };

void RenderMenu() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    if (g_ShowMenu) {
        ImGui::Begin("HantoO Internal", &g_ShowMenu);
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "HantoO Wallhack");
        ImGui::Checkbox("ESP (Wallhack)", &g_ESP);
        ImGui::End();
    }

    if (g_ESP && player_class && camera_class && find_objects_method) {
        ImDrawList* draw = ImGui::GetBackgroundDrawList();
        ImGuiIO& io = ImGui::GetIO();

        // 1. Получаем камеру
        void* main_cam = runtime_invoke(get_main_cam_method, nullptr, nullptr, nullptr);
        
        if (main_cam) {
            // 2. Ищем всех игроков на карте
            void* player_type = type_get_object(class_get_type(player_class));
            void* args[1] = { player_type };
            void* players_array = runtime_invoke(find_objects_method, nullptr, args, nullptr);

            if (players_array) {
                // Читаем длину массива (IL2CPP массив: заголовок + bounds + max_length)
                int length = *(int*)((uintptr_t)players_array + 0x18);
                
                // Начинаем читать элементы (смещение 0x20)
                uintptr_t start = (uintptr_t)players_array + 0x20;

                for (int i = 0; i < length; i++) {
                    void* player = *(void**)(start + (i * 8)); // 8 байт на указатель
                    if (!player) continue;

                    // Получаем трансформ и позицию
                    void* transform = runtime_invoke(get_transform_method, player, nullptr, nullptr);
                    if (!transform) continue;

                    void* pos_obj = runtime_invoke(get_position_method, transform, nullptr, nullptr);
                    if (!pos_obj) continue;

                    Vector3 pos = *(Vector3*)((uintptr_t)pos_obj + 0x10); // В Unity Vector3 начинается с 0x10

                    // W2S
                    void* args_w2s[1] = { &pos };
                    void* screen_pos_obj = runtime_invoke(w2s_method, main_cam, args_w2s, nullptr);

                    if (screen_pos_obj) {
                        Vector3 screenPos = *(Vector3*)((uintptr_t)screen_pos_obj + 0x10);
                        
                        // Инвертируем Y для ImGui
                        float sx = screenPos.x;
                        float sy = io.DisplaySize.y - screenPos.y;

                        if (screenPos.z > 0 && sx > 0 && sx < io.DisplaySize.x && sy > 0 && sy < io.DisplaySize.y) {
                            draw->AddRect(ImVec2(sx - 25, sy - 50), ImVec2(sx + 25, sy), IM_COL32(255, 0, 0, 255), 0, 0, 2.0f);
                        }
                    }
                }
            }
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EGLBoolean hooked_eglSwapBuffers(EGLDisplay dpy, EGLSurface surf) {
    static bool init = false;
    if (!init) {
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui_ImplOpenGL3_Init("#version 300 es");
        InitIL2CPP();
        init = true;
    }
    RenderMenu();
    return orig_eglSwapBuffers(dpy, surf);
}

__attribute__((constructor))
void init_cheat() {
    void* egl = dlopen("libEGL.so", RTLD_LAZY);
    if (egl) {
        void* swap = dlsym(egl, "eglSwapBuffers");
        if (swap) A64HookFunction(swap, (void*)hooked_eglSwapBuffers, (void**)&orig_eglSwapBuffers);
    }
}
