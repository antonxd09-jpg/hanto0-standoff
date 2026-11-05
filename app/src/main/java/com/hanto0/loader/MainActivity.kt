package com.hanto0.loader

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.graphics.Color
import android.graphics.PixelFormat
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.CountDownTimer
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.view.Gravity
import android.view.View
import android.view.WindowManager
import android.webkit.JavascriptInterface
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast

class MainActivity : Activity() {

    private var windowManager: WindowManager? = null
    private var overlayWebView: WebView? = null
    private var overlayParams: WindowManager.LayoutParams? = null
    private var timer: CountDownTimer? = null

    companion object {
        private const val TARGET_PACKAGE = "com.axlebolt.standoff2"
        private const val DELAY_MS = 50_000L
        private var isOverlayShowing = false
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setBackgroundColor(Color.parseColor("#0d0d0f"))
            setPadding(64, 64, 64, 64)
        }

        val badge = TextView(this).apply {
            text = "H"
            textSize = 24f
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
            background = GradientDrawable().apply {
                setColor(Color.parseColor("#8b5cf6"))
                cornerRadius = 24f
            }
            layoutParams = LinearLayout.LayoutParams(120, 120).apply {
                bottomMargin = 28
            }
        }
        root.addView(badge)

        val titleView = TextView(this).apply {
            text = "HantoO Internal"
            textSize = 22f
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(Color.parseColor("#e2e8f0"))
            gravity = Gravity.CENTER
        }
        root.addView(titleView)

        val subTitleView = TextView(this).apply {
            text = "v2.4.1 — Standoff 2 (ARM64)"
            textSize = 13f
            setTextColor(Color.parseColor("#8b5cf6"))
            gravity = Gravity.CENTER
            setPadding(0, 8, 0, 36)
        }
        root.addView(subTitleView)

