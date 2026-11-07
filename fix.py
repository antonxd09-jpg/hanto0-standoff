import os

def w(p, c):
    d = os.path.dirname(p)
    if d: os.makedirs(d, exist_ok=True)
    with open(p, "w") as f: f.write(c)

w("settings.gradle", """rootProject.name = "hanto0-standoff"
include ':app'""")

w("build.gradle", """buildscript {
    repositories { google(); mavenCentral() }
    dependencies {
        classpath 'com.android.tools.build:gradle:8.1.4'
        classpath 'org.jetbrains.kotlin:kotlin-gradle-plugin:1.9.22'
    }
}
allprojects { repositories { google(); mavenCentral() } }""")

w("gradle.properties", """org.gradle.jvmargs=-Xmx2048m
android.useAndroidX=true
android.enableJetifier=true""")

w("app/build.gradle", """plugins { id 'com.android.application'; id 'org.jetbrains.kotlin.android' }
android {
    namespace 'com.hanto.standoff'
    compileSdk 34
    ndkVersion "25.2.9519653"
    defaultConfig {
        applicationId "com.hanto.standoff"
        minSdk 24; targetSdk 34; versionCode 1; versionName "2.0"
        externalNativeBuild { cmake { cppFlags "-std=c++17 -O2 -fexceptions" } }
        ndk { abiFilters "arm64-v8a" }
    }
    externalNativeBuild { cmake { path "src/main/cpp/CMakeLists.txt" } }
    buildTypes { release { minifyEnabled false } }
    compileOptions { sourceCompatibility JavaVersion.VERSION_17; targetCompatibility JavaVersion.VERSION_17 }
    kotlinOptions { jvmTarget = "17" }
}
dependencies {
    implementation 'androidx.core:core-ktx:1.12.0'
    implementation 'androidx.appcompat:appcompat:1.6.1'
}""")

w("app/src/main/AndroidManifest.xml", """<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android">
    <uses-permission android:name="android.permission.SYSTEM_ALERT_WINDOW"/>
    <uses-permission android:name="android.permission.FOREGROUND_SERVICE"/>
    <uses-permission android:name="android.permission.INTERNET"/>
    <application android:allowBackup="true" android:label="hanto v2" android:theme="@style/Theme.AppCompat.NoActionBar">
        <activity android:name=".MainActivity" android:exported="true">
            <intent-filter>
                <action android:name="android.intent.action.MAIN"/>
                <category android:name="android.intent.category.LAUNCHER"/>
            </intent-filter>
        </activity>
        <service android:name=".OverlayService" android:foregroundServiceType="specialUse" android:exported="false"/>
    </application>
</manifest>""")

w("app/src/main/res/values/strings.xml", """<resources><string name="app_name">hanto v2</string></resources>""")

w("app/src/main/java/com/hanto/standoff/MainActivity.kt", """package com.hanto.standoff
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!Settings.canDrawOverlays(this)) {
            startActivity(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName")))
        }
        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER; setPadding(64,64,64,64) }
        root.addView(TextView(this).apply { text = "hanto v2"; textSize = 28f })
        root.addView(Button(this).apply {
            text = "START STANDOFF"; textSize = 18f
            setOnClickListener {
                GameLauncher.start(this@MainActivity)
                Handler(Looper.getMainLooper()).postDelayed({ startForegroundService(Intent(this@MainActivity, OverlayService::class.java)) }, 10_000L)
            }
        })
        root.addView(Button(this).apply {
            text = "STOP OVERLAY"; textSize = 18f
            setOnClickListener { stopService(Intent(this@MainActivity, OverlayService::class.java)) }
        })
        setContentView(root)
    }
}""")

