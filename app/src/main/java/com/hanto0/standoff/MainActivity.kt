package com.hanto0.standoff

import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.view.Gravity
import android.view.View
import android.widget.*

class MainActivity : Activity() {
    private val bg     = Color.rgb(12, 15, 23)
    private val panel  = Color.rgb(20, 25, 37)
    private val accent = Color.rgb(151, 174, 255)
    private val text   = Color.rgb(238, 242, 255)
    private val muted  = Color.rgb(133, 145, 169)
    private val green  = Color.rgb(100, 220, 140)
    private val red    = Color.rgb(255, 90, 90)
    private lateinit var body: LinearLayout
    private lateinit var tabs: LinearLayout
    private var serviceRunning = false

    private val OVERLAY_PERM = 1234

    override fun onCreate(s: Bundle?) {
        super.onCreate(s)
        window.statusBarColor = bg
        window.navigationBarColor = bg
        window.decorView.systemUiVisibility = View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR.inv()

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(bg)
            setPadding(dp(20), dp(18), dp(20), dp(18))
        }

        root.addView(label("HANTO0  /  STANDOFF 2", 12f, accent, true))
        root.addView(label("ESP & Aimbot", 28f, text, true).apply { setPadding(0, dp(8), 0, dp(2)) })
        root.addView(label("Memory · Overlay · ROOT required", 13f, muted, false))