        val statusView = TextView(this).apply {
            text = "Нажмите START для запуска Standoff 2.\nОригинальное меню появится ровно через 50 секунд."
            textSize = 14f
            setTextColor(Color.parseColor("#cbd5e1"))
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 48)
        }
        root.addView(statusView)

        val startButton = Button(this).apply {
            text = "START"
            textSize = 16f
            typeface = Typeface.DEFAULT_BOLD
            setTextColor(Color.WHITE)
            background = GradientDrawable().apply {
                setColor(Color.parseColor("#8b5cf6"))
                cornerRadius = 20f
            }
            layoutParams = LinearLayout.LayoutParams(420, 130)
        }

        startButton.setOnClickListener {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
                val intent = Intent(
                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:$packageName")
                )
                startActivity(intent)
                Toast.makeText(
                    this,
                    "Разрешите отображение поверх других окон и нажмите START",
                    Toast.LENGTH_LONG
                ).show()
                return@setOnClickListener
            }

            startButton.isEnabled = false
            launchStandoff2(statusView, startButton)
        }

        root.addView(startButton)
        setContentView(root)
    }

    private fun checkRootAvailable(): Boolean {
        return try {
            val proc = Runtime.getRuntime().exec(arrayOf("su", "-c", "id"))
            val code = proc.waitFor()
            code == 0
        } catch (e: Exception) {
            false
        }
    }

    private fun launchStandoff2(statusView: TextView, startButton: Button) {
        Thread {
            val hasRoot = checkRootAvailable()
            var launched = false

            val launchIntent = packageManager.getLaunchIntentForPackage(TARGET_PACKAGE)
            if (launchIntent != null) {
                launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                startActivity(launchIntent)
                launched = true
            } else if (hasRoot) {
                try {
                    Runtime.getRuntime().exec(
                        arrayOf("su", "-c", "monkey -p $TARGET_PACKAGE -c android.intent.category.LAUNCHER 1")
                    ).waitFor()
                    launched = true
                } catch (_: Exception) {}
            }

            Handler(Looper.getMainLooper()).post {
                val rootLabel = if (hasRoot) "Root: OK" else "Root: не обнаружен (Overlay-режим)"
                if (!launched) {
                    Toast.makeText(
                        this,
                        "Пакет $TARGET_PACKAGE не найден, таймер меню запущен ($rootLabel)",
                        Toast.LENGTH_LONG
                    ).show()
                }
                startCountdown(statusView, startButton, rootLabel)
            }
        }.start()
    }

    private fun startCountdown(statusView: TextView, startButton: Button, rootLabel: String) {
        timer?.cancel()
        timer = object : CountDownTimer(DELAY_MS, 1000L) {
            override fun onTick(millisUntilFinished: Long) {
                val sec = millisUntilFinished / 1000L
                statusView.text = "Standoff 2 запущен ($rootLabel)\nДо появления меню: $sec сек..."
            }

            override fun onFinish() {
                statusView.text = "Меню HantoO активно поверх экрана."
                startButton.text = "МЕНЮ ЗАПУЩЕНО"
                showOriginalOverlayMenu()
            }
        }.start()
    }

    @SuppressLint("SetJavaScriptEnabled")
    private fun showOriginalOverlayMenu() {
        if (isOverlayShowing) return
        windowManager = getSystemService(WINDOW_SERVICE) as WindowManager

        val layoutType = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
        } else {
            @Suppress("DEPRECATION")
            WindowManager.LayoutParams.TYPE_PHONE
        }

        val density = resources.displayMetrics.density
        val expandedWidth = (310 * density).toInt()

        overlayParams = WindowManager.LayoutParams(
            expandedWidth,
            WindowManager.LayoutParams.MATCH_PARENT,
            layoutType,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                    WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.TOP or Gravity.START
            x = 0
            y = 0
        }

        overlayWebView = WebView(applicationContext).apply {
            setBackgroundColor(Color.TRANSPARENT)
            setLayerType(View.LAYER_TYPE_HARDWARE, null)
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.allowFileAccess = true
            webChromeClient = WebChromeClient()
            addJavascriptInterface(MenuJsBridge(), "HantoNative")

            webViewClient = object : WebViewClient() {
                override fun onPageFinished(view: WebView?, url: String?) {
                    super.onPageFinished(view, url)
                    // Подключаем перехват оригинальных функций toggleMenu и toggleFeat без изменения их дизайна
                    val hookJs = """
                        (function() {
                            var origToggleMenu = window.toggleMenu;
                            if (typeof origToggleMenu === 'function') {
                                window.toggleMenu = function() {
                                    origToggleMenu();
                                    if (window.HantoNative) {
                                        window.HantoNative.onMenuStateChanged(!!window.menuOpen);
                                    }
                                };
                            }
                            var origToggleFeat = window.toggleFeat;
                            if (typeof origToggleFeat === 'function') {
                                window.toggleFeat = function(row, id) {
                                    origToggleFeat(row, id);
                                    var t = document.getElementById('t-' + id);
                                    var on = t ? t.classList.contains('on') : false;
                                    if (window.HantoNative) {
                                        window.HantoNative.onFeatureToggled(id, on);
                                    }
                                };
                            }
                            document.querySelectorAll('input[type=range]').forEach(function(slider) {
                                slider.addEventListener('change', function() {
                                    var valSpan = this.parentElement ? this.parentElement.querySelector('.slider-val') : null;
                                    var id = valSpan ? valSpan.id.replace('v-', '') : 'slider';
                                    if (window.HantoNative) {
                                        window.HantoNative.onSliderChanged(id, parseFloat(this.value) || 0);
                                    }
                                });
                            });
                        })();
                    """.trimIndent()
                    view?.evaluateJavascript(hookJs, null)
                }
            }

            val assetList = assets.list("") ?: emptyArray()
            val targetHtml = when {
                assetList.contains("original_menu.html") -> "file:///android_asset/original_menu.html"
                assetList.contains("menu.html") -> "file:///android_asset/menu.html"
                else -> "file:///android_asset/original_menu.html"
            }
            loadUrl(targetHtml)
        }

        windowManager?.addView(overlayWebView, overlayParams)
        isOverlayShowing = true
    }

    inner class MenuJsBridge {
        @JavascriptInterface
        fun onMenuStateChanged(isOpen: Boolean) {
            Handler(Looper.getMainLooper()).post {
                val wm = windowManager ?: return@post
                val view = overlayWebView ?: return@post
                val params = overlayParams ?: return@post
                val density = resources.displayMetrics.density
                if (isOpen) {
                    params.width = (310 * density).toInt()
                    params.height = WindowManager.LayoutParams.MATCH_PARENT
                } else {
                    // Когда меню свёрнуто, оставляем кликабельным только фиолетовый ярлык #tab (18x72px)
                    params.width = (28 * density).toInt()
                    params.height = (105 * density).toInt()
                }
                wm.updateViewLayout(view, params)
            }
        }

        @JavascriptInterface
        fun onFeatureToggled(id: String, enabled: Boolean) {
            NativeBridge.setFeatureToggle(id, enabled)
        }

        @JavascriptInterface
        fun onSliderChanged(id: String, value: Float) {
            NativeBridge.setSliderValue(id, value)
        }

        @JavascriptInterface
        fun getArchiveInfo(): String {
            return NativeBridge.getArchiveMetadata()
        }
    }
}