w("app/src/main/java/com/hanto/standoff/GameLauncher.kt", """package com.hanto.standoff
import android.content.Context
object GameLauncher {
    const val PKG = "com.axlebolt.standoff2"
    fun start(ctx: Context) {
        Shell.run("am force-stop $PKG")
        Shell.run("monkey -p $PKG -c android.intent.category.LAUNCHER 1")
    }
    fun pid(): Int = Shell.run("pidof $PKG").trim().toIntOrNull() ?: 0
}
object Shell {
    fun run(cmd: String): String = try {
        val p = Runtime.getRuntime().exec(arrayOf("su","-c",cmd))
        val out = p.inputStream.bufferedReader().readText()
        p.waitFor(); out
    } catch (e: Exception) { "" }
}""")

w("app/src/main/java/com/hanto/standoff/Native.kt", """package com.hanto.standoff
object Native {
    init { System.loadLibrary("hanto") }
    external fun attach(pid: Int): Boolean
    external fun detach()
    external fun getEntities(): FloatArray
    external fun setAim(enabled: Boolean, visibleOnly: Boolean, bone: Int, fov: Float)
    external fun setTriggerbot(enabled: Boolean)
    external fun setVisual(box: Boolean, skeleton: Boolean, line: Boolean, chams: Boolean, hp: Boolean, ammo: Boolean)
    external fun applySkin(defIndex: Int)
}""")

w("app/src/main/java/com/hanto/standoff/OverlayService.kt", """package com.hanto.standoff
import android.app.*
import android.content.Intent
import android.graphics.PixelFormat
import android.os.IBinder
import android.view.Gravity
import android.view.WindowManager

class OverlayService : Service() {
    private lateinit var wm: WindowManager
    private lateinit var esp: EspView
    private lateinit var menu: MenuView
    override fun onBind(i: Intent?): IBinder? = null
    override fun onCreate() {
        super.onCreate()
        val ch = NotificationChannel("hanto","hanto", NotificationManager.IMPORTANCE_LOW)
        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).createNotificationChannel(ch)
        startForeground(1, Notification.Builder(this,"hanto").setContentTitle("hanto v2").build())
        Native.attach(GameLauncher.pid())
        wm = getSystemService(WINDOW_SERVICE) as WindowManager
        esp = EspView(this)
        val p1 = WindowManager.LayoutParams(-1,-1, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN, PixelFormat.TRANSLUCENT)
        p1.gravity = Gravity.TOP or Gravity.START
        wm.addView(esp, p1)
        menu = MenuView(this) { wm.removeView(menu) }
        val p2 = WindowManager.LayoutParams((resources.displayMetrics.widthPixels * 0.9f).toInt(), -2, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, PixelFormat.TRANSLUCENT)
        p2.gravity = Gravity.CENTER
        wm.addView(menu, p2)
    }
    override fun onDestroy() {
        super.onDestroy()
        try { wm.removeView(esp) } catch (_:Exception){}
        try { wm.removeView(menu) } catch (_:Exception){}
        Native.detach()
    }
}""")

