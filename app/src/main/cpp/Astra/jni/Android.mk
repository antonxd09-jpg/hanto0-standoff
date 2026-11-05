LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := dobby
LOCAL_SRC_FILES := third_party/dobby/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/third_party/dobby/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := astra
LOCAL_SRC_FILES := src/core/entry.cpp \
    third_party/imgui/imgui.cpp \
    third_party/imgui/imgui_draw.cpp \
    third_party/imgui/imgui_tables.cpp \
    third_party/imgui/imgui_widgets.cpp \
    third_party/imgui/backends/imgui_impl_opengl3.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src $(LOCAL_PATH)/third_party/imgui $(LOCAL_PATH)/third_party/imgui/backends
LOCAL_CPPFLAGS := -DIMGUI_IMPL_OPENGL_ES3 -std=c++17 -O2 -ffunction-sections -fdata-sections -fvisibility=hidden -Wall -Wextra -DNDEBUG
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-u,DobbyHook
LOCAL_LDLIBS := -llog -lEGL -lGLESv3 -landroid -ldl
LOCAL_STATIC_LIBRARIES := dobby
LOCAL_LDLIBS += -lOpenSLES
include $(BUILD_SHARED_LIBRARY)
