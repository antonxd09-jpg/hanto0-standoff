// ASTRA_ESP_INFO_V5
// ASTRA_ESP_GUI_FIX_V4
// ASTRA_ESP_REFINE_V3
// ASTRA_ESP_STYLES_V2
#pragma once
// ASTRA_ESP_STYLE_V1
// Included after g_b, ESP colors and g_S. Rendering only.
namespace astra_esp_extra {
struct Settings {
 bool corners=false,fill=false,snaplines=false,headMarker=false;
 bool shadow=true,outline=true;
 bool distance=false,hpNumber=false,armorNumber=false,lowHealth=false,emptyMagazine=false;
 float lowHealthPercent=25.f,metersPerUnit=1.f;
 float thickness=1.f,fillAlpha=.045f,textScale=1.f;
 ImVec4 armorColor{.50f,.66f,.86f,1.f};
};
static Settings settings;
static float bounded(float v,float lo,float hi){return std::isfinite(v)?std::max(lo,std::min(v,hi)):lo;}
static ImU32 rgba(ImVec4 c){return ImGui::ColorConvertFloat4ToU32(c);}
static float pixel(float v){return std::floor(v)+.5f;}
static float labelSize(){
 // Fixed optical size, independent of the menu's font/window scaling.
 const float screenScale=bounded(ImGui::GetIO().DisplaySize.y/1080.f,.85f,1.25f);
 return std::max(10.f,14.f*screenScale*bounded(settings.textScale,.5f,2.f));
}
static void text(ImDrawList* d,float cx,float y,const char* value,ImVec4 c){
 if(!value||!*value)return;
 auto* font=ImGui::GetFont();const float size=labelSize();
 const auto ext=font->CalcTextSizeA(size,100000.f,0,value);
 const ImVec2 p(std::floor(cx-ext.x*.5f),std::floor(y));
 const auto ink=IM_COL32(0,0,0,210);
 if(settings.outline){
  // One-pixel outline rather than a heavy text halo.
  d->AddText(font,size,ImVec2(p.x-1,p.y),ink,value);
  d->AddText(font,size,ImVec2(p.x+1,p.y),ink,value);
  d->AddText(font,size,ImVec2(p.x,p.y-1),ink,value);
  d->AddText(font,size,ImVec2(p.x,p.y+1),ink,value);
 }else if(settings.shadow)d->AddText(font,size,ImVec2(p.x+1,p.y+1),IM_COL32(0,0,0,230),value);
 d->AddText(font,size,p,rgba(c),value);
}
static void legacyBox(ImDrawList* d,float x,float y,float w,float h,ImU32 c,float thickness){
 x=pixel(x);y=pixel(y);const float r=pixel(x+w),b=pixel(y+h);
 if(!settings.corners){d->AddRect(ImVec2(x,y),ImVec2(r,b),c,0,0,thickness);return;}
 const float lx=std::max(2.f,std::min(w*.24f,24.f*bounded(g_S,.5f,2.f)));
 const float ly=std::max(2.f,std::min(h*.16f,30.f*bounded(g_S,.5f,2.f)));
 for(int i=0;i<4;++i){
  const float sx=(i&1)?-1.f:1.f,sy=(i&2)?-1.f:1.f;
  const ImVec2 a((i&1)?r:x,(i&2)?b:y);
  d->AddLine(a,ImVec2(a.x+sx*lx,a.y),c,thickness);
  d->AddLine(a,ImVec2(a.x,a.y+sy*ly),c,thickness);
 }
}

static int frameStyle=0;
static bool softHalo=false,barDivisions=false;
static float haloOpacity=.10f;
static void styleControls(){
 {
  const auto p=ImGui::GetCursorScreenPos();
  const float w=std::max(1.f,ImGui::CalcItemWidth());
  ImGui::GetWindowDrawList()->AddLine(p,ImVec2(p.x+w,p.y),ImGui::GetColorU32(ImGuiCol_Separator),1.f);
  ImGui::Dummy(ImVec2(w,1.f));
 }
 ImGui::TextUnformatted("Frame style");
 ImGui::SetNextItemWidth(ImGui::CalcItemWidth());
 ImGui::Combo("##esp-frame-style",&frameStyle,"Classic\0Corners\0Rounded\0Dashed\0Side brackets\0");
 ImGui::Checkbox("Soft frame halo",&softHalo);
 ImGui::Checkbox("HP / armor divisions",&barDivisions);
 ImGui::TextUnformatted("Halo opacity");
 ImGui::SliderFloat("##esp-halo-opacity",&haloOpacity,0.f,.25f,"%.2f");
 haloOpacity=bounded(haloOpacity,0.f,.25f);

}
static void box(ImDrawList* d,float x,float y,float w,float h,ImU32 c,float thickness){
 const int mode=std::max(0,std::min(frameStyle,4));
 if(mode==0||mode==1){
  // legacyBox reads the existing corner flag; restore after this draw.
  const bool old=settings.corners;settings.corners=mode==1;
  legacyBox(d,x,y,w,h,c,thickness);settings.corners=old;return;
 }
 x=pixel(x);y=pixel(y);const float r=pixel(x+w),b=pixel(y+h);

 if(mode==4){
  // Tall side brackets, leaving the top and bottom visually open.
  const float arm=std::max(2.f,std::min(w*.23f,18.f*bounded(g_S,.65f,1.7f)));
  d->AddLine(ImVec2(x,y),ImVec2(x,b),c,thickness);
  d->AddLine(ImVec2(r,y),ImVec2(r,b),c,thickness);
  d->AddLine(ImVec2(x,y),ImVec2(x+arm,y),c,thickness);
  d->AddLine(ImVec2(x,b),ImVec2(x+arm,b),c,thickness);
  d->AddLine(ImVec2(r,y),ImVec2(r-arm,y),c,thickness);
  d->AddLine(ImVec2(r,b),ImVec2(r-arm,b),c,thickness);
  return;
 }

 if(mode==2){
  const float rounding=std::max(1.f,std::min({w*.10f,h*.07f,7.f*bounded(g_S,.65f,1.7f)}));
  d->AddRect(ImVec2(x,y),ImVec2(r,b),c,rounding,0,thickness);return;
 }
 // Bounded segment count even for oversized on-screen boxes.
 auto dash=[&](ImVec2 a,ImVec2 z){
  const float dx=z.x-a.x,dy=z.y-a.y,len=std::sqrt(dx*dx+dy*dy);
  if(len<1)return;
  const float step=std::max(10.f*bounded(g_S,.65f,1.7f),len/100.f);
  for(float p=0;p<len;p+=step){
   const float end=std::min(len,p+step*.64f);
   d->AddLine(ImVec2(a.x+dx*p/len,a.y+dy*p/len),ImVec2(a.x+dx*end/len,a.y+dy*end/len),c,thickness);
  }
 };
 dash(ImVec2(x,y),ImVec2(r,y));dash(ImVec2(r,y),ImVec2(r,b));
 dash(ImVec2(r,b),ImVec2(x,b));dash(ImVec2(x,b),ImVec2(x,y));
}

static ImVec4 healthTint(float ratio){
 const ImVec4 low(.96f,.25f,.27f,1),mid(1.f,.76f,.30f,1);
 const ImVec4 high=g_espHpCol;
 const float f=ratio<.5f?ratio*2.f:(ratio-.5f)*2.f;
 const auto a=ratio<.5f?low:mid;const auto b=ratio<.5f?mid:high;
 return ImVec4(a.x+(b.x-a.x)*f,a.y+(b.y-a.y)*f,a.z+(b.z-a.z)*f,1);
}

// Right-side badges are independent of health/armor bar visibility.
static void infoBadgeV5(ImDrawList* d,float x,float& y,const char* value,ImVec4 ink,float scale){
 const float fs=labelSize();
 auto* font=ImGui::GetFont();
 const auto ext=font->CalcTextSizeA(fs,100000.f,0,value);
 const float pad=4.f*scale,h=fs+4.f*scale;
 d->AddRectFilled(ImVec2(std::floor(x),std::floor(y)),
                  ImVec2(std::floor(x+ext.x+2*pad),std::floor(y+h)),
                  IM_COL32(7,10,15,175),2.f*scale);
 text(d,x+pad+ext.x*.5f,y+2.f*scale,value,ink);
 y+=h+3.f*scale;
}

template<class B> static void draw(ImDrawList* d,const B& b){
 const float x=b.left,y=b.top,w=b.right-b.left,h=b.bottom-b.top;
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(h)||w<4||h<4)return;
 const float s=bounded(ImGui::GetIO().DisplaySize.y/1080.f,.85f,1.25f),right=x+w,bottom=y+h,cx=x+w*.5f;
 const float th=bounded(settings.thickness,1,4);
 const auto edge=IM_COL32(3,5,8,220);
 const auto display=ImGui::GetIO().DisplaySize;
 if(settings.snaplines&&b.enemy){
  ImVec2 from(display.x*.5f,display.y-2),to(cx,bottom);
  auto tint=g_espBoxCol;tint.w*=.55f;
  d->AddLine(from,to,IM_COL32(0,0,0,150),th+1.5f);
  d->AddLine(from,to,rgba(tint),th);
 }
 if(g_b.esp_box){
  if(settings.fill){
   ImVec4 top=g_espBoxCol,low=g_espBoxCol;
   const float alpha=bounded(settings.fillAlpha,0,.35f);
   top.w*=alpha*.10f;low.w*=alpha;
   d->AddRectFilledMultiColor(ImVec2(x,y),ImVec2(right,bottom),rgba(top),rgba(top),rgba(low),rgba(low));
  }

  if(softHalo){
   // Static layered halo: no pulsing and no positional interpolation.
   auto glow=g_espBoxCol;
   for(int layer=3;layer>=1;--layer){
    glow.w=g_espBoxCol.w*bounded(haloOpacity,0,.25f)*(.34f/float(layer));
    box(d,x,y,w,h,rgba(glow),th+float(layer)*3.f*s);
   }
  }

  // Wide dark stroke leaves one dark edge on either side of the color.
  box(d,x,y,w,h,edge,th+2.f);
  box(d,x,y,w,h,rgba(g_espBoxCol),th);
 }
 if(settings.headMarker&&b.enemy){
  // Decorative screen marker, not an anatomical head position.
  const ImVec2 p(pixel(cx),pixel(y-7*s));const float r=3*s;
  d->AddCircle(p,r,edge,16,th+2);d->AddCircle(p,r,rgba(g_espBoxCol),16,th);
 }
 if(g_b.esp_hp&&b.statsValid&&b.maxHp>0){
  const float ratio=bounded(float(b.hp)/float(b.maxHp),0,1);
  const float hpScale=bounded(g_S,.65f,1.7f);
  const float bar=std::max(2.f,std::round(2.f*hpScale));
  const float bx=std::floor(x-5*hpScale-bar),by=std::floor(y),bh=std::max(1.f,std::round(h));
  d->AddRectFilled(ImVec2(bx-1,by-1),ImVec2(bx+bar+1,by+bh+1),edge);
  d->AddRectFilled(ImVec2(bx,by),ImVec2(bx+bar,by+bh),IM_COL32(28,31,36,230));
  const float filled=bh*ratio,fy=by+bh-filled;
  if(ratio>0){
   auto tint=healthTint(ratio);auto dark=tint;dark.x*=.70f;dark.y*=.70f;dark.z*=.70f;
   d->AddRectFilledMultiColor(ImVec2(bx,fy),ImVec2(bx+bar,by+bh),rgba(tint),rgba(tint),rgba(dark),rgba(dark));
   if(bar>=3)d->AddLine(ImVec2(bx+.5f,fy),ImVec2(bx+.5f,by+bh),IM_COL32(255,255,255,45));
  }

  if(barDivisions&&bh>=45*s){
   for(int i=1;i<5;++i){
    const float yy=std::floor(by+bh*float(i)/5.f);
    d->AddLine(ImVec2(bx,yy),ImVec2(bx+bar,yy),IM_COL32(4,6,10,150),1.f);
   }
  }

  // Only damaged players get a numeric label: less visual noise at full HP.
  if(!settings.hpNumber&&b.hp<b.maxHp&&h>labelSize()*2){
   char hp[24];std::snprintf(hp,sizeof(hp),"%u",unsigned(b.hp));
   const float tw=ImGui::GetFont()->CalcTextSizeA(labelSize(),100000,0,hp).x;
   const float ly=std::max(y,std::min(fy-labelSize()*.5f,bottom-labelSize()));
   text(d,bx-3*s-tw*.5f,ly,hp,ImVec4(.96f,.97f,1,1));
  }
 }

