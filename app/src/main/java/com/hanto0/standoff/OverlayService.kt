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
import kotlin.math.sin
import kotlin.math.cos
import kotlin.random.Random

class OverlayService : Service() {

    private lateinit var windowManager: WindowManager
    private lateinit var floatingButton: Button
    private lateinit var menuLayout: LinearLayout
    private lateinit var espView: EspView
    private var isMenuVisible = false
    private val handler = Handler(Looper.getMainLooper())

    private var showBox      = true
    private var showLines    = true
    private var showSkeleton = true

    private val updateRunnable = object : Runnable {
        override fun run() {
            espView.update(generateSimEntities())
            handler.postDelayed(this, 50)
        }
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        windowManager = getSystemService(WINDOW_SERVICE) as WindowManager

        setupEspView()
        setupFloatingButton()
        setupMenu()

        handler.post(updateRunnable)
    }

    private fun setupEspView() {
        espView = EspView(this)
        val params = WindowManager.LayoutParams(
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
            WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or
            WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT
        )
        windowManager.addView(espView, params)
    }

    private fun setupFloatingButton() {
        floatingButton = Button(this).apply {
            text = "ESP"
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#AA1A4A8A"))
            textSize = 12f
        }

        val btnParams = WindowManager.LayoutParams(
            120, 80,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            x = 0; y = 200
        }

        var initialX = 0; var initialY = 0
        var initialTouchX = 0f; var initialTouchY = 0f

        floatingButton.setOnTouchListener { v, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    initialX = btnParams.x; initialY = btnParams.y
                    initialTouchX = event.rawX; initialTouchY = event.rawY
                    true
                }
                MotionEvent.ACTION_MOVE -> {
                    btnParams.x = initialX + (event.rawX - initialTouchX).toInt()
                    btnParams.y = initialY + (event.rawY - initialTouchY).toInt()
                    windowManager.updateViewLayout(floatingButton, btnParams)
                    true
                }
                MotionEvent.ACTION_UP -> {
                    if (Math.abs(event.rawX - initialTouchX) < 5 &&
                        Math.abs(event.rawY - initialTouchY) < 5) {
                        toggleMenu()
                    }
                    true
                }
                else -> false
            }
        }
        windowManager.addView(floatingButton, btnParams)
    }

    private fun setupMenu() {
        menuLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.parseColor("#DD0D0D0D"))
            setPadding(20, 20, 20, 20)
            visibility = View.GONE
        }

        fun addToggle(label: String, initial: Boolean, onChange: (Boolean) -> Unit) {
            val row = LinearLayout(this).apply {
                orientation = LinearLayout.HORIZONTAL
                setPadding(0, 6, 0, 6)
            }
            val tv = TextView(this).apply {
                text = label; setTextColor(Color.WHITE); textSize = 14f
                layoutParams = LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f)
            }
            val sw = Switch(this).apply {
                isChecked = initial
                setOnCheckedChangeListener { _, b -> onChange(b) }
            }
            row.addView(tv); row.addView(sw)
            menuLayout.addView(row)
        }

        addToggle("ESP Box",      showBox)      { showBox = it;      espView.showBox = it }
        addToggle("ESP Lines",    showLines)    { showLines = it;    espView.showLines = it }
        addToggle("ESP Skeleton", showSkeleton) { showSkeleton = it; espView.showSkeleton = it }

        val menuParams = WindowManager.LayoutParams(
            500,
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            x = 130; y = 200
        }
        windowManager.addView(menuLayout, menuParams)
    }

    private fun toggleMenu() {
        isMenuVisible = !isMenuVisible
        menuLayout.visibility = if (isMenuVisible) View.VISIBLE else View.GONE
    }

    private var tick = 0f
    private fun generateSimEntities(): List<Entity> {
        tick += 0.05f
        val sw = resources.displayMetrics.widthPixels.toFloat()
        val sh = resources.displayMetrics.heightPixels.toFloat()
        return (0 until 5).map { i ->
            val cx = sw / 2f + cos(tick + i * 1.2f) * sw * 0.3f
            val cy = sh / 2f + sin(tick + i * 0.9f) * sh * 0.2f
            val bw = 80f; val bh = 160f
            val bones = listOf(
                cx to cy - bh,
                cx to cy - bh * 0.8f,
                cx to cy - bh * 0.5f,
                cx to cy - bh * 0.2f,
                cx - bw*0.4f to cy - bh*0.75f,
                cx - bw*0.4f to cy - bh*0.55f,
                cx - bw*0.4f to cy - bh*0.35f,
                cx + bw*0.4f to cy - bh*0.75f,
                cx + bw*0.4f to cy - bh*0.55f,
                cx + bw*0.4f to cy - bh*0.35f,
                cx - bw*0.2f to cy - bh*0.2f,
                cx - bw*0.2f to cy - bh*0.1f,
                cx - bw*0.2f to cy,
                cx + bw*0.2f to cy - bh*0.2f,
                cx + bw*0.2f to cy - bh*0.1f,
                cx + bw*0.2f to cy
            )
            Entity(cx - bw/2f, cy - bh, bw, bh, i % 2, bones)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacks(updateRunnable)
        windowManager.removeView(espView)
        windowManager.removeView(floatingButton)
        windowManager.removeView(menuLayout)
    }
}
