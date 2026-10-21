#pragma once
// Included inside main.cpp's anonymous namespace; all widgets stay on the render thread.
struct EspAppearance {
    bool box=true,health=true,armor=true,weapon=true,ammo=true,skeleton=false,fill=false;
    int boxStyle=0;
    float lineWidth=1.5f;
    ImVec4 boxColor{0.69f,0.77f,1.0f,1.0f}, skeletonColor{0.91f,0.94f,1.0f,1.0f};
    ImVec4 healthColor{0.36f,0.87f,0.65f,1.0f},armorColor{0.43f,0.68f,1.0f,1.0f};
    ImVec4 textColor{0.91f,0.94f,1.0f,1.0f},accent{0.59f,0.68f,1.0f,1.0f};
};
EspAppearance g_appearance;
float g_uiScale=1.0f,g_menuAnimation=0.0f,g_pageAnimation=1.0f;
int g_section=1,g_subsection=0;
float uiEase(float value,float target,float speed=18.0f){return value+(target-value)*(1.0f-std::exp(-speed*std::min(ImGui::GetIO().DeltaTime,0.05f)));}
ImU32 uiColor(ImVec4 c,float alpha=1.0f){c.w*=alpha*ImGui::GetStyle().Alpha;return ImGui::ColorConvertFloat4ToU32(c);}
ImVec2 uiAdd(ImVec2 a,float x,float y){return ImVec2(a.x+x,a.y+y);}
void uiText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size=0,ImFont* font=nullptr){
    if(!font)font=ImGui::GetFont();if(size<=0)size=21*g_uiScale;
    const ImVec2 t=font->CalcTextSizeA(size,FLT_MAX,0,text);
    d->AddText(font,size,ImVec2(center.x-t.x*0.5f,center.y-t.y*0.5f),color,text);
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
bool uiRailButton(const char* label,int icon,bool selected,float width){
    const float h=58*g_uiScale;const ImVec2 a=ImGui::GetCursorScreenPos();
    const bool pressed=ImGui::InvisibleButton(label,ImVec2(width,h));
    ImGuiStorage* store=ImGui::GetStateStorage();const ImGuiID id=ImGui::GetItemID();
    float t=uiEase(store->GetFloat(id,selected?1:0),selected?1:0);store->SetFloat(id,t);
    auto* d=ImGui::GetWindowDrawList();
    d->AddRectFilled(a,uiAdd(a,width,h),uiColor(ImVec4(0.10f,0.13f,0.21f,0.85f),t),10*g_uiScale);
    if(ImGui::IsItemHovered())d->AddRectFilled(a,uiAdd(a,width,h),uiColor(ImVec4(1,1,1,0.035f)),10*g_uiScale);
    uiIcon(d,icon,uiAdd(a,(width-23*g_uiScale)*0.5f,8*g_uiScale),22*g_uiScale,uiColor(selected?g_appearance.accent:ImVec4(0.48f,0.54f,0.66f,1)));
    uiText(d,uiAdd(a,width*0.5f,43*g_uiScale),label,uiColor(selected?ImVec4(0.93f,0.95f,1,1):ImVec4(0.54f,0.60f,0.71f,1)),16*g_uiScale,g_uiSmallFont);
    return pressed;
}
bool uiToggle(const char* label,bool& value){
    ImGui::PushID(label);const ImVec2 a=ImGui::GetCursorScreenPos();const float w=ImGui::GetContentRegionAvail().x,h=43*g_uiScale;
    const bool hit=ImGui::InvisibleButton("##switch",ImVec2(w,h));if(hit)value=!value;
    ImGuiStorage* store=ImGui::GetStateStorage();auto id=ImGui::GetItemID();
    float t=uiEase(store->GetFloat(id,value?1:0),value?1:0,20);store->SetFloat(id,t);
    auto* d=ImGui::GetWindowDrawList();const float sh=24*g_uiScale,sw=44*g_uiScale;
    const ImVec2 p=uiAdd(a,w-sw,0.5f*(h-sh));
    ImVec4 off(0.15f,0.18f,0.24f,1),on=g_appearance.accent;
    ImVec4 track(off.x+(on.x-off.x)*t,off.y+(on.y-off.y)*t,off.z+(on.z-off.z)*t,1);
    d->AddRectFilled(p,uiAdd(p,sw,sh),uiColor(track),sh*0.5f);
    d->AddCircleFilled(uiAdd(p,sh*0.5f+(sw-sh)*t,sh*0.5f),8.6f*g_uiScale,uiColor(ImVec4(0.97f,0.98f,1,1)),20);
    uiText(d,uiAdd(a,(w-sw-12*g_uiScale)*0.5f,h*0.5f),label,uiColor(ImVec4(0.85f,0.89f,0.96f,1)),18*g_uiScale);
    d->AddLine(uiAdd(a,0,h),uiAdd(a,w,h),uiColor(ImVec4(0.18f,0.20f,0.27f,0.40f)),1);
    ImGui::PopID();return hit;
}
void uiColorRow(const char* label,ImVec4& c){
    ImGui::PushID(label);const ImVec2 p=ImGui::GetCursorScreenPos();float w=ImGui::GetContentRegionAvail().x;
    uiText(ImGui::GetWindowDrawList(),uiAdd(p,(w-40*g_uiScale)*0.5f,17*g_uiScale),label,uiColor(ImVec4(0.76f,0.81f,0.90f,1)),17*g_uiScale,g_uiSmallFont);
    ImGui::SetCursorScreenPos(uiAdd(p,w-32*g_uiScale,0));
    if(ImGui::ColorButton("##color",c,ImGuiColorEditFlags_NoTooltip,ImVec2(30*g_uiScale,30*g_uiScale)))ImGui::OpenPopup("##picker");
    if(ImGui::BeginPopup("##picker")){
        ImGui::SetNextItemWidth(230*g_uiScale);
        ImGui::ColorPicker4("##rgba",&c.x,ImGuiColorEditFlags_AlphaBar|ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_NoSidePreview);
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(uiAdd(p,0,39*g_uiScale));ImGui::PopID();
}
void uiEspText(ImDrawList* d,ImVec2 center,const char* text,ImU32 color,float size){
    uiText(d,uiAdd(center,0,1),text,IM_COL32(0,0,0,235),size,g_uiSmallFont);
    uiText(d,center,text,color,size,g_uiSmallFont);
}
void drawStyledEsp(ImDrawList* d,const EspScreenBox& b,bool preview=false){
    const ImVec2 a(b.left,b.top),z(b.right,b.bottom);float h=z.y-a.y,w=z.x-a.x;
    if(h<8||w<2)return;const float center=(a.x+z.x)*0.5f;
    const float thick=g_appearance.lineWidth;const auto boxColor=uiColor(g_appearance.boxColor);
    if(g_appearance.box){
        if(g_appearance.fill)d->AddRectFilled(a,z,uiColor(g_appearance.boxColor,0.055f),3);
        if(g_appearance.boxStyle==0){
            d->AddRect(a,z,IM_COL32(0,0,0,170),3,0,thick+2);d->AddRect(a,z,boxColor,3,0,thick);
        }else{
            float l=std::min(w*0.27f,h*0.15f);const ImVec2 pts[4]={a,ImVec2(z.x,a.y),ImVec2(a.x,z.y),z};
            for(int i=0;i<4;++i){float dx=i%2?-l:l,dy=i>=2?-l:l;
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),IM_COL32(0,0,0,190),thick+2);d->AddLine(pts[i],uiAdd(pts[i],0,dy),IM_COL32(0,0,0,190),thick+2);
                d->AddLine(pts[i],uiAdd(pts[i],dx,0),boxColor,thick);d->AddLine(pts[i],uiAdd(pts[i],0,dy),boxColor,thick);
            }
        }
    }
    if(g_appearance.skeleton&&!preview){
        for(const auto& link:lemming::esp::detail::kSkeletonLinks){
            const int i=link[0],j=link[1];if(!(b.skeletonMask&(1u<<i))||!(b.skeletonMask&(1u<<j)))continue;
            ImVec2 u(b.skeleton[i].x,b.skeleton[i].y),v(b.skeleton[j].x,b.skeleton[j].y);
            d->AddLine(u,v,IM_COL32(0,0,0,155),thick+1.8f);d->AddLine(u,v,uiColor(g_appearance.skeletonColor),thick);
        }
        if((b.skeletonMask&(1u<<18))&&(b.skeletonMask&(1u<<19))){
            d->AddLine(ImVec2(b.skeleton[18].x,b.skeleton[18].y),ImVec2(b.skeleton[19].x,b.skeleton[19].y),uiColor(g_appearance.skeletonColor),thick);
        }
    }
    char text[48];
    if(g_appearance.health&&b.statsValid){
        const float ratio=b.maxHp?std::clamp(float(b.hp)/b.maxHp,0.0f,1.0f):0;
        const ImVec2 u(a.x-8,a.y),v(a.x-4,z.y);d->AddRectFilled(uiAdd(u,-1,-1),uiAdd(v,1,1),IM_COL32(5,8,14,220),3);
        d->AddRectFilled(u,v,IM_COL32(35,41,53,220),2);
        ImVec4 c=g_appearance.healthColor;
        const ImVec4 low(1.0f,0.34f,0.40f,1);c=ImVec4(low.x+(c.x-low.x)*ratio,low.y+(c.y-low.y)*ratio,low.z+(c.z-low.z)*ratio,1);
        d->AddRectFilled(ImVec2(u.x,z.y-h*ratio),v,uiColor(c),2);
        if(b.hp<b.maxHp){std::snprintf(text,sizeof(text),"%u",unsigned(b.hp));uiEspText(d,ImVec2(a.x-19,z.y-h*ratio),text,uiColor(c),14*g_uiScale);}
    }
    float labelY=z.y+12*g_uiScale;
    if(g_appearance.armor&&b.statsValid&&b.maxArmor>0){
        float ratio=std::clamp(float(b.armor)/b.maxArmor,0.0f,1.0f);
        d->AddRectFilled(ImVec2(a.x-1,z.y+5),ImVec2(z.x+1,z.y+10),IM_COL32(5,8,14,210),3);
        if(ratio>0)d->AddRectFilled(ImVec2(a.x,z.y+6),ImVec2(a.x+w*ratio,z.y+9),uiColor(g_appearance.armorColor),2);
        labelY+=8*g_uiScale;
    }
    if(b.weaponValid&&g_appearance.weapon){
        std::snprintf(text,sizeof(text),"WEAPON %u",unsigned(b.weaponId));uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor),16*g_uiScale);labelY+=18*g_uiScale;
    }
    if(b.weaponValid&&g_appearance.ammo){
        std::snprintf(text,sizeof(text),"%u / %u",unsigned(b.magazineAmmo),unsigned(b.reserveAmmo));
        uiEspText(d,ImVec2(center,labelY),text,uiColor(g_appearance.textColor,0.73f),14*g_uiScale);
    }
}
void renderEspBoxes(){
    if(!g_espBoxesEnabled.load(std::memory_order_relaxed))return;
    std::lock_guard<std::mutex> lock(g_espMutex);auto* d=ImGui::GetBackgroundDrawList();
    for(const auto& b:g_espScreenBoxes)drawStyledEsp(d,b);
}
void renderWatermark(){
    auto& io=ImGui::GetIO();const float w=std::round(208*g_uiScale),h=std::round(50*g_uiScale);
    ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x-w)*0.5f,22*g_uiScale),ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin("##watermark",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground);
    const bool clicked=ImGui::InvisibleButton("##toggle_menu",ImVec2(w,h));
    const ImVec2 p=ImGui::GetItemRectMin();auto* d=ImGui::GetWindowDrawList();uiPanel(d,p,uiAdd(p,w,h),14);
    const float tw=ImGui::GetFont()->CalcTextSizeA(20*g_uiScale,FLT_MAX,0,"LemmingRMT").x;
    const float total=tw+31*g_uiScale,x=(w-total)*0.5f;
    uiIcon(d,8,uiAdd(p,x,14*g_uiScale),22*g_uiScale,uiColor(g_appearance.accent));
    uiText(d,uiAdd(p,x+31*g_uiScale+tw*0.5f,h*0.5f),"LemmingRMT",uiColor(ImVec4(.94f,.96f,1,1)),20*g_uiScale);
    if(ImGui::IsItemHovered())d->AddRect(p,uiAdd(p,w,h),uiColor(g_appearance.accent,0.6f),14*g_uiScale);
    if(clicked){const bool open=!g_menuOpen.load(std::memory_order_relaxed);g_menuOpen.store(open,std::memory_order_relaxed);LOGI("menu watermark toggle: open=%d",open);}
    ImGui::End();ImGui::PopStyleVar();
}
void uiSectionChange(int section,int sub){if(g_section!=section||g_subsection!=sub){g_section=section;g_subsection=sub;g_pageAnimation=0;}}
void renderMinimalMenu(){
    auto& io=ImGui::GetIO();g_uiScale=std::min({io.DisplaySize.y/1080.0f,io.DisplaySize.x/1440.0f,io.DisplaySize.y/760.0f,1.3f});
    g_uiScale=std::max(0.25f,g_uiScale);io.FontGlobalScale=g_uiScale;
    lemming::esp::detail::g_skeletonEnabled.store(g_appearance.skeleton&&g_espBoxesEnabled.load(),std::memory_order_relaxed);
    renderEspBoxes();renderWatermark();
    const bool open=g_menuOpen.load(std::memory_order_relaxed);g_menuAnimation=uiEase(g_menuAnimation,open?1:0);
    if(g_menuAnimation<0.005f&&!open)return;
    g_pageAnimation=uiEase(g_pageAnimation,1,15);
    const float s=g_uiScale,w=std::round(700*s),h=std::round(590*s),x=std::floor((io.DisplaySize.x-w)*.5f),y=std::floor((io.DisplaySize.y-h)*.5f)+(1-g_menuAnimation)*12*s;
    auto flags=ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground;
    if(!open)flags|=ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation);ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(12*s,12*s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,6*s));
    ImGui::SetNextWindowPos(ImVec2(x-308*s,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(132*s,280*s),ImGuiCond_Always);
    ImGui::Begin("##sections",nullptr,flags);uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),132*s,280*s));
    if(uiRailButton("Aim",0,g_section==0,108*s))uiSectionChange(0,0);
    if(uiRailButton("Visuals",1,g_section==1,108*s))uiSectionChange(1,0);
    if(uiRailButton("Misc",2,g_section==2,108*s))uiSectionChange(2,0);
    if(uiRailButton("Settings",7,g_section==3,108*s))uiSectionChange(3,0);ImGui::End();
    const char* labels[4][4]={{"Legit","Silent","Targeting",nullptr},{"Players","Chams","World","Appearance"},{"General","Movement",nullptr,nullptr},{"Theme","About",nullptr,nullptr}};
    const int icons[4][4]={{0,0,3,0},{3,4,4,5},{2,2,0,0},{5,8,0,0}};
    int n=0;while(n<4&&labels[g_section][n])++n;
    ImGui::SetNextWindowPos(ImVec2(x-160*s,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(144*s,(24+n*64)*s),ImGuiCond_Always);
    ImGui::Begin("##subsections",nullptr,flags);uiPanel(ImGui::GetWindowDrawList(),ImGui::GetWindowPos(),uiAdd(ImGui::GetWindowPos(),144*s,(24+n*64)*s));
    for(int i=0;i<n;++i)if(uiRailButton(labels[g_section][i],icons[g_section][i],g_subsection==i,120*s))uiSectionChange(g_section,i);
    ImGui::End();
    ImGui::SetNextWindowPos(ImVec2(x,y),ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(w,h),ImGuiCond_Always);
    ImGui::Begin("##main",nullptr,flags);auto* d=ImGui::GetWindowDrawList();uiPanel(d,ImVec2(x,y),ImVec2(x+w,y+h),16);
    const char* sectionNames[]={"Aim","Visuals","Misc","Settings"};char title[64];std::snprintf(title,sizeof(title),"%s / %s",sectionNames[g_section],labels[g_section][g_subsection]);
    uiText(d,ImVec2(x+w*.5f,y+36*s),title,uiColor(ImVec4(.94f,.96f,1,1)),25*s,g_uiTitleFont);
    uiText(d,ImVec2(x+w*.5f,y+64*s),g_section==1&&g_subsection==0?"Player overlays, refined.":"LemmingRMT",uiColor(ImVec4(.49f,.55f,.66f,1)),16*s,g_uiSmallFont);
    d->AddLine(ImVec2(x+24*s,y+88*s),ImVec2(x+w-24*s,y+88*s),uiColor(ImVec4(.18f,.21f,.29f,.65f)),1);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha,g_menuAnimation*(0.18f+.82f*g_pageAnimation));
    const float slide=(1-g_pageAnimation)*10*s;
    if(g_section==1&&g_subsection==0){
        ImGui::SetCursorPos(ImVec2(30*s,107*s+slide));
        ImGui::BeginChild("##controls",ImVec2(342*s,451*s),ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
        bool enabled=g_espBoxesEnabled.load();if(uiToggle("Enemy ESP",enabled)){g_espBoxesEnabled.store(enabled);LOGI("ESP appearance master: enabled=%d",enabled);}
        uiToggle("Box",g_appearance.box);uiToggle("Skeleton",g_appearance.skeleton);uiToggle("Health",g_appearance.health);uiToggle("Armor",g_appearance.armor);
        uiToggle("Weapon",g_appearance.weapon);uiToggle("Ammo",g_appearance.ammo);uiToggle("Soft fill",g_appearance.fill);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::Combo("##box_style",&g_appearance.boxStyle,"Full rectangle\0Corner frame\0");
        ImGui::EndChild();
        const ImVec2 a(x+400*s,y+111*s),b(x+670*s,y+548*s);
        d->AddRectFilled(a,b,uiColor(ImVec4(.066f,.078f,.104f,1)),12*s);d->AddRect(a,b,uiColor(ImVec4(.18f,.21f,.28f,.60f)),12*s);
        uiText(d,ImVec2((a.x+b.x)*.5f,a.y+26*s),"STYLE PREVIEW",uiColor(ImVec4(.48f,.54f,.66f,1)),13*s,g_uiSmallFont);
        EspScreenBox demo{};demo.left=a.x+83*s;demo.right=b.x-83*s;demo.top=a.y+77*s;demo.bottom=b.y-102*s;
        demo.statsValid=demo.weaponValid=demo.enemy=true;demo.hp=76;demo.maxHp=100;demo.armor=64;demo.maxArmor=100;demo.weaponId=12;demo.magazineAmmo=30;demo.reserveAmmo=90;
        // The preview mannequin is decorative; live skeleton is only real bone data.
        const float cx=(a.x+b.x)*.5f,cy=demo.top+34*s;
        d->AddCircle(ImVec2(cx,cy),13*s,uiColor(ImVec4(.33f,.39f,.51f,.50f)),24,2*s);
        d->AddLine(ImVec2(cx,cy+14*s),ImVec2(cx,cy+120*s),uiColor(ImVec4(.33f,.39f,.51f,.50f)),3*s);
        d->AddLine(ImVec2(cx-29*s,cy+40*s),ImVec2(cx+29*s,cy+40*s),uiColor(ImVec4(.33f,.39f,.51f,.50f)),3*s);
        d->AddLine(ImVec2(cx,cy+120*s),ImVec2(cx-27*s,cy+190*s),uiColor(ImVec4(.33f,.39f,.51f,.50f)),3*s);
        d->AddLine(ImVec2(cx,cy+120*s),ImVec2(cx+27*s,cy+190*s),uiColor(ImVec4(.33f,.39f,.51f,.50f)),3*s);
        drawStyledEsp(d,demo,true);
    }else if((g_section==1&&g_subsection==3)||(g_section==3&&g_subsection==0)){
        ImGui::SetCursorPos(ImVec2(72*s,116*s+slide));ImGui::BeginChild("##appearance",ImVec2(556*s,420*s),ImGuiChildFlags_None);
        uiColorRow("Accent",g_appearance.accent);uiColorRow("Box",g_appearance.boxColor);uiColorRow("Skeleton",g_appearance.skeletonColor);
        uiColorRow("Health",g_appearance.healthColor);uiColorRow("Armor",g_appearance.armorColor);uiColorRow("Text",g_appearance.textColor);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);ImGui::SliderFloat("##line_width",&g_appearance.lineWidth,1,3,"Line weight: %.1f px");ImGui::EndChild();
    }else if(g_section==3&&g_subsection==1){
        uiIcon(d,8,ImVec2(x+w*.5f-22*s,y+195*s),44*s,uiColor(g_appearance.accent));
        uiText(d,ImVec2(x+w*.5f,y+280*s),"LemmingRMT",uiColor(ImVec4(.94f,.96f,1,1)),30*s,g_uiTitleFont);
        uiText(d,ImVec2(x+w*.5f,y+320*s),"Lucide icons / Mozilla Text",uiColor(ImVec4(.54f,.61f,.72f,1)),18*s,g_uiSmallFont);
    }else{
        uiIcon(d,icons[g_section][g_subsection],ImVec2(x+w*.5f-22*s,y+199*s),44*s,uiColor(g_appearance.accent,.7f));
        uiText(d,ImVec2(x+w*.5f,y+286*s),"Coming later",uiColor(ImVec4(.78f,.83f,.92f,1)),25*s,g_uiTitleFont);
        uiText(d,ImVec2(x+w*.5f,y+323*s),"This section is reserved. No feature is enabled.",uiColor(ImVec4(.47f,.54f,.66f,1)),17*s,g_uiSmallFont);
    }
    ImGui::PopStyleVar();ImGui::End();ImGui::PopStyleVar(3);
}