 float infoY=y;
 char infoBuffer[48];
 if(settings.hpNumber&&b.statsValid&&b.maxHp>0){
  std::snprintf(infoBuffer,sizeof(infoBuffer),"HP %u",unsigned(b.hp));
  infoBadgeV5(d,right+6*s,infoY,infoBuffer,healthTint(bounded(float(b.hp)/float(b.maxHp),0,1)),s);
 }
 if(settings.armorNumber&&b.statsValid&&b.maxArmor>0){
  std::snprintf(infoBuffer,sizeof(infoBuffer),"AP %u",unsigned(b.armor));
  infoBadgeV5(d,right+6*s,infoY,infoBuffer,settings.armorColor,s);
 }
 if(settings.lowHealth&&b.statsValid&&b.maxHp>0&&b.hp>0&&
    float(b.hp)/float(b.maxHp)<=bounded(settings.lowHealthPercent,5,50)*.01f){
  infoBadgeV5(d,right+6*s,infoY,"LOW HP",ImVec4(1,.36f,.34f,1),s);
 }
 if(settings.emptyMagazine&&b.weaponValid&&b.magazineAmmo==0){
  // Empty magazine does not imply a reload is in progress.
  infoBadgeV5(d,right+6*s,infoY,"EMPTY",ImVec4(1,.77f,.36f,1),s);
 }

