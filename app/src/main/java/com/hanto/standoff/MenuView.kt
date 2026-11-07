package com.hanto.standoff
import android.content.Context
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.view.Gravity
import android.view.View
import android.widget.*

object cfgVisual { var box=true; var skeleton=true; var line=true; var chams=false; var hp=true; var ammo=true }
object cfgAim { var silent=true; var visibleOnly=true; var trigger=false; var bone=0 }

class MenuView(ctx: Context, onClose: () -> Unit) : LinearLayout(ctx) {
    private val container = FrameLayout(ctx)
    private val tabVisual = LinearLayout(ctx)
    private val tabAim = LinearLayout(ctx)
    private val tabSkin = LinearLayout(ctx)
    private val skinList = LinearLayout(ctx)
    init {
        orientation = VERTICAL
        background = GradientDrawable().apply { setColor(Color.argb(200,255,255,255)); cornerRadius = 40f; setStroke(2, Color.argb(120,0,0,0)) }
        setPadding(30,30,30,30)
        addView(TextView(ctx).apply { text = "hanto v2"; textSize = 22f; setTextColor(Color.rgb(20,20,30)); gravity = Gravity.CENTER; setPadding(0,10,0,20) })
        val tabs = LinearLayout(ctx).apply { orientation = HORIZONTAL; gravity = Gravity.CENTER }
        tabs.addView(btn(ctx,"VISUAL"){ switchTab(0) })
        tabs.addView(btn(ctx,"AIM"){ switchTab(1) })
        tabs.addView(btn(ctx,"SKIN"){ switchTab(2) })
        addView(tabs); addView(container)
        buildVisual(); buildAim(); buildSkin(); switchTab(0)
        addView(btn(ctx,"CLOSE"){ onClose() })
    }
    private fun btn(ctx: Context, t: String, onClick: () -> Unit) = Button(ctx).apply { text = t; textSize = 14f; setBackgroundColor(Color.argb(60,0,100,255)); setTextColor(Color.rgb(20,20,30)); setOnClickListener { onClick() } }
    private fun switchTab(i: Int) { container.removeAllViews(); when(i){0->container.addView(tabVisual);1->container.addView(tabAim);2->container.addView(tabSkin)} }
    private fun row(ctx: Context, label: String, checked: Boolean, onChange: (Boolean)->Unit) = LinearLayout(ctx).apply {
        orientation = HORIZONTAL; gravity = Gravity.CENTER_VERTICAL; setPadding(0,12,0,12)
        addView(TextView(ctx).apply { text = label; textSize = 16f; setTextColor(Color.rgb(20,20,30)); layoutParams = LayoutParams(0, LayoutParams.WRAP_CONTENT, 1f) })
        addView(Switch(ctx).apply { isChecked = checked; setOnCheckedChangeListener { _, b -> onChange(b) } })
    }
    private fun buildVisual() {
        tabVisual.orientation = VERTICAL
        tabVisual.addView(row(context,"Box",true){cfgVisual.box=it;pushVisual()})
        tabVisual.addView(row(context,"Skeleton",true){cfgVisual.skeleton=it;pushVisual()})
        tabVisual.addView(row(context,"Line",true){cfgVisual.line=it;pushVisual()})
        tabVisual.addView(row(context,"Chams",false){cfgVisual.chams=it;pushVisual()})
        tabVisual.addView(row(context,"HP",true){cfgVisual.hp=it;pushVisual()})
        tabVisual.addView(row(context,"Ammo",true){cfgVisual.ammo=it;pushVisual()})
    }
    private fun buildAim() {
        tabAim.orientation = VERTICAL
        tabAim.addView(row(context,"Silent aim",true){cfgAim.silent=it;pushAim()})
        tabAim.addView(row(context,"Visible only",true){cfgAim.visibleOnly=it;pushAim()})
        tabAim.addView(row(context,"Triggerbot",false){cfgAim.trigger=it;pushAim()})
        tabAim.addView(Spinner(context).apply {
            adapter = ArrayAdapter(context, android.R.layout.simple_spinner_dropdown_item, arrayOf("head","body","arms","legs"))
            setSelection(cfgAim.bone)
            onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) { cfgAim.bone = pos; pushAim() }
                override fun onNothingSelected(p: AdapterView<*>?) {}
            }
        })
    }
    private fun buildSkin() {
        tabSkin.orientation = VERTICAL
        val cats = listOf("Knife" to 0, "AWP" to 1, "AK" to 2)
        val grid = GridLayout(context).apply { columnCount = 2 }
        cats.forEach { (n, id) -> grid.addView(btn(context,n){ showSkins(id) }) }
        tabSkin.addView(grid); skinList.orientation = VERTICAL; tabSkin.addView(skinList)
    }
    private fun showSkins(catId: Int) {
        skinList.removeAllViews()
        val skins = when(catId) { 0 -> listOf("Karambit" to 508, "Butterfly" to 515); 1 -> listOf("Dragon Lore" to 344); 2 -> listOf("Redline" to 282); else -> emptyList() }
        skins.forEach { (n, id) -> skinList.addView(btn(context,n){ Native.applySkin(id) }) }
    }
    private fun pushVisual() = Native.setVisual(cfgVisual.box,cfgVisual.skeleton,cfgVisual.line,cfgVisual.chams,cfgVisual.hp,cfgVisual.ammo)
    private fun pushAim() { Native.setAim(cfgAim.silent,cfgAim.visibleOnly,cfgAim.bone,6f); Native.setTriggerbot(cfgAim.trigger) }
}