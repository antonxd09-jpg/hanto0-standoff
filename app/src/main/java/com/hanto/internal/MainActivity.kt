package com.hanto.internal

import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 48, 48, 48)
        }

        val status = TextView(this).apply { text = "ROOT: checking..." }
        val start = Button(this).apply { text = "START" }

        start.setOnClickListener {
            if (!RootUtil.hasRoot()) {
                Toast.makeText(this, "ROOT access required", Toast.LENGTH_LONG).show()
                return@setOnClickListener
            }
            Toast.makeText(this, "Launching game. Menu in 50s...", Toast.LENGTH_LONG).show()
            RootUtil.setupAndInject(this)
            finish()
        }

        layout.addView(status)
        layout.addView(start)
        setContentView(layout)

        Thread {
            val ok = RootUtil.hasRoot()
            runOnUiThread {
                status.text = if (ok) "ROOT: granted" else "ROOT: denied"
            }
        }.start()
    }
}
