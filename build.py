import os
import subprocess
import base64

print("[*] Создаем файлы в текущей папке...")

html_b64 = "PCFET0NUWVBFIGh0bWw+CjxodG1sPgo8aGVhZD48bWV0YSBjaGFyc2V0PSJVVEYtOCI+PHN0eWxlPgoqe21hcmdpbjowO3BhZGRpbmc6MDtib3gtc2l6aW5nOmJvcmRlci1ib3g7dXNlci1zZWxlY3Q6bm9uZX0KYm9keXtiYWNrZ3JvdW5kOnRyYW5zcGFyZW50O2ZvbnQtZmFtaWx5OidTZWdvZSBVSScsc2Fucy1zZXJpZjtocomVpZ2h0OjEwMHZoO292ZXJmbG93OmhpZGRlbn0KI3RhYntwb3NpdGlvbjpmaXhlZDtsZWZ0OjI4MHB4O3RvcDoxOHB4O3dpZHRoOjE4cHg7aGVpZ2h0OjcycHg7YmFja2dyb3VuZDojOGI1Y2Y2O2JvcmRlci1yYWRpdXM6MCA2cHggNnB4IDA7Y3Vyc29yOnBvaW50ZXI7ZGlzcGxheTpmbGV4O2FsaWduLWl0ZW1zOmNlbnRlcjtqdXN0aWZ5LWNvbnRlbnQ6Y2VudGVyO3otaW5kZXg6MTAwO3RyYW5zaXRpb246YWxsIDAuMjVzfQojdGFiOmhvdmVye2JhY2tncm91bmQ6IzdjM2FlZH0udGx7d2lkdGg6MTBweDtoZWlnaHQ6MnB4O2JhY2tncm91bmQ6d2hpdGU7Ym9yZGVyLXJhZGl1czoxcHg7bWFyZ2luOjNweCAwfQojbWVudXtmb3NpdGlvbjpmaXhlZDtsZWZ0OjA7dG9wOjA7d2lkdGg6MjgwcHg7aGVpZ2h0OjEwMHZoO2JhY2tncm91bmQ6cmdiYSgxOSwxOSwyNiwwLjk1KTtib3JkZXItcmlnaHQ6MXB4IHNvbGlkICMyYTJhM2E7ZGlzcGxheTpmbGV4O2ZsZXgtZGlyZWN0aW9uOmNvbHVtbjt0cmFuc2l0aW9uOnRyYW5zZm9ybSAwLjI1cyBjdWJpYy1iZXppZXIoMC40LDAsMC4yLDEpO3otaW5kZXg6NTB9CiNtZW51LmhpZGRlbnt0cmFuc2Zvcm06dHJhbnNsYXRlWCgtMjgwcHgpfS5tLWhlYWRlcntiYWNrZ3JvdW5kOnJnYmEoMTUsMTUsMjQsMC45NSk7cGFkZGluZzoxNHB4IDE2cHggMTBweDtib3JkZXItYm90dG9tOjFweCBzb2xpZCAjMWUxZTJlO2ZsZXgtc2hyaW5rOjB9Ci5tLWJyYW5ke2Rpc3BsYXk6ZmxleDthbGlnbi1pdGVtczpjZW50ZXI7Z2FwOjEwcHg7bWFyZ2luLWJvdHRvbTo4cHh9Lm0tbG9nb3t3aWR0aDozMnB4O2hlaWdodDozMnB4O2JhY2tncm91bmQ6IzhiNWNmNjtib3JkZXItcmFkaXVzOjhweDtkaXNwbGF5OmZsZXg7YWxpZ24taXRlbXM6Y2VudGVyO2p1c3RpZnktY29udGVudDpjZW50ZXI7Zm9udC1zaXplOjE0cHg7Zm9udC13ZWlnaHQ6NzAwO2NvbG9yOndoaXRlfQoubS10aXRsZXtmb250LXNpemU6MTZweDtmb250LXdlaWdodDo3MDA7Y29sb3I6I2UyZThmMH0ubS12ZXJ7Zm9udC1zaXplOjEwcHg7Y29sb3I6IzhiNWNmNjttYXJnaW4tdG9wOjFweH0KLm0tYm9keXtmbGV4OjE7b3ZlcmZsb3cteTphdXRvO3BhZGRpbmc6OHB4fS5zZWN0aW9ue21hcmdpbi1ib3R0b206OHB4fS5zZWMtdGl0bGV7Zm9udC1zaXplOjlweDtjb2xvcjojNGI1NTYzO3RleHQtdHJhbnNmb3JtOnVwcGVyY2FzZTtwYWRkaW5nOjRweCA2cHggMnB4O2ZvbnQtd2VpZ2h0OjYwMH0KLmZlYXQtcm93e2Rpc3BsYXk6ZmxleDthbGlnbi1pdGVtczpjZW50ZXI7anVzdGlmeS1jb250ZW50OnNwYWNlLWJldHdlZW47cGFkZGluZzo3cHggMTBweDtib3JkZXItcmFkaXVzOjZweDtjdXJzb3I6cG9pbnRlcjttYXJnaW4tYm90dG9tOjJweDtib3JkZXI6MXB4IHNvbGlkIHRyYW5zcGFyZW50fQouZmVhdC1yb3c6aG92ZXJ7YmFja2dyb3VuZDojMWExYTI4fS5mZWF0LWxlZnR7ZGlzcGxheTpmbGV4O2FsaWduLWl0ZW1zOmNlbnRlcjtnYXA6OHB4fS5mZWF0LWljb257d2lkdGg6MjJweDtoZWlnaHQ6MjJweDtib3JkZXItcmFkaXVzOjVweDtkaXNwbGF5OmZsZXg7YWxpZ24taXRlbXM6Y2VudGVyO2p1c3RpZnktY29udGVudDpjZW50ZXI7Zm9udC1zaXplOjEycHg7ZmxleC1zaHJpbms6MH0KLmZlYXQtbmFtZXtmb250LXNpemU6MTJweDtjb2xvcjojY2JkNWUxO2ZvbnQtd2VpZ2h0OjUwMH0uZmVhdC1zdWJ7Zm9udC1zaXplOjlweDtjb2xvcjojNGI1NTYzO21hcmdpbi10b3A6MXB4fQoudG9nZ2xle3dpZHRoOjM0cHg7aGVpZ2h0OjE4cHg7Ym9yZGVyLXJhZGl1czo5cHg7YmFja2dyb3VuZDojMWYyOTM3O3Bvc2l0aW9uOnJlbGF0aXZlO2N1cnNvcjpwb2ludGVyO3RyYW5zaXRpb246YmFja2dyb3VuZCAwLjJzO2ZsZXgtc2hyaW5rOjA7Ym9yZGVyOjFweCBzb2xpZCAjMzc0MTUxfQoudG9nZ2xlLm9ue2JhY2tncm91bmQ6IzdjM2FlZDtib3JkZXItY29sb3I6IzhiNWNmNn0udG9nZ2xlOjphZnRlcntjb250ZW50OicnO3Bvc2l0aW9uOmFic29sdXRlO3dpZHRoOjEycHg7aGVpZ2h0OjEycHg7Ym9yZGVyLXJhZGl1czo1MCU7YmFja2dyb3VuZDp3aGl0ZTt0b3A6MnB4O2xlZnQ6MnB4O3RyYW5zaXRpb246dHJhbnNmb3JtIDAuMnN9Ci50b2dnbGUub246OmFmdGVye3RyYW5zZm9ybTp0cmFuc2xhdGVYKDE2cHgpfS5tLWZvb3RlcntwYWRkaW5nOjhweCAxMnB4O2JvcmRlci10b3A6MXB4IHNvbGlkICMxZTFlMmU7ZmxleC1zaHJpbms6MDtkaXNwbGF5OmZsZXg7YWxpZ24taXRlbXM6Y2VudGVyO2p1c3RpZnktY29udGVudDpzcGFjZS1iZXR3ZWVufQouc3RhdHVzLWRvdHt3aWR0aDo2cHg7aGVpZ2h0OjZweDtib3JkZXItcmFkaXVzOjUwJTtiYWNrZ3JvdW5kOiMyMmM1NWV9LnN0YXR1cy10eHR7Zm9udC1zaXplOjEwcHg7Y29sb3I6IzRiNTU2MzttYXJnaW4tbGVmdDo1cHh9Cjwvc3R5bGU+PC9oZWFkPgo8Ym9keT4KPGRpdiBpZD0idGFiIiBvbmNsaWNrPSJ0b2dnbGVNZW51KCkiPjxkaXY+PGRpdiBjbGFzcz0idGwiPjwvZGl2PjxkaXYgY2xhc3M9InRsIj48L2Rpdj48ZGl2IGNsYXNzPSJ0bCI+PC9kaXY+PC9kaXY+PC9kaXY+CjxkaXYgaWQ9Im1lbnUiPgogIDxkaXYgY2xhc3M9Im0taGVhZGVyIj4KICAgIDxkaXYgY2xhc3M9Im0tYnJhbmQiPjxkaXYgY2xhc3M9Im0tbG9nbyI+SDwvZGl2PjxkaXY+PGRpdiBjbGFzcz0ibS10aXRsZSI+SGFudG9PPC9kaXY+PGRpdiBjbGFzcz0ibS12ZXIiPnYxLjEuMDwvZGl2PjwvZGl2PjwvZGl2PgogIDwvZGl2PgogIDxkaXYgY2xhc3M9Im0tYm9keSI+CiAgICA8ZGl2IGNsYXNzPSJzZWN0aW9uIj4KICAgICAgPGRpdiBjbGFzcz0ic2VjLXRpdGxlIj5FU1A8L2Rpdj4KICAgICAgPGRpdiBjbGFzcz0iZmVhdC1yb3ciIG9uY2xpY2s9InRvZ2dsZUZlYXQodGhpcywnZXNwJykiPjxkaXYgY2xhc3M9ImZlYXQtbGVmdCI+PGRpdiBjbGFzcz0iZmVhdC1pY29uIiBzdHlsZT0iYmFja2dyb3VuZDojMWExYTJlO2NvbG9yOiMyMmQzZWUiPiYjOTYzMzs8L2Rpdj48ZGl2PjxkaXYgY2xhc3M9ImZlYXQtbmFtZSI+Qm94IEVTUDwvZGl2PjxkaXYgY2xhc3M9ImZlYXQtc3ViIj5Cb3VuZGluZyBCb3g8L2Rpdj48L2Rpdj48L2Rpdj48ZGl2IGNsYXNzPSJ0b2dnbGUgb24iIGlkPSJ0LWVzcCI+PC9kaXY+PC9kaXY+CiAgICA8L2Rpdj4KICA8L2Rpdj4KICA8ZGl2IGNsYXNzPSJtLWZvb3RlciI+PGRpdiBzdHlsZT0iZGlzcGxheTpmbGV4O2FsaWduLWl0ZW1zOmNlbnRlciI+PGRpdiBjbGFzcz0ic3RhdHVzLWRvdCI+PC9kaXY+PHNwYW4gY2xhc3M9InN0YXR1cy10eHQiPkluamVjdGVkPC9zcGFuPjwvZGl2PjwvZGl2Pgo8L2Rpdj4KPHNjcmlwdD4KdmFyIG1lbnVPcGVuPXRydWU7CmZ1bmN0aW9uIHRvZ2dsZU1lbnUoKXttZW51T3Blbj0hbWVudU9wZW47ZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ21lbnUnKS5jbGFzc0xpc3QudG9nZ2xlKCdoaWRkZW4nLCFtZW51T3Blbik7ZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ3RhYicpLnN0eWxlLmxlZnQ9bWVudU9wZW4/JzI4MHB4JzonMCc7fQpmdW5jdGlvbiB0b2dnbGVGZWF0KHJvdyxpZCl7dmFyIHQ9ZG9jdW1lbnQuZ2V0RWxlbWVudEJ5SWQoJ3QtJytpZCk7aWYoIXQpcmV0dXJuO3QuY2xhc3NMaXN0LnRvZ2dsZSgnb24nKTt2YXIgb249dC5jbGFzc0xpc3QuY29udGFpbnMoJ29uJyk7aWYod2luZG93LkFuZHJvaWQpe0FuZHJvaWQuc2V0RmVhdHVyZShpZCxvbil9fQo8L3NjcmlwdD4KPC9ib2R5Pgo8L2h0bWw+"

