package com.hanto.standoff
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.Gravity
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (!Settings.canDrawOverlays(this)) {
            startActivity(Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:$packageName")))
        }
        val root = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER; setPadding(64,64,64,64) }
        root.addView(TextView(this).apply { text = "hanto v2"; textSize = 28f })
        root.addView(Button(this).apply {
            text = "START STANDOFF"; textSize = 18f
            setOnClickListener {
                GameLauncher.start(this@MainActivity)
                Handler(Looper.getMainLooper()).postDelayed({ startForegroundService(Intent(this@MainActivity, OverlayService::class.java)) }, 10_000L)
            }
        })
        root.addView(Button(this).apply {
            text = "STOP OVERLAY"; textSize = 18f
            setOnClickListener { stopService(Intent(this@MainActivity, OverlayService::class.java)) }
        })
        setContentView(root)
    }
}