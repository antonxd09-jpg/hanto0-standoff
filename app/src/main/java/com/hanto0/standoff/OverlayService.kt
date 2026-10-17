package com.hanto0.standoff

import android.app.Service
import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.graphics.PixelFormat
import android.util.Log
import android.view.*
import android.widget.*

class OverlayService : Service() {
    companion object {
        var showBox      = true
        var showLines    = true
        var showSkeleton = true
        var showHP       = true
        var showInfo     = true
        var silentAim    = true
    }

    private val BG     = Color.rgb(12, 15, 23)
    private val PANEL  = Color.rgb(20, 25, 37)
    private val ACCENT = Color.rgb(151, 174, 255)
    private val TEXT   = Color.rgb(238, 242, 255)
    private val MUTED  = Color.rgb(133, 145, 169)

    private lateinit var wm: WindowManager
    private lateinit var btn: View
    private lateinit var menu: LinearLayout
    private lateinit var menuBody: LinearLayout
    private lateinit var esp: EspView
    private var menuVisible = false
    private var currentTab = "AIMBOT"
    private val handler = Handler(Looper.getMainLooper())

    private val loop = object : Runnable {
        override fun run() {
            try {
                val players = MemoryReader.getPlayers()
                val cam = Projection.CamData(
                    MemoryReader.getCamPos(),
                    MemoryReader.getCamYaw(),
                    MemoryReader.getCamPitch(),
                    90f,
                    resources.displayMetrics.widthPixels,
                    resources.displayMetrics.heightPixels
                )
                if (silentAim) {
                    SilentAim.calc(cam.pos, cam.yaw, cam.pitch, players)?.let {
                        MemoryReader.writeCamAngles(it.yaw, it.pitch)
                    }
                }
                esp.showBox = showBox; esp.showLines = showLines
                esp.showSkeleton = showSkeleton; esp.showHP = showHP; esp.showInfo = showInfo
                esp.update(players, cam)
            } catch (e: Exception) {
                Log.e("hanto0_esp", "loop error: $e")
            }
            handler.postDelayed(this, 50)
        }
    }

    override fun onBind(i: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        Log.d("hanto0_esp", "OverlayService onCreate")
        try {
            val ok = MemoryReader.init()
            Log.d("hanto0_esp", "MemoryReader.init=$ok")
        } catch (e: Exception) {
            Log.e("hanto0_esp", "MemoryReader init error: $e")
        }
        wm = getSystemService(WINDOW_SERVICE) as WindowManager
        setupEsp()
        setupBtn()
        setupMenu()
        handler.post(loop)
    }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun rounded(color: Int, r: Int) = GradientDrawable().apply {
        setColor(color)
        cornerRadius = dp(r).toFloat()
        setStroke(dp(1), 0x442F3B55)
    }

    private fun label(v: String, sz: Float, c: Int, bold: Boolean) = TextView(this).apply {
        text = v; textSize = sz; setTextColor(c)
        if (bold) typeface = Typeface.DEFAULT_BOLD
    }

