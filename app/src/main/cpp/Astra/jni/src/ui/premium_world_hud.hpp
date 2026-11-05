#pragma once
// GPU Night only. World/Fog/HUD controls remain absent.
static void worldHudControls(){
 if(!astra_sections::header("GPU Night mode",width))return;
 ImGui::PushID("gpu-night-recovery");
 astra_night_diag::controls();
 ImGui::PopID();
}