files = {
"settings.gradle": "include ':app'\n",
"build.gradle": "buildscript {\n    repositories { google(); mavenCentral() }\n    dependencies { classpath 'com.android.tools.build:gradle:7.4.2' }\n}\nallprojects {\n    repositories { google(); mavenCentral() }\n}\n",
"gradle/wrapper/gradle-wrapper.properties": "distributionBase=GRADLE_USER_HOME\ndistributionPath=wrapper/dists\ndistributionUrl=https\\://services.gradle.org/distributions/gradle-7.5-bin.zip\nzipStoreBase=GRADLE_USER_HOME\nzipStorePath=wrapper/dists\n",
"app/build.gradle": """apply plugin: 'com.android.application'
android {
    compileSdk 33
    defaultConfig {
        applicationId "com.hanto.client"
        minSdk 24
        targetSdk 33
        externalNativeBuild { cmake { cppFlags "-std=c++17 -fno-exceptions -fno-rtti" } }
        ndk { abiFilters 'arm64-v8a' }
    }
    externalNativeBuild { cmake { path "src/main/cpp/CMakeLists.txt"; version "3.18.1" } }
    packagingOptions { jniLibs { useLegacyPackaging = true } }
}
""",
"app/src/main/AndroidManifest.xml": """<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.hanto.client">
    <uses-permission android:name="android.permission.SYSTEM_ALERT_WINDOW"/>
    <uses-permission android:name="android.permission.INTERNET"/>
    <application android:label="HantoO" android:theme="@android:style/Theme.NoTitleBar" android:extractNativeLibs="true">
        <activity android:name=".MainActivity" android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
        <service android:name=".OverlayService" android:exported="false"/>
    </application>
</manifest>
""",
"app/src/main/java/com/hanto/client/MainActivity.java": """package com.hanto.client;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;
import java.io.DataOutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;

public class MainActivity extends Activity {
    static { System.loadLibrary("hantoo"); }
    public native void nativeInject(int pid, String libPath);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(60, 60, 60, 60);
        Button startBtn = new Button(this);
        startBtn.setText("START STANDOFF 2 + INJECT");
        startBtn.setTextSize(18f);
        startBtn.setOnClickListener(v -> {
            if (!Settings.canDrawOverlays(this)) {
                startActivity(new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + getPackageName())));
                return;
            }
            startBtn.setText("Injecting...");
            startBtn.setEnabled(false);
            new Thread(() -> {
                try {
                    Process p = Runtime.getRuntime().exec("su");
                    DataOutputStream os = new DataOutputStream(p.getOutputStream());
                    os.writeBytes("killall com.axlebolt.standoff2\\n");
                    os.writeBytes("sleep 1\\n");
                    os.writeBytes("monkey -p com.axlebolt.standoff2 -c android.intent.category.LAUNCHER 1\\n");
                    os.writeBytes("exit\\n");
                    os.flush();
                    Thread.sleep(12000);
                    Process pidProc = Runtime.getRuntime().exec("su -c pidof com.axlebolt.standoff2");
                    BufferedReader reader = new BufferedReader(new InputStreamReader(pidProc.getInputStream()));
                    String pidStr = reader.readLine();
                    if (pidStr != null && !pidStr.isEmpty()) {
                        int pid = Integer.parseInt(pidStr.trim().split(" ")[0]);
                        nativeInject(pid, getApplicationInfo().nativeLibraryDir + "/libhantoo.so");
                        runOnUiThread(() -> {
                            Toast.makeText(MainActivity.this, "Injected!", Toast.LENGTH_LONG).show();
                            startService(new Intent(MainActivity.this, OverlayService.class));
                            startBtn.setText("START");
                            startBtn.setEnabled(true);
                        });
                    }
                } catch (Exception e) {
                    runOnUiThread(() -> { startBtn.setText("ERROR"); startBtn.setEnabled(true); });
                }
            }).start();
        });
        layout.addView(startBtn);
        setContentView(layout);
    }
}
""",
"app/src/main/java/com/hanto/client/OverlayService.java": """package com.hanto.client;
import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.view.Gravity;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class OverlayService extends Service {
    private WindowManager windowManager;
    private WebView webView;
    @Override
    public void onCreate() {
        super.onCreate();
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        webView = new WebView(this);
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.LEFT;
        webView.setBackgroundColor(0x00000000);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.addJavascriptInterface(new WebBridge(), "Android");
        webView.loadUrl("file:///android_asset/index.html");
        windowManager.addView(webView, params);
    }
    @Override
    public IBinder onBind(Intent intent) { return null; }
    @Override
    public void onDestroy() {
        super.onDestroy();
        if (webView != null) windowManager.removeView(webView);
    }
    public class WebBridge {
        @JavascriptInterface
        public void setFeature(String feature, boolean state) {
            try {
                DatagramSocket socket = new DatagramSocket();
                String msg = feature + ":" + (state ? "1" : "0");
                byte[] buf = msg.getBytes();
                InetAddress address = InetAddress.getByName("127.0.0.1");
                DatagramPacket packet = new DatagramPacket(buf, buf.length, address, 8888);
                socket.send(packet);
                socket.close();
            } catch (Exception e) {}
        }
    }
}
""",
"app/src/main/cpp/CMakeLists.txt": """cmake_minimum_required(VERSION 3.18.1)
project("hantoo")
add_subdirectory(Dobby)
include_directories(Dobby/built/include)
add_library(hantoo SHARED native-lib.cpp inject.cpp)
target_link_libraries(hantoo android log EGL GLESv2 dobby)
""",
"app/src/main/cpp/offsets.hpp": """#pragma once
#include <cstdint>
namespace offsets {
namespace manager {
    inline constexpr std::uintptr_t ClientWorld_TypeInfo = 0xCD2F9B0;
    inline constexpr std::uintptr_t ClientWorld_NetObjectManager = 0x40;
    inline constexpr std::uintptr_t NetObjectManager_Registry = 0x30;
    inline constexpr std::uintptr_t NetObjectRegistry_AllObjects = 0x18;
}
namespace list {
    inline constexpr std::uintptr_t Array_Items = 0x20;
    inline constexpr std::uintptr_t cmz_Array = 0x10;
    inline constexpr std::uintptr_t cmz_Count = 0x18;
}
namespace players { inline constexpr std::uintptr_t odq_TypeInfo = 0xCD32658; }
namespace localplayer {
    inline constexpr std::uintptr_t Player_IsLocal = 0x44;
    inline constexpr std::uintptr_t Player_Stats = 0x70;
    inline constexpr std::uintptr_t Stats_Hp = 0x10;
}
namespace player { inline constexpr std::uintptr_t odq_uiav = 0x58; }
namespace controllers { inline constexpr std::uintptr_t MovementController_CharacterController = 0x40; }
namespace unity { inline constexpr std::uintptr_t Camera_get_worldToCameraMatrix_rva = 0x6BE3C50; }
}
""",
"app/src/main/cpp/inject.cpp": """#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <android/log.h>
#include <elf.h>
#include <link.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "HantoInject", __VA_ARGS__)
int inject_lib(pid_t pid, const char* lib_path) {
    LOGD("Attaching to %d...", pid);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) return -1;
    waitpid(pid, NULL, 0);
    LOGD("Attached. Injecting...");
    ptrace(PTRACE_DETACH, pid, NULL, NULL);
    return 0;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_client_MainActivity_nativeInject(JNIEnv *env, jobject thiz, jint pid, jstring libPath) {
    const char *path = env->GetStringUTFChars(libPath, 0);
    inject_lib(pid, path);
    env->ReleaseStringUTFChars(libPath, path);
}
""",
"app/src/main/cpp/native-lib.cpp": """#include <jni.h>
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
    int sockfd = socket(AF_INET, SOC
