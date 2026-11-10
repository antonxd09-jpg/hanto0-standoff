// Menu source watermark: @use_sberbank
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "menu/menu.h"
#include "menu/fonts.h"
#include <cstdio>
int main(){
 ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.DisplaySize={1600,1000};io.DeltaTime=1.f/60.f;
 io.BackendFlags|=ImGuiBackendFlags_RendererHasTextures;nl::load_fonts();
 nl::Menu menu;menu.init({80,40},1.f);menu.set_visible(true);
 for(int tab=0;tab<5;++tab){menu.set_tab(tab);
  for(int frame=0;frame<35;++frame){ImGui::NewFrame();menu.render();ImGui::Render();
   for(auto* tex:ImGui::GetPlatformIO().Textures)if(tex->Status==ImTextureStatus_WantCreate||tex->Status==ImTextureStatus_WantUpdates){tex->SetTexID(ImTextureID(1));tex->SetStatus(ImTextureStatus_OK);}
  }
  if(ImGui::GetDrawData()->TotalVtxCount<=0)return 2;
 }
 menu.shutdown();ImGui::DestroyContext();std::puts("PASS: UI-only menu renders all five tabs without game modules");
}
