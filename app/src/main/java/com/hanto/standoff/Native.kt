package com.hanto.standoff
object Native {
    init { System.loadLibrary("hanto") }
    external fun attach(pid: Int): Boolean
    external fun detach()
    external fun getEntities(): FloatArray
    external fun setAim(enabled: Boolean, visibleOnly: Boolean, bone: Int, fov: Float)
    external fun setTriggerbot(enabled: Boolean)
    external fun setVisual(box: Boolean, skeleton: Boolean, line: Boolean, chams: Boolean, hp: Boolean, ammo: Boolean)
    external fun applySkin(defIndex: Int)
}