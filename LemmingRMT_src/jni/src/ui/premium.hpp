#pragma once
// Included inside main.cpp's anonymous namespace; all widgets stay on the render thread.
struct EspAppearance {
    bool box=true,health=true,armor=true,weapon=true,ammo=true,fill=false,rounded=true;
    int boxStyle=0;
    float lineWidth=1.5f,fillIntensity=0.08f,cornerRadius=6.0f;
    ImVec4 boxColor{0.69f,0.77f,1.0f,1.0f};
    ImVec4 healthColor{0.36f,0.87f,0.65f,1.0f},armorColor{0.43f,0.68f,1.0f,1.0f};
    ImVec4 textColor{0.91f,0.94f,1.0f,1.0f},accent{0.59f,0.68f,1.0f,1.0f};
};
EspAppearance g_appearance;
float g_uiScale=1.0f,g_menuAnimation=0.0f,g_pageAnimation=1.0f;
int g_section=2,g_subsection=0;
bool g_espEditorOpen=false;

float uiEase(float value,float target,float speed=18.0f){return value+(target-value)*(1.0f-std::exp(-speed*std::min(ImGui::GetIO().DeltaTime,0.05f)));}
ImU32 uiColor(ImVec4 c,float alpha=1.0f){c.w*=alpha*ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
ImVec2 uiAdd(ImVec2 a,float x,float y){return ImVec2(a.x+x,a.y+y);}
void uiText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;
    const ImVec2 t=font->CalcTextSizeA(size,FLT_MAX,0,text);
    d->AddText(font,size,ImVec2(center.x-t.x*0.5f,center.y-t.y*0.5f),color,text);
}
void uiTextLeft(ImDrawList* d,ImVec2 p,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;d->AddText(font,size,p,color,text);
}
void uiIcon(ImDrawList* d,int icon,ImVec2 p,float size,ImU32 color){
    const auto& data=lemming::icons::kIcons[std::clamp(icon,0,8)];
    for(int j=0;j<data.count;++j){const auto& line=lemming::icons::kSegments[data.first+j];
        const ImVec2 a(p.x+line.x1*size/24,p.y+line.y1*size/24),b(p.x+line.x2*size/24,p.y+line.y2*size/24);
        d->AddLine(a,b,color,1.65f*size/24);d->AddCircleFilled(a,0.825f*size/24,color,6);
    }
}
void uiPanel(ImDrawList* d,ImVec2 a,ImVec2 b,float rounding=14){
    d->AddRectFilled(uiAdd(a,0,6*g_uiScale),uiAdd(b,0,6*g_uiScale),uiColor(ImVec4(0,0,0,0.23f)),rounding*g_uiScale);
    d->AddRectFilled(a,b,uiColor(ImVec4(0.047f,0.055f,0.075f,0.985f)),rounding*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(0.19f,0.22f,0.29f,0.65f)),rounding*g_uiScale,0,1.0f);
}
void uiCard(ImDrawList* d,ImVec2 a,ImVec2 b){
    d->AddRectFilled(a,b,uiColor(ImVec4(.066f,.078f,.104f,1)),12*g_uiScale);
    d->AddRect(a,b,uiColor(ImVec4(.18f,.21f,.28f,.58f)),12*g_uiScale);
}
bool uiRailButton(const char* label,int icon,bool selected,float width){
    const float h=68*g_uiScale;const ImVec2 a=ImGui::GetCursorScreenPos();
    const bool pressed=ImGui::InvisibleButton(label,ImVec2(width,h));auto* d=ImGui::GetWindowDrawList();
    if(selected)d->AddRectFilled(a,uiAdd(a,width,h),uiColor(ImVec4(0.10f,0.13f,0.21f,0.88f)),11*g_uiScale);
    uiIcon(d,icon,uiAdd(a,(width-27*g_uiScale)*0.5f,9*g_uiScale),26*g_uiScale,uiColor(selected?g_appearance.accent:ImVec4(0.48f,0.54f,0.66f,1)));
    uiText(d,uiAdd(a,width*0.5f,52*g_uiScale),label,uiColor(selected?ImVec4(0.93f,0.95f,1,1):ImVec4(0.54f,0.60f,0.71f,1)),18*g_uiScale,g_uiSmallFont);
    return pressed;
}
bool uiToggle(const char* label,bool& value){
    ImGui::PushID(label);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=48*g_uiScale;
    const bool hit=ImGui::InvisibleButton("##switch",ImVec2(w,h));if(hit)value=!value;
    ImGuiStorage* store=ImGui::GetStateStorage();auto id=ImGui::GetItemID();
    float t=uiEase(store->GetFloat(id,value?1:0),value?1:0,20);store->SetFloat(id,t);
    auto* d=ImGui::GetWindowDrawList();const float sh=26*g_uiScale,sw=48*g_uiScale;
    const ImVec2 p=uiAdd(a,w-sw,0.5f*(h-sh));ImVec4 off(0.15f,0.18f,0.24f,1),on=g_appearance.accent;
    ImVec4 track(off.x+(on.x-off.x)*t,off.y+(on.y-off.y)*t,off.z+(on.z-off.z)*t,1);
    d->AddRectFilled(p,uiAdd(p,sw,sh),uiColor(track),sh*0.5f);
    d->AddCircleFilled(uiAdd(p,sh*0.5f+(sw-sh)*t,sh*0.5f),9.4f*g_uiScale,uiColor(ImVec4(0.97f,0.98f,1,1)),20);
    uiText(d,uiAdd(a,(w-sw-14*g_uiScale)*0.5f,h*0.5f),label,uiColor(ImVec4(0.85f,0.89f,0.96f,1)),19*g_uiScale);
    d->AddLine(uiAdd(a,0,h),uiAdd(a,w,h),uiColor(ImVec4(0.18f,0.20f,0.27f,0.40f)),1);
    ImGui::PopID();return hit;
}
void uiColorRow(const char* label,ImVec4& c){
    ImGui::PushID(label);const ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
    uiText(ImGui::GetWindowDrawList(),uiAdd(p,(w-42*g_uiScale)*0.5f,19*g_uiScale),label,uiColor(ImVec4(0.76f,0.81f,0.90f,1)),18*g_uiScale,g_uiSmallFont);
    ImGui::SetCursorScreenPos(uiAdd(p,w-34*g_uiScale,2*g_uiScale));
    if(ImGui::ColorButton("##color",c,ImGuiColorEditFlags_NoTooltip,ImVec2(32*g_uiScale,32*g_uiScale)))ImGui::OpenPopup("##picker");
    if(ImGui::BeginPopup("##picker")){
        ImGui::SetNextItemWidth(250*g_uiScale);
        ImGui::ColorPicker4("##rgba",&c.x,ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(uiAdd(p,0,44*g_uiScale));ImGui::PopID();
}
bool uiChoiceButton(const char* id,const char* label,bool selected,ImVec2 size){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();const bool pressed=ImGui::InvisibleButton("##choice",size);
    auto* d=ImGui::GetWindowDrawList();const ImVec4 bg=selected?ImVec4(.12f,.16f,.26f,1):ImVec4(.075f,.09f,.122f,1);
    d->AddRectFilled(a,uiAdd(a,size.x,size.y),uiColor(bg),8*g_uiScale);
    d->AddRect(a,uiAdd(a,size.x,size.y),uiColor(selected?g_appearance.accent:ImVec4(.18f,.21f,.28f,.75f),selected?.75f:1),8*g_uiScale);
    uiText(d,uiAdd(a,size.x*.5f,size.y*.5f),label,uiColor(selected?ImVec4(.95f,.97f,1,1):ImVec4(.68f,.73f,.82f,1)),17*g_uiScale,g_uiSmallFont);
    ImGui::PopID();return pressed;
}
void uiSlider(const char* id,const char* label,float& value,float lo,float hi,const char* format){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();
    uiTextLeft(ImGui::GetWindowDrawList(),a,label,uiColor(ImVec4(.68f,.73f,.82f,1)),16*g_uiScale,g_uiSmallFont);
    ImGui::SetCursorScreenPos(uiAdd(a,0,23*g_uiScale));ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::SliderFloat("##value",&value,lo,hi,format);
    ImGui::SetCursorScreenPos(uiAdd(a,0,64*g_uiScale));ImGui::PopID();
}
bool uiActionButton(const char* id,const char* title,const char* subtitle,int icon){
    ImGui::PushID(id);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=98*g_uiScale;
    const bool pressed=ImGui::InvisibleButton("##action",ImVec2(w,h));auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(a,uiAdd(a,w,h),uiColor(ImVec4(.075f,.09f,.122f,1)),12*g_uiScale);
    d->AddRect(a,uiAdd(a,w,h),uiColor(g_appearance.accent,.52f),12*g_uiScale);
    uiIcon(d,icon,uiAdd(a,24*g_uiScale,34*g_uiScale),30*g_uiScale,uiColor(g_appearance.accent));
    uiTextLeft(d,uiAdd(a,72*g_uiScale,23*g_uiScale),title,uiColor(ImVec4(.94f,.96f,1,1)),21*g_uiScale,g_uiTitleFont);
    uiTextLeft(d,uiAdd(a,72*g_uiScale,57*g_uiScale),subtitle,uiColor(ImVec4(.49f,.55f,.66f,1)),16*g_uiScale,g_uiSmallFont);
    uiIcon(d,6,uiAdd(a,w-43*g_uiScale,37*g_uiScale),24*g_uiScale,uiColor(ImVec4(.55f,.61f,.72f,1)));
    ImGui::PopID();return pressed;
}
void uiEspText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size){
    uiText(d,uiAdd(center,0,1),text,IM_COL32(0,0,0,235),size,g_uiSmallFont);uiText(d,center,text,color,size,g_uiSmallFont);
}
void drawStyledEsp(ImDrawList* d,const EspScreenBox& b){
    const ImVec2 a(b.left,b.top),z(b.right,b.bottom);float h=z.y-a.y,w=z.x-a.x;if(h<8||w<2)return;
    const float center=(a.x+z.x)*0.5f,thick=g_appearance.lineWidth;
    const float rounding=g_appearance.rounded?g_appearance.cornerRadius*g_uiScale:0.0f;const auto boxColor=uiColor(g_appearance.boxColor);
    if(g_appearance.box){
        if(g_appearance.fill)d->AddRectFilled(a,z,uiColor(g_appearance.boxColor,std::clamp(g_appearance.fillIntensity,0.0f,0.35f)),rounding);
        if(g_appearance.boxStyle==0){
            d->AddRect(a,z,IM_COL32(0,0,0,170),rounding,0,thick+2);d->AddRect(a,z,boxColor,rounding,0,thick);
        }else{
            float l=std::min(w*0.27f,h*0.15f);const ImVec2 pts[4]={a,ImVec2(z.x,a.y),ImVec2(a.x,z.y),z};
            for(int i=0;i<4;++i){float dx=i%2?-l:l,dy=i>=2?-l:l;
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),IM_COL32(0,0,0,190),thick+2);d->AddLine(pts[i],uiAdd(pts[i],0,dy),IM_COL32(0,0,0,190),thick+2);
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),boxColor,thick);d->AddLine(pts[i],uiAdd(pts[i],0,dy),boxColor,thick);
            }
        }
    }
    char text[48];
    if(g_appearance.health&&b.statsValid){
        const float ratio=b.maxHp?std::clamp(float(b.hp)/b.maxHp,0.0f,1.0f):0;const ImVec2 u(a.x-9,a.y),v(a.x-5,z.y);
        d->AddRectFilled(uiAdd(u,-1,-1),uiAdd(v,1,1),IM_COL32(5,8,14,220),3);d->AddRectFilled(u,v,IM_COL32(35,41,53,220),2);
        ImVec4 c=g_appearance.healthColor,low(1.0f,0.34f,0.40f,1);c=ImVec4(low.x+(c.x-low.x)*ratio,low.y+(c.y-low.y)*ratio,low.z+(c.z-low.z)*ratio,1);
        d->AddRectFilled(ImVec2(u.x,z.y-h*ratio),v,uiColor(c),2);
        if(b.hp<b.maxHp){std::snprintf(text,sizeof(text),"%u",unsigned(b.hp));uiEspText(d,ImVec2(a.x-20,z.y-h*ratio),text,uiColor(c),14*g_uiScale);}
    }
    float labelY=z.y+13*g_uiScale;
    if(g_appearance.armor&&b.statsValid&&b.maxArmor>0){
        float ratio=std::clamp(float(b.armor)/b.maxArmor,0.0f,1.0f);d->AddRectFilled(ImVec2(a.x-1,z.y+5),ImVec2(z.x+1,z.y+10),IM_COL32(5,8,14,210),3);
        if(ratio>0)d->AddRectFilled(ImVec2(a.x,z.y+6),ImVec2(a.x+w*ratio,z.y+9),uiColor(g_appearance.armorColor),2);labelY+=9*g_uiScale;
    }
    if(b.weaponValid&&g_appearance.weapon){std::snprintf(text,sizeof(text),"WEAPON %u",unsigned(b.weaponId));uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor),16*g_uiScale);labelY+=19*g_uiScale;}
    if(b.weaponValid&&g_appearance.ammo){std::snprintf(text,sizeof(text),"%u / %u",unsigned(b.magazineAmmo),unsigned(b.reserveAmmo));uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor,.73f),14*g_uiScale);}
}
void renderEspBoxes(){
    if(!g_espBoxesEnabled.load(std::memory_order_relaxed))return;std::lock_guard<std::mutex> lock(g_espMutex);auto* d=ImGui::GetBackgroundDrawList();
    for(const auto& b:g_espScreenBoxes)drawStyledEsp(d,b);
}
void renderWatermark(){
    auto& io=ImGui::GetIO();const float w=std::round(228*g_uiScale),h=std::round(56*g_uiScale);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x-w)*0.5f,22*g_uiScale),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("##watermark",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
    const bool clicked=ImGui::InvisibleButton("##toggle_menu",ImVec2(w,h));const ImVec2 p=ImGui::GetItemRectMin();auto* d=ImGui::GetWindowDrawList();uiPanel(d,p,uiAdd(p,w,h),15);
    const float tw=ImGui::GetFont()->CalcTextSizeA(21*g_uiScale,FLT_MAX,0,"LemmingRMT").x,total=tw+34*g_uiScale,x=(w-total)*0.5f;
    uiIcon(d,8,uiAdd(p,x,16*g_uiScale),24*g_uiScale,uiColor(g_appearance.accent));uiText(d,uiAdd(p,x+34*g_uiScale+tw*.5f,h*.5f),"LemmingRMT",uiColor(ImVec4(.94f,.96f,1,1)),21*g_uiScale);
    if(clicked){const bool open=!g_menuOpen.load(std::memory_order_relaxed);g_menuOpen.store(open,std::memory_order_relaxed);LOGI("menu watermark toggle: open=%d",open);}
    ImGui::End();ImGui::PopStyleVar();
}
void renderEspEditor(){
    if(!g_espEditorOpen)return;auto& io=ImGui::GetIO();const float s=g_uiScale,w=std::round(980*s),h=std::round(650*s);
    ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x-w)*.5f,(io.DisplaySize.y-h)*.5f),ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(14*s,14*s));ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,14*s);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,7*s));
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.040f,.047f,.064f,.995f));ImGui::PushStyleColor(ImGuiCol_TitleBg,ImVec4(.060f,.072f,.100f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,ImVec4(.060f,.072f,.100f,1));ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed,ImVec4(.060f,.072f,.100f,1));
    auto flags=ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse;
    if(ImGui::Begin("ESP Editor##esp_editor",&g_espEditorOpen,flags)){
        const ImVec2 base=ImGui::GetCursorScreenPos();const ImVec2 avail=ImGui::GetContentRegionAvail();auto* d=ImGui::GetWindowDrawList();
        const float gap=14*s,previewW=294*s,colW=(avail.x-previewW-gap*2)*.5f,cardH=avail.y;
        const ImVec2 geomA=base,geomB=uiAdd(geomA,colW,cardH);const ImVec2 colorA=uiAdd(base,colW+gap,0),colorB=uiAdd(colorA,colW,cardH);
        const ImVec2 prevA=uiAdd(base,(colW+gap)*2,0),prevB=uiAdd(prevA,previewW,cardH);uiCard(d,geomA,geomB);uiCard(d,colorA,colorB);uiCard(d,prevA,prevB);
        uiText(d,ImVec2((geomA.x+geomB.x)*.5f,geomA.y+24*s),"GEOMETRY",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        uiText(d,ImVec2((colorA.x+colorB.x)*.5f,colorA.y+24*s),"ELEMENTS & COLORS",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        uiText(d,ImVec2((prevA.x+prevB.x)*.5f,prevA.y+24*s),"ESP PREVIEW",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        ImGui::SetCursorScreenPos(uiAdd(geomA,14*s,48*s));ImGui::BeginChild("##esp_geometry",ImVec2(colW-28*s,cardH-62*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        const float choiceW=(ImGui::GetContentRegionAvail().x-8*s)*.5f;if(uiChoiceButton("full","Full",g_appearance.boxStyle==0,ImVec2(choiceW,44*s)))g_appearance.boxStyle=0;
        ImGui::SameLine(0,8*s);if(uiChoiceButton("corners","Corners",g_appearance.boxStyle==1,ImVec2(choiceW,44*s)))g_appearance.boxStyle=1;
        uiToggle("Box",g_appearance.box);uiToggle("Rounded",g_appearance.rounded);uiToggle("Soft fill",g_appearance.fill);
        uiSlider("thickness","Thickness",g_appearance.lineWidth,1.0f,4.0f,"%.1f px");
        uiSlider("fill_intensity","Fill intensity",g_appearance.fillIntensity,0.02f,0.30f,"%.2f");
        uiSlider("corner_radius","Corner radius",g_appearance.cornerRadius,0.0f,18.0f,"%.0f px");ImGui::EndChild();
        ImGui::SetCursorScreenPos(uiAdd(colorA,14*s,48*s));ImGui::BeginChild("##esp_colors",ImVec2(colW-28*s,cardH-62*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        uiToggle("Health",g_appearance.health);uiToggle("Armor",g_appearance.armor);uiToggle("Weapon",g_appearance.weapon);uiToggle("Ammo",g_appearance.ammo);
        uiColorRow("Box color",g_appearance.boxColor);uiColorRow("Health color",g_appearance.healthColor);uiColorRow("Armor color",g_appearance.armorColor);
        uiColorRow("Text color",g_appearance.textColor);uiColorRow("Accent",g_appearance.accent);ImGui::EndChild();
        EspScreenBox demo{};demo.left=prevA.x+91*s;demo.right=prevB.x-67*s;demo.top=prevA.y+88*s;demo.bottom=prevB.y-126*s;
        demo.statsValid=demo.weaponValid=demo.enemy=true;demo.hp=76;demo.maxHp=100;demo.armor=64;demo.maxArmor=100;demo.weaponId=12;demo.magazineAmmo=30;demo.reserveAmmo=90;
        const float cx=(demo.left+demo.right)*.5f,cy=demo.top+35*s;const ImU32 mannequin=uiColor(ImVec4(.24f,.29f,.38f,.50f));
        d->AddCircleFilled(ImVec2(cx,cy),13*s,mannequin,24);
        d->AddRectFilled(ImVec2(cx-25*s,cy+18*s),ImVec2(cx+25*s,cy+112*s),mannequin,12*s);
        d->AddRectFilled(ImVec2(cx-39*s,cy+30*s),ImVec2(cx-27*s,cy+105*s),mannequin,6*s);d->AddRectFilled(ImVec2(cx+27*s,cy+30*s),ImVec2(cx+39*s,cy+105*s),mannequin,6*s);
        d->AddRectFilled(ImVec2(cx-22*s,cy+105*s),ImVec2(cx-4*s,cy+190*s),mannequin,8*s);d->AddRectFilled(ImVec2(cx+4*s,cy+105*s),ImVec2(cx+22*s,cy+190*s),mannequin,8*s);drawStyledEsp(d,demo);
        uiText(d,ImVec2((prevA.x+prevB.x)*.5f,prevB.y-28*s),"Drag by the title bar",uiColor(ImVec4(.45f,.51f,.62f,1)),14*s,g_uiSmallFont);
    }
    ImGui::End();ImGui::PopStyleColor(4);ImGui::PopStyleVar(3);
}
#include "ui/pages/aim.hpp"

void uiSectionChange(int section,int sub){if(g_section!=section||g_subsection!=sub){g_section=section;g_subsection=sub;g_pageAnimation=0;}}
void renderMinimalMenu(){
    auto& io=ImGui::GetIO();g_uiScale=std::min({io.DisplaySize.y/1080.0f,io.DisplaySize.x/1440.0f,io.DisplaySize.y/760.0f,1.25f});
    g_uiScale=std::max(0.25f,g_uiScale);io.FontGlobalScale=g_uiScale;renderEspBoxes();renderAngleFov();renderWatermark();lemming::aim::g_menuOpen.store(g_menuOpen.load(),std::memory_order_release);
    const bool open=g_menuOpen.load(std::memory_order_relaxed);g_menuAnimation=uiEase(g_menuAnimation,open?1:0);if(g_menuAnimation<0.005f&&!open)return;
    g_pageAnimation=uiEase(g_pageAnimation,1,15);const float s=g_uiScale,w=std::round(820*s),h=std::round(650*s);
    const float x=std::floor((io.DisplaySize.x-w)*.5f),y=std::floor((io.DisplaySize.y-h)*.5f)+(1-g_menuAnimation)*12*s;
    auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground;
    if(!open)flags|=ImGuiWindowFlags_NoInputs;ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,6*s));
    const char* sectionNames[]={"Rage","Aim","Visuals","Misc","Config","Settings"};const int sectionIcons[]={6,0,1,2,5,7};
    const int sectionCount=6;const float primaryH=(24+sectionCount*74)*s;
    ImGui::SetNextWindowPos(ImVec2(x-176*s,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(156*s,primaryH),ImGuiCond_Always);
    ImGui::Begin("##sections",nullptr,flags);uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),156*s,primaryH/s));
    for(int i=0;i<sectionCount;++i)if(uiRailButton(sectionNames[i],sectionIcons[i],g_section==i,132*s))uiSectionChange(i,0);ImGui::End();
    const char* labels[6][6]={{"General","Targeting","Exploits",nullptr,nullptr},{"Angles","Silent","Targeting","Multipoints","Automation","Prediction"},{"Players","Chams","World","Appearance",nullptr},{"General","Movement","Other",nullptr,nullptr},{"Profiles","Import","Export",nullptr,nullptr},{"Theme","About",nullptr,nullptr,nullptr}};
    const int icons[6][6]={{6,3,2,0,0},{0,0,3,2,0},{3,4,4,5,0},{2,2,7,0,0},{5,6,6,0,0},{5,8,0,0,0}};
    int n=0;while(n<6&&labels[g_section][n])++n;const float secondaryH=(24+n*74)*s;
    ImGui::SetNextWindowPos(ImVec2(x+w+20*s,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(172*s,secondaryH),ImGuiCond_Always);
    ImGui::Begin("##subsections",nullptr,flags);uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),172*s,secondaryH/s));
    for(int i=0;i<n;++i)if(uiRailButton(labels[g_section][i],icons[g_section][i],g_subsection==i,148*s))uiSectionChange(g_section,i);ImGui::End();
    ImGui::SetNextWindowPos(ImVec2(x,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);
    ImGui::Begin("##main",nullptr,flags);auto* d=ImGui::GetWindowDrawList();uiPanel(d,ImVec2(x,y),ImVec2(x+w,y+h),17);
    char title[72];std::snprintf(title,sizeof(title),"%s / %s",sectionNames[g_section],labels[g_section][g_subsection]);
    uiText(d,ImVec2(x+w*.5f,y+40*s),title,uiColor(ImVec4(.94f,.96f,1,1)),29*s,g_uiTitleFont);
    uiText(d,ImVec2(x+w*.5f,y+72*s),g_section==2&&g_subsection==0?"Player overlays and detailed styling.":"LemmingRMT",uiColor(ImVec4(.49f,.55f,.66f,1)),17*s,g_uiSmallFont);
    d->AddLine(ImVec2(x+28*s,y+100*s),ImVec2(x+w-28*s,y+100*s),uiColor(ImVec4(.18f,.21f,.29f,.65f)),1);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation*(0.18f+.82f*g_pageAnimation));const float slide=(1-g_pageAnimation)*10*s;
    if(g_section==1){renderAimControls(x,y,slide);
    }else if(g_section==2&&g_subsection==0){
        const ImVec2 coreA(x+30*s,y+124*s),coreB(x+395*s,y+474*s),detailA(x+425*s,y+124*s),detailB(x+790*s,y+474*s);uiCard(d,coreA,coreB);uiCard(d,detailA,detailB);
        uiText(d,ImVec2((coreA.x+coreB.x)*.5f,coreA.y+25*s),"ESP CORE",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        uiText(d,ImVec2((detailA.x+detailB.x)*.5f,detailA.y+25*s),"OVERLAYS",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        ImGui::SetCursorPos(ImVec2(47*s,165*s+slide));ImGui::BeginChild("##core_controls",ImVec2(331*s,286*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        bool enabled=g_espBoxesEnabled.load();if(uiToggle("Enemy ESP",enabled)){g_espBoxesEnabled.store(enabled);LOGI("ESP appearance master: enabled=%d",enabled);}
        uiToggle("Box",g_appearance.box);uiToggle("Soft fill",g_appearance.fill);uiToggle("Rounded",g_appearance.rounded);ImGui::EndChild();
        ImGui::SetCursorPos(ImVec2(442*s,165*s+slide));ImGui::BeginChild("##detail_controls",ImVec2(331*s,286*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        uiToggle("Health",g_appearance.health);uiToggle("Armor",g_appearance.armor);uiToggle("Weapon",g_appearance.weapon);uiToggle("Ammo",g_appearance.ammo);ImGui::EndChild();
        ImGui::SetCursorPos(ImVec2(30*s,500*s+slide));if(uiActionButton("esp_editor","Open ESP Editor","Style, thickness, fill, rounding, colors and preview",5))g_espEditorOpen=true;
    }else if(g_section==2&&g_subsection==3){
        ImGui::SetCursorPos(ImVec2(96*s,190*s+slide));ImGui::BeginChild("##appearance_action",ImVec2(628*s,180*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        if(uiActionButton("appearance_editor","Open ESP Editor","Move the panel and tune every visual detail",5))g_espEditorOpen=true;ImGui::EndChild();
        uiText(d,ImVec2(x+w*.5f,y+420*s),"The editor contains the live ESP preview and all appearance controls.",uiColor(ImVec4(.50f,.56f,.67f,1)),17*s,g_uiSmallFont);
    }else if(g_section==5&&g_subsection==0){
        ImGui::SetCursorPos(ImVec2(116*s,155*s+slide));ImGui::BeginChild("##theme",ImVec2(588*s,350*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        uiColorRow("Accent",g_appearance.accent);uiColorRow("Panel text",g_appearance.textColor);ImGui::EndChild();
    }else if(g_section==5&&g_subsection==1){
        uiIcon(d,8,ImVec2(x+w*.5f-25*s,y+224*s),50*s,uiColor(g_appearance.accent));uiText(d,ImVec2(x+w*.5f,y+326*s),"LemmingRMT",uiColor(ImVec4(.94f,.96f,1,1)),34*s,g_uiTitleFont);
        uiText(d,ImVec2(x+w*.5f,y+371*s),"ARM64 / GLES3 overlay",uiColor(ImVec4(.54f,.61f,.72f,1)),19*s,g_uiSmallFont);
    }else{
        uiIcon(d,icons[g_section][g_subsection],ImVec2(x+w*.5f-25*s,y+225*s),50*s,uiColor(g_appearance.accent,.7f));uiText(d,ImVec2(x+w*.5f,y+326*s),"Coming later",uiColor(ImVec4(.78f,.83f,.92f,1)),29*s,g_uiTitleFont);
        uiText(d,ImVec2(x+w*.5f,y+368*s),"This tab is ready for future controls.",uiColor(ImVec4(.47f,.54f,.66f,1)),18*s,g_uiSmallFont);
    }
    ImGui::PopStyleVar();ImGui::End();ImGui::PopStyleVar(3);if(open&&g_espEditorOpen)renderEspEditor();
}