    private fun setupEsp() {
        esp = EspView(this)
        wm.addView(esp, WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
            WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT))
    }

    private fun setupBtn() {
        val b = TextView(this).apply {
            text = "FS"
            textSize = 12f
            typeface = Typeface.DEFAULT_BOLD
            gravity = Gravity.CENTER
            setTextColor(ACCENT)
            background = rounded(0xDD0C0F17.toInt(), 10)
        }
        btn = b
        val p = WindowManager.LayoutParams(dp(44), dp(44),
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT).apply {
            gravity = Gravity.TOP or Gravity.START; x = 8; y = 120
        }
        var ix = 0; var iy = 0; var itx = 0f; var ity = 0f
        b.setOnTouchListener { _, e ->
            when (e.action) {
                MotionEvent.ACTION_DOWN -> { ix=p.x; iy=p.y; itx=e.rawX; ity=e.rawY; true }
                MotionEvent.ACTION_MOVE -> { p.x=ix+(e.rawX-itx).toInt(); p.y=iy+(e.rawY-ity).toInt(); wm.updateViewLayout(btn,p); true }
                MotionEvent.ACTION_UP   -> { if (Math.abs(e.rawX-itx)<8 && Math.abs(e.rawY-ity)<8) toggleMenu(); true }
                else -> false
            }
        }
        wm.addView(btn, p)
    }

    private fun setupMenu() {
        menu = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = rounded(0xF00C0F17.toInt(), 20)
            setPadding(dp(16), dp(14), dp(16), dp(16))
            visibility = View.GONE
        }

        // header
        val header = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(0, 0, 0, dp(10))
        }
        header.addView(label("HANTO0  /  STANDOFF 2", 10f, ACCENT, true))
        header.addView(label("ESP & Aimbot", 22f, TEXT, true).apply { setPadding(0, dp(4), 0, 0) })
        menu.addView(header)

        // divider
        menu.addView(View(this).apply {
            setBackgroundColor(0x332F3B55)
        }, LinearLayout.LayoutParams(-1, dp(1)).apply { bottomMargin = dp(12) })

        // tabs
        val tabRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(0, 0, 0, dp(12))
        }
        val tabNames = listOf("AIMBOT", "VISUALS")
        val tabViews = mutableListOf<TextView>()
        tabNames.forEachIndexed { i, t ->
            val tv = TextView(this).apply {
                text = t
                textSize = 10f
                typeface = Typeface.DEFAULT_BOLD
                gravity = Gravity.CENTER
                setTextColor(if (i == 0) ACCENT else MUTED)
                background = rounded(if (i == 0) 0x332F4D88 else 0x00000000, 10)
                setPadding(dp(10), dp(8), dp(10), dp(8))
            }
            tabViews.add(tv)
            tabRow.addView(tv, LinearLayout.LayoutParams(0, dp(34), 1f).apply { marginEnd = if (i==0) dp(6) else 0 })
            tv.setOnClickListener {
                tabViews.forEach { tb ->
                    tb.setTextColor(MUTED)
                    tb.background = rounded(0x00000000, 10)
                }
                tv.setTextColor(ACCENT)
                tv.background = rounded(0x332F4D88, 10)
                currentTab = t
                renderTabBody()
            }
        }
        menu.addView(tabRow)

        menuBody = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        menu.addView(menuBody)

        renderTabBody()

        val mp = WindowManager.LayoutParams(dp(260),
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT).apply {
            gravity = Gravity.TOP or Gravity.START; x = 60; y = 120
        }
        wm.addView(menu, mp)
    }

    private fun renderTabBody() {
        menuBody.removeAllViews()
        when (currentTab) {
            "AIMBOT" -> {
                overlayToggle("Silent Aim", silentAim) { silentAim = it; SilentAim.enabled = it }
            }
            "VISUALS" -> {
                overlayToggle("ESP Box",      showBox)      { showBox      = it }
                overlayToggle("ESP Lines",    showLines)    { showLines    = it }
                overlayToggle("ESP Skeleton", showSkeleton) { showSkeleton = it }
                overlayToggle("ESP HP",       showHP)       { showHP       = it }
                overlayToggle("ESP Info",     showInfo)     { showInfo     = it }
            }
        }
    }

    private fun overlayToggle(name: String, init: Boolean, fn: (Boolean) -> Unit) {
        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            background = rounded(PANEL, 12)
            setPadding(dp(12), dp(6), dp(10), dp(6))
        }
        row.addView(label(name, 13f, TEXT, false), LinearLayout.LayoutParams(0, -2, 1f))
        val sw = Switch(this).apply {
            isChecked = init
            buttonTintList = android.content.res.ColorStateList.valueOf(ACCENT)
            setOnCheckedChangeListener { _, b -> fn(b) }
        }
        row.addView(sw)
        menuBody.addView(row, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(6) })
    }

    private fun toggleMenu() {
        menuVisible = !menuVisible
        menu.visibility = if (menuVisible) View.VISIBLE else View.GONE
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacks(loop)
        MemoryReader.close()
        listOf(esp, btn, menu).forEach { try { wm.removeView(it) } catch (_: Exception) {} }
    }
}
