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
#include <cmath>
#include <vector>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
#define LOGI(...) ((void)0)
using EspScreenBox=lemming::esp::ScreenBox;
std::atomic<bool> g_menuOpen{true},g_espBoxesEnabled{true};
std::mutex g_espMutex;std::vector<EspScreenBox> g_espScreenBoxes;
ImFont* g_uiTitleFont=nullptr;ImFont* g_uiSmallFont=nullptr;
#include "ui_premium.hpp"
static bool graphics=false;
static void configureStyle(){
 auto& style=ImGui::GetStyle();style.WindowPadding=ImVec2(18,16);style.FramePadding=ImVec2(12,8);style.ItemSpacing=ImVec2(10,12);style.ItemInnerSpacing=ImVec2(8,6);
 style.WindowRounding=15;style.ChildRounding=11;style.FrameRounding=9;style.PopupRounding=11;style.ScrollbarRounding=9;style.GrabRounding=9;style.WindowBorderSize=1;style.FrameBorderSize=0;
 auto* c=style.Colors;c[ImGuiCol_Text]=ImVec4(.93f,.95f,.98f,1);c[ImGuiCol_TextDisabled]=ImVec4(.46f,.51f,.60f,1);c[ImGuiCol_WindowBg]=ImVec4(.035f,.043f,.059f,.98f);c[ImGuiCol_Border]=ImVec4(.16f,.19f,.25f,.85f);
 c[ImGuiCol_FrameBg]=ImVec4(.075f,.090f,.122f,1);c[ImGuiCol_FrameBgHovered]=c[ImGuiCol_FrameBg];c[ImGuiCol_FrameBgActive]=c[ImGuiCol_FrameBg];c[ImGuiCol_Button]=c[ImGuiCol_FrameBg];c[ImGuiCol_ButtonHovered]=c[ImGuiCol_Button];c[ImGuiCol_ButtonActive]=c[ImGuiCol_Button];
 c[ImGuiCol_CheckMark]=ImVec4(.35f,.68f,1,1);c[ImGuiCol_SliderGrab]=ImVec4(.35f,.68f,1,1);c[ImGuiCol_SliderGrabActive]=c[ImGuiCol_SliderGrab];c[ImGuiCol_Header]=ImVec4(.12f,.16f,.22f,1);c[ImGuiCol_HeaderHovered]=c[ImGuiCol_Header];c[ImGuiCol_HeaderActive]=c[ImGuiCol_Header];c[ImGuiCol_Separator]=ImVec4(.14f,.17f,.23f,1);
}
static void frame(){ImGui::GetIO().DeltaTime=1.0f/60;ImGui::NewFrame();renderMinimalMenu();ImGui::Render();
 const auto* dd=ImGui::GetDrawData();assert(dd);for(int i=0;i<dd->CmdListsCount;++i)for(const auto& v:dd->CmdLists[i]->VtxBuffer)assert(std::isfinite(v.pos.x)&&std::isfinite(v.pos.y));
 if(graphics){auto size=ImGui::GetIO().DisplaySize;glViewport(0,0,int(size.x),int(size.y));glClearColor(.018,.025,.037,1);glClear(GL_COLOR_BUFFER_BIT);ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());assert(glGetError()==GL_NO_ERROR);}
}
static void click(float x,float y){auto& io=ImGui::GetIO();io.AddMousePosEvent(x,y);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMouseButtonEvent(0,false);frame();frame();}
static void drag(float x1,float y1,float x2,float y2){auto& io=ImGui::GetIO();io.AddMousePosEvent(x1,y1);frame();io.AddMouseButtonEvent(0,true);frame();io.AddMousePosEvent(x2,y2);frame();frame();io.AddMouseButtonEvent(0,false);frame();}
static void settle(){for(int i=0;i<45;++i)frame();}
static ImGuiWindow* containing(const char* needle){for(auto* w:ImGui::GetCurrentContext()->Windows)if(std::strstr(w->Name,needle))return w;return nullptr;}
static void checkLayout(){
 for(const char* id:{"##watermark","##main","##sections","##subsections"}){auto* w=ImGui::FindWindowByName(id);assert(w);const auto size=ImGui::GetIO().DisplaySize;assert(w->Pos.x>=-1&&w->Pos.y>=-1&&w->Pos.x+w->Size.x<=size.x+1&&w->Pos.y+w->Size.y<=size.y+1);assert(w->Flags&ImGuiWindowFlags_NoTitleBar);assert(w->Flags&ImGuiWindowFlags_NoMove);}
 auto* main=ImGui::FindWindowByName("##main");auto* primary=ImGui::FindWindowByName("##sections");auto* secondary=ImGui::FindWindowByName("##subsections");
 std::cerr<<"LAYOUT viewport="<<ImGui::GetIO().DisplaySize.x<<","<<ImGui::GetIO().DisplaySize.y<<" main="<<main->Pos.x<<","<<main->Pos.y<<" size="<<main->Size.x<<","<<main->Size.y<<" scale="<<g_uiScale<<"\n";
 assert(std::fabs(main->Pos.x+main->Size.x/2-ImGui::GetIO().DisplaySize.x/2)<1);assert(primary->Pos.x+primary->Size.x<=main->Pos.x+1);assert(secondary->Pos.x>=main->Pos.x+main->Size.x-1);
 if(g_uiScale>.99f){assert(main->Size.x>=819&&main->Size.y>=649);assert(primary->Size.x>=155&&secondary->Size.x>=171);}
}
static void checkNoScrollbars(bool editor){
 const char* base[]={"##core_controls","##detail_controls"};for(auto* name:base){auto* w=containing(name);assert(w);assert(w->Flags&ImGuiWindowFlags_NoScrollbar);assert(w->ScrollMax.y<=1);}
 if(editor)for(const char* name:{"##esp_geometry","##esp_colors"}){auto* w=containing(name);assert(w);assert(w->Flags&ImGuiWindowFlags_NoScrollbar);assert(w->ScrollMax.y<=1);}
}
static void writePpm(const char* path){auto size=ImGui::GetIO().DisplaySize;const int w=int(size.x),h=int(size.y);std::vector<unsigned char> pixels(w*h*4);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());assert(glGetError()==GL_NO_ERROR);std::ofstream f(path,std::ios::binary);f<<"P6\n"<<w<<" "<<h<<"\n255\n";for(int row=h-1;row>=0;--row)for(int col=0;col<w;++col)f.write(reinterpret_cast<char*>(pixels.data()+(row*w+col)*4),3);}
int main(int argc,char** argv){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.ConfigWindowsMoveFromTitleBarOnly=true;io.DisplaySize=ImVec2(2340,1080);configureStyle();
 ImFontConfig cfg{};cfg.FontDataOwnedByAtlas=false;auto data=const_cast<std::uint8_t*>(lemming::fonts::kMozillaText);auto bytes=int(lemming::fonts::kMozillaTextSize);
 io.Fonts->AddFontFromMemoryTTF(data,bytes,21,&cfg);g_uiTitleFont=io.Fonts->AddFontFromMemoryTTF(data,bytes,27,&cfg);g_uiSmallFont=io.Fonts->AddFontFromMemoryTTF(data,bytes,17,&cfg);unsigned char* atlas;int aw,ah;io.Fonts->GetTexDataAsRGBA32(&atlas,&aw,&ah);
 settle();checkLayout();auto* main=ImGui::FindWindowByName("##main");auto* primary=ImGui::FindWindowByName("##sections");auto* secondary=ImGui::FindWindowByName("##subsections");const float x=main->Pos.x,y=main->Pos.y,primaryX=primary->Pos.x+primary->Size.x*.5f,secondaryX=secondary->Pos.x+secondary->Size.x*.5f;
 checkNoScrollbars(false);auto* watermark=ImGui::FindWindowByName("##watermark");click(watermark->Pos.x+watermark->Size.x*.5f,watermark->Pos.y+watermark->Size.y*.5f);assert(!g_menuOpen);settle();click(watermark->Pos.x+watermark->Size.x*.5f,watermark->Pos.y+watermark->Size.y*.5f);assert(g_menuOpen);settle();
 click(x+190,y+189);assert(!g_espBoxesEnabled);click(x+190,y+189);assert(g_espBoxesEnabled);
 click(primaryX,y+46);assert(g_section==0&&g_subsection==0);click(primaryX,y+46+4*74);assert(g_section==4&&g_subsection==0);click(primaryX,y+46+2*74);assert(g_section==2&&g_subsection==0);click(secondaryX,y+46+3*74);assert(g_subsection==3);click(secondaryX,y+46);assert(g_subsection==0);settle();
 click(x+410,y+549);settle();assert(g_espEditorOpen);auto* editor=ImGui::FindWindowByName("ESP Editor##esp_editor");assert(editor);assert(editor->Size.x>=979&&editor->Size.y>=649);assert(!(editor->Flags&ImGuiWindowFlags_NoMove));assert(editor->Flags&ImGuiWindowFlags_NoResize);assert(!(editor->Flags&ImGuiWindowFlags_NoTitleBar));checkNoScrollbars(true);
 auto before=editor->Pos;drag(before.x+editor->Size.x*.5f,before.y+10,before.x+editor->Size.x*.5f+64,before.y+50);settle();editor=ImGui::FindWindowByName("ESP Editor##esp_editor");assert(editor->Pos.x>before.x+30&&editor->Pos.y>before.y+15);
 auto* geom=containing("##esp_geometry");assert(geom);click(geom->Pos.x+geom->Size.x*.75f,geom->Pos.y+22);assert(g_appearance.boxStyle==1);click(geom->Pos.x+geom->Size.x*.25f,geom->Pos.y+22);assert(g_appearance.boxStyle==0);
 g_espEditorOpen=false;settle();for(const ImVec2 size:{ImVec2(1920,1080),ImVec2(1600,900),ImVec2(1280,720),ImVec2(960,540),ImVec2(1080,2340)}){io.DisplaySize=size;settle();checkLayout();}
 io.DisplaySize=ImVec2(2340,1080);settle();setenv("EGL_PLATFORM","surfaceless",1);EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor;
 if(display!=EGL_NO_DISPLAY&&eglInitialize(display,&major,&minor)){assert(eglBindAPI(EGL_OPENGL_ES_API));EGLint attr[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};EGLConfig config;EGLint n;assert(eglChooseConfig(display,attr,&config,1,&n)&&n>0);EGLint surfaceAttr[]={EGL_WIDTH,2340,EGL_HEIGHT,1080,EGL_NONE};auto surf=eglCreatePbufferSurface(display,config,surfaceAttr);assert(surf!=EGL_NO_SURFACE);EGLint ctxAttr[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};auto ctx=eglCreateContext(display,config,EGL_NO_CONTEXT,ctxAttr);assert(ctx!=EGL_NO_CONTEXT);assert(eglMakeCurrent(display,surf,surf,ctx));assert(ImGui_ImplOpenGL3_Init("#version 300 es"));assert(ImGui_ImplOpenGL3_CreateDeviceObjects());graphics=true;
  g_espEditorOpen=false;settle();frame();if(argc>1)writePpm(argv[1]);g_espEditorOpen=true;settle();editor=ImGui::FindWindowByName("ESP Editor##esp_editor");assert(editor);ImGui::SetWindowPos(editor,ImVec2(1320,205),ImGuiCond_Always);frame();if(argc>2)writePpm(argv[2]);
  ImGui_ImplOpenGL3_Shutdown();eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(display,ctx);eglDestroySurface(display,surf);eglTerminate(display);std::cout<<"PASS GLES3 offscreen rendering and GL_NO_ERROR\n";
 }else std::cout<<"SKIP GLES3 screenshot: headless EGL unavailable\n";
 ImGui::DestroyContext();std::cout<<"PASS larger menu and rails, Rage/Config navigation, movable ESP Editor, style switching, no scrollbars, finite vertices and six viewport sizes\n";
}
