// Included from premium.hpp before NLPageRage.
// Provides: NLSyncAimFromToggles()
// Uses: g_nls.toggles[], g_nls.sliders[], astra::aim::*, g_aimUi

static astra::aim::Configuration g_aimUi{};

// Called at end of NLPageRage to push toggle/slider state → aim config.
// toggle IDs used in NLPageRage MAIN card start at page*24 = 0*24 = 0:
//  id0 = enabled, id1 = onAttack, id2 = pauseInMenu
//  slider[0+page*24+3] = fov, [4]=maxDist, [5]=smooth
//  OTHER card ids start at 6: id6=showFov, id7=colorUnused, id8=visCheck, id9=rcs, slider[10]=rcsStr
//  SELECTION card ids start at 12: id12=lockTarget, id13=priority-select, id14=bodies-chevron
static void NLSyncAimFromToggles(int /*unused*/){
    int base=0*24; // page 0
    auto& n=g_aimUi.normal;
    n.enabled       = g_nls.toggles[base+0];
    n.onAttack      = g_nls.toggles[base+1];
    g_aimUi.pauseInMenu = g_nls.toggles[base+2];
    // fov slider: 0..1 → 0..360 degrees (half-angle stored)
    if(g_nls.sliderInit[base+3]) n.fov=g_nls.sliders[base+3]*180.f;
    // maxDistance slider: 0..1 → 1..500
    if(g_nls.sliderInit[base+4]) n.maxDistance=1.f+g_nls.sliders[base+4]*499.f;
    // smoothing slider: 0..1
    if(g_nls.sliderInit[base+5]) n.smoothing=g_nls.sliders[base+5];
    // OTHER
    n.showFov       = g_nls.toggles[base+6];
    // id7 = color (no toggle backing)
    n.visibleCheck  = g_nls.toggles[base+8];
    n.rcs           = g_nls.toggles[base+9];
    if(g_nls.sliderInit[base+10]) n.rcsStrength=g_nls.sliders[base+10]*100.f;
    // SELECTION
    n.lockTarget    = g_nls.toggles[base+12];
    n.priority      = std::clamp(g_nls.selects[base+13],0,2);
    astra::aim::publish(g_aimUi);
}