w("app/src/main/java/com/hanto/standoff/EspView.kt", """package com.hanto.standoff
import android.content.Context
import android.graphics.*
import android.view.SurfaceHolder
import android.view.SurfaceView

class EspView(ctx: Context) : SurfaceView(ctx), SurfaceHolder.Callback, Runnable {
    private val paint = Paint().apply { style = Paint.Style.STROKE; strokeWidth = 3f; isAntiAlias = true }
    private val fill = Paint().apply { style = Paint.Style.FILL }
    private val txt = Paint().apply { color = Color.WHITE; textSize = 28f; isAntiAlias = true }
    private var running = false
    init { holder.addCallback(this) }
    override fun surfaceCreated(h: SurfaceHolder) { running = true; Thread(this).start() }
    override fun surfaceChanged(h: SurfaceHolder, f: Int, w: Int, hh: Int) {}
    override fun surfaceDestroyed(h: SurfaceHolder) { running = false }
    override fun run() {
        while (running) {
            val c = holder.lockCanvas() ?: continue
            try { draw(c) } finally { holder.unlockCanvasAndPost(c) }
            Thread.sleep(16)
        }
    }
    private fun draw(c: Canvas) {
        c.drawColor(0, PorterDuff.Mode.CLEAR)
        val data = Native.getEntities()
        if (data.isEmpty()) return
        val s = 12; var i = 0
        while (i + s <= data.size) {
            val hx=data[i]; val hy=data[i+1]; val fx=data[i+2]; val fy=data[i+3]
            val hp=data[i+4]; val ammo=data[i+5]; val team=data[i+6].toInt()
            paint.color = if (team==0) Color.rgb(255,80,80) else Color.rgb(80,200,255)
            val bw = (fx-hx)*0.6f
            c.drawRect(hx-bw, hy, fx+bw, fy, paint)
            fill.color = Color.argb(200,0,255,0)
            val hpPx = (fy-hy)*(hp.coerceIn(0f,100f)/100f)
            c.drawRect(hx-bw-6, fy-hpPx, hx-bw-2, fy, fill)
            c.drawText("$ammo", hx, hy-10, txt)
            i += s
        }
    }
}""")

w("app/src/main/java/com/hanto/standoff/MenuView.kt", """package com.hanto.standoff
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.widget.*

object cfgVisual { var box=true; var skeleton=true; var line=true; var chams=false; var hp=true; var ammo=true }
object cfgAim { var silent=true; var visibleOnly=true; var trigger=false; var bone=0 }

class MenuView(ctx: Context, onClose: () -> Unit) : LinearLayout(ctx) {
    private val container = FrameLayout(ctx)
    private val tabVisual = LinearLayout(ctx)
    private val tabAim = LinearLayout(ctx)
    private val tabSkin = LinearLayout(ctx)
    private val skinList = LinearLayout(ctx)
    init {
        orientation = VERTICAL
        background = GradientDrawable().apply { setColor(Color.argb(200,255,255,255)); cornerRadius = 40f; setStroke(2, Color.argb(120,0,0,0)) }
        setPadding(30,30,30,30)
        addView(TextView(ctx).apply { text = "hanto v2"; textSize = 22f; setTextColor(Color.rgb(20,20,30)); gravity = Gravity.CENTER; setPadding(0,10,0,20) })
        val tabs = LinearLayout(ctx).apply { orientation = HORIZONTAL; gravity = Gravity.CENTER }
        tabs.addView(btn(ctx,"VISUAL"){ switchTab(0) })
        tabs.addView(btn(ctx,"AIM"){ switchTab(1) })
        tabs.addView(btn(ctx,"SKIN"){ switchTab(2) })
        addView(tabs); addView(container)
        buildVisual(); buildAim(); buildSkin(); switchTab(0)
        addView(btn(ctx,"CLOSE"){ onClose() })
    }
    private fun btn(ctx: Context, t: String, onClick: () -> Unit) = Button(ctx).apply { text = t; textSize = 14f; setBackgroundColor(Color.argb(60,0,100,255)); setTextColor(Color.rgb(20,20,30)); setOnClickListener { onClick() } }
    private fun switchTab(i: Int) { container.removeAllViews(); when(i){0->container.addView(tabVisual);1->container.addView(tabAim);2->container.addView(tabSkin)} }
    private fun row(ctx: Context, label: String, checked: Boolean, onChange: (Boolean)->Unit) = LinearLayout(ctx).apply {
        orientation = HORIZONTAL; gravity = Gravity.CENTER_VERTICAL; setPadding(0,12,0,12)
        addView(TextView(ctx).apply { text = label; textSize = 16f; setTextColor(Color.rgb(20,20,30)); layoutParams = LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f) })
        addView(Switch(ctx).apply { isChecked = checked; setOnCheckedChangeListener { _, b -> onChange(b) } })
    }
    private fun buildVisual() {
        tabVisual.orientation = VERTICAL
        tabVisual.addView(row(context,"Box",true){cfgVisual.box=it;pushVisual()})
        tabVisual.addView(row(context,"Skeleton",true){cfgVisual.skeleton=it;pushVisual()})
        tabVisual.addView(row(context,"Line",true){cfgVisual.line=it;pushVisual()})
        tabVisual.addView(row(context,"Chams",false){cfgVisual.chams=it;pushVisual()})
        tabVisual.addView(row(context,"HP",true){cfgVisual.hp=it;pushVisual()})
        tabVisual.addView(row(context,"Ammo",true){cfgVisual.ammo=it;pushVisual()})
    }
    private fun buildAim() {
        tabAim.orientation = VERTICAL
        tabAim.addView(row(context,"Silent aim",true){cfgAim.silent=it;pushAim()})
        tabAim.addView(row(context,"Visible only",true){cfgAim.visibleOnly=it;pushAim()})
        tabAim.addView(row(context,"Triggerbot",false){cfgAim.trigger=it;pushAim()})
        tabAim.addView(Spinner(context).apply {
            adapter = ArrayAdapter(context, android.R.layout.simple_spinner_dropdown_item, arrayOf("head","body","arms","legs"))
            setSelection(cfgAim.bone)
            onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) { cfgAim.bone = pos; pushAim() }
                override fun onNothingSelected(p: AdapterView<*>?) {}
            }
        })
    }
    private fun buildSkin() {
        tabSkin.orientation = VERTICAL
        val cats = listOf("Knife" to 0, "AWP" to 1, "AK" to 2)
        val grid = GridLayout(context).apply { columnCount = 2 }
        cats.forEach { (n, id) -> grid.addView(btn(context,n){ showSkins(id) }) }
        tabSkin.addView(grid); skinList.orientation = VERTICAL; tabSkin.addView(skinList)
    }
    private fun showSkins(catId: Int) {
        skinList.removeAllViews()
        val skins = when(catId) { 0 -> listOf("Karambit" to 508, "Butterfly" to 515); 1 -> listOf("Dragon Lore" to 344); 2 -> listOf("Redline" to 282); else -> emptyList() }
        skins.forEach { (n, id) -> skinList.addView(btn(context,n){ Native.applySkin(id) }) }
    }
    private fun pushVisual() = Native.setVisual(cfgVisual.box,cfgVisual.skeleton,cfgVisual.line,cfgVisual.chams,cfgVisual.hp,cfgVisual.ammo)
    private fun pushAim() { Native.setAim(cfgAim.silent,cfgAim.visibleOnly,cfgAim.bone,6f); Native.setTriggerbot(cfgAim.trigger) }
}""")

