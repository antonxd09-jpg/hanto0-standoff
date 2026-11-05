"""Run the actual watermark function against headless Dear ImGui (host g++)."""
from pathlib import Path
import subprocess, tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'jni/src/ui/kazakh/menu.hpp').read_text()
def function(name):
    start = source.index('static void '+name+'(')
    body = source.index('{', start)
    level = 1
    end = body + 1
    while level:
        level += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
assert 'static constexpr int g_openStyle = 1;' in source
assert 'kTabs[] = {"visuals", "silent", "config"}' in source
assert 'menu-open-style' not in source
page = function('astraPage')
assert 'aimLeft' not in page and 'silentLeft() : silentRight()' in page and 'antiAimLeft' not in page
for name in ['aimLeft','aimRight','silentLeft','silentRight','antiAimLeft','antiAimRight','commit']:
    assert function(name)
assert 'unload modification' in function('configLeft')
assert 'No unload was performed' in function('configLeft')
cpp = r'''
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <ctime>
#include <cassert>
static bool opened=false;
static bool menuOpen(){return opened;}
static void setMenuOpen(bool v){opened=v;}
'''+function('drawWatermark')+r'''
static void frame(bool down, float x, float y) {
    auto& io=ImGui::GetIO();
    io.AddMousePosEvent(x,y);
    io.AddMouseButtonEvent(0,down);
    ImGui::NewFrame();
    drawWatermark();
    ImGui::Render();
}
int main(){
    ImGui::CreateContext();
    auto& io=ImGui::GetIO();
    io.IniFilename=nullptr; io.DisplaySize=ImVec2(1920,1080); io.DeltaTime=1.f/60;
    unsigned char* pixels; int w,h;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    frame(false,-100,-100);
    auto* win=ImGui::FindWindowByName("##astra-watermark-toggle");
    assert(win && win->Active);
    const float x=win->Pos.x+win->Size.x/2, y=win->Pos.y+win->Size.y/2;
    frame(false,x,y); frame(true,x,y); frame(false,x,y);
    assert(opened);
    frame(true,x,y); frame(false,x,y);
    assert(!opened);
    frame(true,20,500); frame(false,20,500);
    assert(!opened);
    for (auto sz : {ImVec2(1080,1920),ImVec2(1280,720)}) {
        io.DisplaySize=sz;
        frame(false,-100,-100);
        win=ImGui::FindWindowByName("##astra-watermark-toggle");
        assert(win && win->Active && win->Pos.x>=0 && win->Pos.y>=0);
        assert(win->Pos.x+win->Size.x<=sz.x);
    }
    ImGui::DestroyContext();
}
'''
imgui=root/'jni/third_party/imgui'
with tempfile.TemporaryDirectory() as td:
    td=Path(td); (td/'test.cpp').write_text(cpp)
    subprocess.run(['g++','-std=c++17','-O0','-g','-I',str(imgui),str(td/'test.cpp'),*[str(imgui/f) for f in ['imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp']],'-o',str(td/'test')],check=True)
    subprocess.run([str(td/'test')],check=True)
print('PASS: real watermark opens/closes; outside taps; rotation; three tab routing; hidden page functions retained.')
print('LIMIT: unload is unavailable and explicitly reported. No Android build or on-device test performed.')
