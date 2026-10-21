#include "esp_runtime.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "fonts/mozilla_text.hpp"
#include "lucide_vectors.hpp"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cassert>
#include <cfloat>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <cstring>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
#define LOGI(...) ((void)0)
using EspScreenBox=lemming::esp::ScreenBox;
std::atomic<bool> g_menuOpen{true},g_espBoxesEnabled{true};
std::mutex g_espMutex;std::vector<EspScreenBox> g_espScreenBoxes;
ImFont* g_uiTitleFont=nullptr;ImFont* g_uiSmallFont=nullptr;
#include "ui_premium.hpp"
static bool graphics=false;
static void frame(){ImGui::GetIO().DeltaTime=1.0f/60;ImGui::NewFrame();renderMinimalMenu();ImGui::Render();
 const auto* dd=ImGui::GetDrawData();assert(dd);
 for(int i=0;i<dd->CmdListsCount;++i)for(const auto& v:dd->CmdLists[i]->VtxBuffer){assert(std::isfinite(v.pos.x)&&std::isfinite(v.pos.y));}
 if(graphics){auto size=ImGui::GetIO().DisplaySize;glViewport(0,0,int(size.x),int(size.y));glClearColor(.018,.025,.037,1);glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());assert(glGetError()==GL_NO_ERROR);}
}
static void click(float x,float y){auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();}
static void settle(){for(int i=0;i<45;++i)frame();}
static void checkLayout(){for(const char* id:{"##watermark","##main","##sections","##subsections"}){
 auto* w=ImGui::FindWindowByName(id);assert(w);const auto size=ImGui::GetIO().DisplaySize;
 assert(w->Pos.x>=0&&w->Pos.y>=0&&w->Pos.x+w->Size.x<=size.x+1&&w->Pos.y+w->Size.y<=size.y+1);
 assert(w->Flags&ImGuiWindowFlags_NoTitleBar);assert(w->Flags&ImGuiWindowFlags_NoMove);
 }
 auto* w=ImGui::FindWindowByName("##main");auto* primary=ImGui::FindWindowByName("##sections");auto* secondary=ImGui::FindWindowByName("##subsections");
 std::cerr<<"LAYOUT viewport="<<ImGui::GetIO().DisplaySize.x<<","<<ImGui::GetIO().DisplaySize.y<<" main="<<w->Pos.x<<","<<w->Pos.y<<" size="<<w->Size.x<<","<<w->Size.y<<" scale="<<g_uiScale<<"\n";
 assert(std::fabs(w->Pos.x+w->Size.x/2-ImGui::GetIO().DisplaySize.x/2)<1);
 assert(primary->Pos.x+primary->Size.x<=w->Pos.x+1);assert(secondary->Pos.x>=w->Pos.x+w->Size.x-1);
}
static void checkNoScrollbars(){
 int found=0;for(auto* w:ImGui::GetCurrentContext()->Windows){
  if(std::strstr(w->Name,"##core_controls")||std::strstr(w->Name,"##detail_controls")||std::strstr(w->Name,"##style_controls")){
   assert(w->Flags&ImGuiWindowFlags_NoScrollbar);assert(w->ScrollMax.y<=1.0f);++found;
  }
 }
 assert(found==3);
}
int main(int argc,char** argv){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize=ImVec2(2340,1080);
 ImFontConfig cfg{};cfg.FontDataOwnedByAtlas=false;
 auto data=const_cast<std::uint8_t*>(lemming::fonts::kMozillaText);auto bytes=int(lemming::fonts::kMozillaTextSize);
 io.Fonts->AddFontFromMemoryTTF(data,bytes,21,&cfg);g_uiTitleFont=io.Fonts->AddFontFromMemoryTTF(data,bytes,27,&cfg);g_uiSmallFont=io.Fonts->AddFontFromMemoryTTF(data,bytes,17,&cfg);
 unsigned char* atlas;int aw,ah;io.Fonts->GetTexDataAsRGBA32(&atlas,&aw,&ah);
 settle();checkLayout();
 auto* main=ImGui::FindWindowByName("##main");auto* primary=ImGui::FindWindowByName("##sections");auto* secondary=ImGui::FindWindowByName("##subsections");
 const float x=main->Pos.x,y=main->Pos.y,primaryX=primary->Pos.x+66,secondaryX=secondary->Pos.x+72;
 checkNoScrollbars();
 click(1170,47);assert(!g_menuOpen);settle();click(1170,47);assert(g_menuOpen);settle();
 click(x+180,y+170);assert(!g_espBoxesEnabled);click(x+180,y+170);assert(g_espBoxesEnabled);
 click(x+180,y+268);assert(g_appearance.skeleton);assert(lemming::esp::detail::g_skeletonEnabled);
 click(x+180,y+268);assert(!g_appearance.skeleton);
 click(primaryX,y+41);assert(g_section==0);click(secondaryX,y+105);assert(g_subsection==1);settle();
 assert(g_espBoxesEnabled&&!g_appearance.skeleton);
 click(primaryX,y+105);assert(g_section==1&&g_subsection==0);
 click(secondaryX,y+233);assert(g_subsection==3);settle();
 click(x+614,y+131);
 bool popup=false;for(auto* w:ImGui::GetCurrentContext()->Windows)if(w->Active&&(w->Flags&ImGuiWindowFlags_Popup))popup=true;assert(popup);
 click(2200,900);click(secondaryX,y+41);assert(g_subsection==0);settle();checkNoScrollbars();
 for(const ImVec2 size:{ImVec2(1920,1080),ImVec2(1600,900),ImVec2(1280,720),ImVec2(960,540),ImVec2(1080,2340)}){io.DisplaySize=size;settle();checkLayout();}
 io.DisplaySize=ImVec2(2340,1080);settle();
 setenv("EGL_PLATFORM","surfaceless",1);EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor;
 if(display!=EGL_NO_DISPLAY&&eglInitialize(display,&major,&minor)){
  assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig config;EGLint n;
  assert(eglChooseConfig(display,attr,&config,1,&n)&&n>0);EGLint surfaceAttr[]={EGL_WIDTH,2340,EGL_HEIGHT,1080,EGL_NONE};auto surf=eglCreatePbufferSurface(display,config,surfaceAttr);assert(surf!=EGL_NO_SURFACE);
  EGLint ctxAttr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto ctx=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxAttr);assert(ctx!=EGL_NO_CONTEXT);assert(eglMakeCurrent(display,surf,surf,ctx));
  assert(ImGui_ImplOpenGL3_Init("#version 300 es"));assert(ImGui_ImplOpenGL3_CreateDeviceObjects());graphics=true;frame();
  std::vector<unsigned char> pixels(2340*1080*4);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,2340,1080,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);
  if(argc>1){std::ofstream f(argv[1],std::ios::binary);f<<"P6\n2340 1080\n255\n";for(int row=1079;row>=0;--row)for(int col=0;col<2340;++col)f.write(reinterpret_cast<char*>(pixels.data()+(row*2340+col)*4),3);}
  ImGui_ImplOpenGL3_Shutdown();eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,ctx);eglDestroySurface(display,surf);eglTerminate(display);
  std::cout<<"PASS GLES3 offscreen rendering and GL_NO_ERROR\n";
 }else std::cout<<"SKIP GLES3 screenshot: headless EGL unavailable\n";
 ImGui::DestroyContext();std::cout<<"PASS watermark-only open/close, separate ESP switches, tab/subtab transitions, actual RGBA popup, finite draw vertices and centered/clipped layout at six viewport sizes\n";
}