w("app/src/main/cpp/CMakeLists.txt", """cmake_minimum_required(VERSION 3.22)
project(hanto)
set(CMAKE_CXX_STANDARD 17)
add_library(hanto SHARED main.cpp mem.cpp)
find_library(log-lib log)
target_link_libraries(hanto ${log-lib})""")

w("app/src/main/cpp/mem.h", """#pragma once
#include <cstdint>
class Mem {
public:
    static bool attach(int pid); static void detach(); static bool ok();
    static uintptr_t base(const char* so_name);
    static size_t read(uintptr_t addr, void* out, size_t n);
    static size_t write(uintptr_t addr, const void* in, size_t n);
    template<typename T> static T rd(uintptr_t a) { T v{}; read(a,&v,sizeof(T)); return v; }
    template<typename T> static bool wr(uintptr_t a, T v) { return write(a,&v,sizeof(T))==sizeof(T); }
};""")

w("app/src/main/cpp/mem.cpp", """#include "mem.h"
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"hanto",__VA_ARGS__)
static int g_pid=0, g_fd=-1; static bool g_ok=false;
bool Mem::attach(int pid) {
    if (pid<=0) return false; g_pid=pid;
    char path[64]; snprintf(path,sizeof(path),"/proc/%d/mem",pid);
    g_fd=open(path,O_RDWR); if (g_fd<0){LOG("open mem fail");return false;}
    g_ok=true; return true;
}
void Mem::detach(){ if(g_fd>=0)close(g_fd); g_fd=-1; g_ok=false; g_pid=0; }
bool Mem::ok(){ return g_ok; }
size_t Mem::read(uintptr_t addr, void* out, size_t n){
    if(!g_ok)return 0; struct iovec l{out,n}, r{(void*)addr,n};
    ssize_t res=process_vm_readv(g_pid,&l,1,&r,1,0);
    if(res<=0){ if(pread(g_fd,out,n,(off_t)addr)==(ssize_t)n)return n; return 0; }
    return (size_t)res;
}
size_t Mem::write(uintptr_t addr, const void* in, size_t n){
    if(!g_ok)return 0; struct iovec l{(void*)in,n}, r{(void*)addr,n};
    ssize_t res=process_vm_writev(g_pid,&l,1,&r,1,0);
    if(res<=0){ if(pwrite(g_fd,in,n,(off_t)addr)==(ssize_t)n)return n; return 0; }
    return (size_t)res;
}
uintptr_t Mem::base(const char* so){
    char path[64]; snprintf(path,sizeof(path),"/proc/%d/maps",g_pid);
    FILE* f=fopen(path,"r"); if(!f)return 0; char line[512];
    while(fgets(line,sizeof(line),f)){
        if(strstr(line,so)&&strstr(line,"r-xp")){ uintptr_t a=strtoull(line,nullptr,16); fclose(f); return a; }
    }
    fclose(f); return 0;
}""")

