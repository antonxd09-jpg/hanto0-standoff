package com.hanto.internal

import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    private lateinit var statusText: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(48, 48, 48, 48)
        }

        statusText = TextView(this).apply {
            text = "Status: Ready"
            textSize = 18f
        }
        
        val start = Button(this).apply { text = "START" }

        start.setOnClickListener {
            statusText.text = "Checking ROOT..."
            start.isEnabled = false // Блокируем кнопку от двойного нажатия
            
            Thread {
                val hasRoot = RootUtil.hasRoot()
                if (!hasRoot) {
                    runOnUiThread {
                        statusText.text = "ROOT DENIED"
                        Toast.makeText(this, "ROOT access required", Toast.LENGTH_LONG).show()
                        start.isEnabled = true
                    }
                    return@Thread
                }
                
                runOnUiThread { statusText.text = "Injecting..." }
                val success = RootUtil.setupAndInject(this)
                
                runOnUiThread {
                    if (success) {
                        Toast.makeText(this, "Launching game... Menu in 40s.", Toast.LENGTH_SHORT).show()
                        finish() // Мгновенно закрываем приложение, игра выходит на передний план
                    } else {
                        statusText.text = "Injection FAILED. Check assets."
                        start.isEnabled = true
                    }
                }
            }.start()
        }

        layout.addView(statusText)
        layout.addView(start)
        setContentView(layout)
    }
}
