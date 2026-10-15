package com.hanto0.standoff

import android.app.Service
import android.content.Intent
import android.graphics.Color
import android.graphics.PixelFormat
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.view.*
import android.widget.*

class OverlayService : Service() {

    private lateinit var windowManager: WindowManager
    private lateinit var floatingButton: Button
    private lateinit var menuLayout: LinearLayout
    private lateinit var espView: EspView
    private var isMenuVisible = false
    private val handler = Handler(Looper.getMainLooper())

    var showBox      = true
    var showLines    = true
    var showSkeleton = true

    private val updateRunnable = object : Runnable {
        override fun run() {
            val players = MemoryReader.getPlayers()
            /* TODO: read real cam pos/yaw/pitch/fov from memory */
            val cam = Projection.CamData(
                pos   = MemoryReader.Vec3(0f, 0f, 0f),
                yaw   = 0f, pitch = 0f, fov = 90f,
                sw    = resources.displayMetrics.widthPixels,
                sh    = resources.displayMetrics.heightPixels
            )
            espView.showBox      = showBox
            espView.showLines    = showLines
            espView.showSkeleton = showSkeleton
            espView.update(players, cam)
            handler.postDelayed(this, 50)
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        MemoryReader.init()
        windowManager = getSystemService(WINDOW_SERVICE) as WindowManager
        setupEspView()
        setupFloatingButton()
        setupMenu()
        handler.post(updateRunnable)
    }

    private fun setupEspView() {
        espView = EspView(this)
        val p = WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
            WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT
        )
        windowManager.addView(espView, p)
    }

    private fun setupFloatingButton() {
        floatingButton = Button(this).apply {
            text = "ESP"; setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#AA1A4A8A")); textSize = 12f
        }
        val bp = WindowManager.LayoutParams(
            120, 80,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply { gravity = Gravity.TOP or Gravity.START; x = 0; y = 200 }

        var ix=0; var iy=0; var itx=0f; var ity=0f
        floatingButton.setOnTouchListener { _, e ->
            when (e.action) {
                MotionEvent.ACTION_DOWN -> { ix=bp.x; iy=bp.y; itx=e.rawX; ity=e.rawY; true }
                MotionEvent.ACTION_MOVE -> { bp.x=ix+(e.rawX-itx).toInt(); bp.y=iy+(e.rawY-ity).toInt(); windowManager.updateViewLayout(floatingButton,bp); true }
                MotionEvent.ACTION_UP   -> { if(Math.abs(e.rawX-itx)<5&&Math.abs(e.rawY-ity)<5) toggleMenu(); true }
                else -> false
            }
        }
        windowManager.addView(floatingButton, bp)
    }

    private fun setupMenu() {
        menuLayout = LinearLayout(this).apply {
            orientation=LinearLayout.VERTICAL
            setBackgroundColor(Color.parseColor("#DD0D0D0D"))
            setPadding(20,20,20,20); visibility=View.GONE
        }
        fun tog(label: String, init: Boolean, fn: (Boolean)->Unit) {
            val row = LinearLayout(this).apply { orientation=LinearLayout.HORIZONTAL; setPadding(0,6,0,6) }
            val tv  = TextView(this).apply { text=label; setTextColor(Color.WHITE); textSize=14f
                layoutParams=LinearLayout.LayoutParams(0,LinearLayout.LayoutParams.WRAP_CONTENT,1f) }
            val sw  = Switch(this).apply { isChecked=init; setOnCheckedChangeListener{_,b->fn(b)} }
            row.addView(tv); row.addView(sw); menuLayout.addView(row)
        }
        tog("ESP Box",      true)  { showBox=it }
        tog("ESP Lines",    true)  { showLines=it }
        tog("ESP Skeleton", true)  { showSkeleton=it }

        val mp = WindowManager.LayoutParams(
            500, WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply { gravity=Gravity.TOP or Gravity.START; x=130; y=200 }
        windowManager.addView(menuLayout, mp)
    }

    private fun toggleMenu() {
        isMenuVisible = !isMenuVisible
        menuLayout.visibility = if (isMenuVisible) View.VISIBLE else View.GONE
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacks(updateRunnable)
        MemoryReader.close()
        windowManager.removeView(espView)
        windowManager.removeView(floatingButton)
        windowManager.removeView(menuLayout)
    }
}