w("app/src/main/cpp/main.cpp", """#include <jni.h>
#include <android/log.h>
#include <atomic>
#include <vector>
#include <cmath>
#include "mem.h"
#include "offsets.hpp"
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"hanto",__VA_ARGS__)
static uintptr_t g_base=0;
static std::atomic<bool> v_box{true},v_skel{true},v_line{true},v_chams{false},v_hp{true},v_ammo{true};
static std::atomic<bool> a_silent{true},a_vis{true},a_trig{false};
static std::atomic<int> a_bone{0};

extern "C" JNIEXPORT jboolean JNICALL Java_com_hanto_standoff_Native_attach(JNIEnv*, jobject, jint pid){
    if(!Mem::attach(pid)) return JNI_FALSE;
    g_base=Mem::base("libunity.so");
    LOG("attach pid=%d base=0x%lx",pid,(unsigned long)g_base);
    return JNI_TRUE;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_detach(JNIEnv*, jobject){ Mem::detach(); }
extern "C" JNIEXPORT jfloatArray JNICALL Java_com_hanto_standoff_Native_getEntities(JNIEnv* env, jobject){
    std::vector<float> out;
    if(Mem::ok() && g_base){
        // заготовка: чтение по off::LOCAL_PLAYER, ENTITY_LIST
    }
    jfloatArray a=env->NewFloatArray(out.size());
    env->SetFloatArrayRegion(a,0,out.size(),out.data());
    return a;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setVisual(JNIEnv*, jobject, jboolean b,jboolean s,jboolean l,jboolean c,jboolean h,jboolean am){
    v_box=b;v_skel=s;v_line=l;v_chams=c;v_hp=h;v_ammo=am;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setAim(JNIEnv*, jobject, jboolean en,jboolean vis,jint bone,jfloat fov){
    a_silent=en;a_vis=vis;a_bone=bone;
}
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_setTriggerbot(JNIEnv*, jobject, jboolean en){ a_trig=en; }
extern "C" JNIEXPORT void JNICALL Java_com_hanto_standoff_Native_applySkin(JNIEnv*, jobject, jint def){ LOG("skin %d",def); }""")

w("app/src/main/cpp/offsets.hpp", "#pragma once\n")
print("Done! All files written cleanly.")