 float labelY=bottom+4*s;
 if(g_b.esp_armor&&b.statsValid&&b.maxArmor>0){
  const float ratio=bounded(float(b.armor)/float(b.maxArmor),0,1);
  const float bar=std::max(2.f,std::round(2*s));
  d->AddRectFilled(ImVec2(x-1,labelY-1),ImVec2(right+1,labelY+bar+1),edge);
  d->AddRectFilled(ImVec2(x,labelY),ImVec2(right,labelY+bar),IM_COL32(28,31,36,230));
  if(ratio>0){
   auto light=settings.armorColor,dark=light;dark.x*=.72f;dark.y*=.72f;dark.z*=.72f;
   d->AddRectFilledMultiColor(ImVec2(x,labelY),ImVec2(x+w*ratio,labelY+bar),rgba(light),rgba(dark),rgba(dark),rgba(light));
  }

  if(barDivisions&&w>=35*s){
   for(int i=1;i<5;++i){
    const float xx=std::floor(x+w*float(i)/5.f);
    d->AddLine(ImVec2(xx,labelY),ImVec2(xx,labelY+bar),IM_COL32(4,6,10,150),1.f);
   }
  }

  labelY+=bar+4*s;
 }
 char buf[64];const float line=labelSize()+1*s;
 if(b.weaponValid&&g_b.esp_wep){
  // Real weapon names require a verified ID table; do not invent one.
  std::snprintf(buf,sizeof(buf),"WPN #%u",unsigned(b.weaponId));
  text(d,cx,labelY,buf,g_espWepCol);labelY+=line;
 }
 if(b.weaponValid&&g_b.esp_ammo){
  std::snprintf(buf,sizeof(buf),"%u / %u",unsigned(b.magazineAmmo),unsigned(b.reserveAmmo));
  text(d,cx,labelY,buf,g_espAmmoCol);labelY+=line;
 }