        tabs = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setPadding(0, dp(20), 0, dp(14))
        }
        listOf("GENERAL", "VISUALS", "SETTINGS").forEachIndexed { i, t ->
            val b = TextView(this).apply {
                this.text = t
                textSize = 11f
                typeface = Typeface.DEFAULT_BOLD
                gravity = Gravity.CENTER
                setTextColor(if (i == 0) accent else muted)
                background = rounded(if (i == 0) 0x332F4D88 else 0x00141825, 12)
                setPadding(dp(12), dp(12), dp(12), dp(12))
            }
            tabs.addView(b, LinearLayout.LayoutParams(0, dp(42), 1f).apply { marginEnd = dp(6) })
            b.setOnClickListener {
                for (j in 0 until tabs.childCount) {
                    val tb = tabs.getChildAt(j) as TextView
                    tb.setTextColor(muted)
                    tb.background = rounded(0x00141825, 12)
                }
                b.setTextColor(accent)
                b.background = rounded(0x332F4D88, 12)
                showPage(t)
            }
        }
        root.addView(tabs)
        body = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        root.addView(body, LinearLayout.LayoutParams(-1, 0, 1f))
        root.addView(label("hanto0 v2.1 · github.com/antonxd09-jpg/hanto0-standoff", 11f, muted, false).apply {
            gravity = Gravity.CENTER
            setPadding(0, dp(16), 0, dp(4))
        })
        setContentView(root)
        showPage("GENERAL")
    }

    private fun showPage(page: String) {
        body.removeAllViews()
        when (page) {
            "GENERAL" -> {
                cardTitle("Status", "Launch Standoff 2, enter a match, then start.")
                val statusText = label(
                    if (serviceRunning) "● ESP running" else "○ Stopped",
                    14f, if (serviceRunning) green else muted, true
                )
                body.addView(statusText, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(12); setMargins(dp(4),0,0,dp(8)) })

                val startBtn = TextView(this).apply {
                    text = if (serviceRunning) "STOP" else "START"
                    gravity = Gravity.CENTER
                    textSize = 15f
                    typeface = Typeface.DEFAULT_BOLD
                    setTextColor(Color.rgb(15, 20, 32))
                    background = rounded(if (serviceRunning) 0xFFFF5A5A.toInt() else 0xFF97AEFF.toInt(), 14)
                    setPadding(0, dp(18), 0, dp(18))
                }
                startBtn.setOnClickListener {
                    if (serviceRunning) {
                        stopService(Intent(this, OverlayService::class.java))
                        serviceRunning = false
                    } else {
                        if (!Settings.canDrawOverlays(this)) {
                            startActivityForResult(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                                Uri.parse("package:$packageName")), OVERLAY_PERM)
                        } else {
                            startService(Intent(this, OverlayService::class.java))
                            serviceRunning = true
                            Toast.makeText(this, "ESP started — switch to Standoff 2", Toast.LENGTH_SHORT).show()
                        }
                    }
                    showPage("GENERAL")
                }
                body.addView(startBtn, LinearLayout.LayoutParams(-1, -2).apply { topMargin = dp(4); bottomMargin = dp(10) })

                cardTitle("Info", "ROOT + SYSTEM_ALERT_WINDOW required.")
                val note = label("Open hanto0 AFTER entering a match.\nAll ESP toggles are in the VISUALS tab.\nIn-game overlay button: tap to show/hide menu.", 13f, muted, false)
                note.setPadding(dp(4), dp(6), dp(4), dp(6))
                body.addView(note)
            }
            "VISUALS" -> {
                cardTitle("ESP", "Controls what draws over the game.")
                espToggle("Silent Aim",   OverlayService.silentAim)  { OverlayService.silentAim   = it; SilentAim.enabled = it }
                espToggle("ESP Box",      OverlayService.showBox)     { OverlayService.showBox     = it }
                espToggle("ESP Lines",    OverlayService.showLines)   { OverlayService.showLines   = it }
                espToggle("ESP Skeleton", OverlayService.showSkeleton){ OverlayService.showSkeleton= it }
                espToggle("ESP HP",       OverlayService.showHP)      { OverlayService.showHP      = it }
                espToggle("ESP Info",     OverlayService.showInfo)    { OverlayService.showInfo    = it }
            }
            else -> {
                cardTitle("Build", "Android · Kotlin · GitHub Actions")
                val info = label("Package: com.hanto0.standoff\nVersion: 2.1\nRepo: antonxd09-jpg/hanto0-standoff\n\nMemory reader via /proc/pid/mem\nOverlay via SYSTEM_ALERT_WINDOW\nil2cpp string-scan for PhotonNetwork", 13f, muted, false)
                info.setPadding(dp(4), dp(8), dp(4), dp(8))
                body.addView(info)
            }
        }
    }

    private fun espToggle(name: String, init: Boolean, fn: (Boolean) -> Unit) {
        val row = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            background = rounded(panel, 14)
            setPadding(dp(14), dp(8), dp(12), dp(8))
        }
        row.addView(label(name, 14f, text, false), LinearLayout.LayoutParams(0, -2, 1f))
        val sw = Switch(this).apply {
            buttonTintList = android.content.res.ColorStateList.valueOf(accent)
            isChecked = init
            setOnCheckedChangeListener { _, b -> fn(b) }
        }
        row.addView(sw)
        body.addView(row, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(8) })
    }

    private fun cardTitle(title: String, subtitle: String) {
        val card = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = rounded(panel, 18)
            setPadding(dp(16), dp(16), dp(16), dp(10))
        }
        card.addView(label(title, 17f, text, true))
        card.addView(label(subtitle, 12f, muted, false).apply { setPadding(0, dp(5), 0, dp(8)) })
        body.addView(card, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(12) })
    }

    private fun label(v: String, sz: Float, c: Int, bold: Boolean) = TextView(this).apply {
        text = v; textSize = sz; setTextColor(c)
        if (bold) typeface = Typeface.DEFAULT_BOLD
    }
    private fun rounded(color: Int, r: Int) = GradientDrawable().apply {
        setColor(color); cornerRadius = dp(r).toFloat(); setStroke(dp(1), 0x332F3B55)
    }
    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    override fun onActivityResult(req: Int, res: Int, data: Intent?) {
        if (req == OVERLAY_PERM && Settings.canDrawOverlays(this)) {
            startService(Intent(this, OverlayService::class.java))
            serviceRunning = true
            showPage("GENERAL")
        }
    }
}