 if(settings.distance&&b.distanceValid&&std::isfinite(b.distanceUnits)&&b.distanceUnits>=0){
  // Unit conversion is explicitly configurable; 1 Unity unit = 1 m is a default assumption.
  const float meters=b.distanceUnits*bounded(settings.metersPerUnit,.01f,10.f);
  std::snprintf(buf,sizeof(buf),"%.0f m",meters);
  text(d,cx,labelY,buf,ImVec4(.82f,.85f,.91f,1));
 }

}
static void controls(){
 auto& c=settings;
 ImGui::Checkbox("Box fill",&c.fill);
 ImGui::Checkbox("Snaplines",&c.snaplines);ImGui::Checkbox("Box head marker",&c.headMarker);
 const float width=ImGui::CalcItemWidth();
 auto slider=[&](const char* name,float* v,float lo,float hi,const char* format){
  ImGui::PushID(name);ImGui::TextUnformatted(name);ImGui::SetNextItemWidth(width);
  ImGui::SliderFloat("##value",v,lo,hi,format);*v=bounded(*v,lo,hi);ImGui::PopID();
 };
 slider("Line width",&c.thickness,1,4,"%.1f");slider("Fill opacity",&c.fillAlpha,0,.35f,"%.2f");
 slider("ESP text scale",&c.textScale,.5f,2,"%.2f");
 ImGui::Checkbox("Text shadow",&c.shadow);ImGui::Checkbox("Text outline",&c.outline);
 ImGui::ColorEdit3("Armor color",&c.armorColor.x,ImGuiColorEditFlags_NoInputs);
}
static void reset(){settings=Settings{};frameStyle=0;softHalo=false;barDivisions=false;haloOpacity=.10f;}
}
