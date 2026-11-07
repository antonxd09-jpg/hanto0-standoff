package com.hanto.standoff
import android.app.*
import android.content.Intent
import android.graphics.PixelFormat
import android.os.IBinder
import android.view.Gravity
import android.view.WindowManager

class OverlayService : Service() {
    private lateinit var wm: WindowManager
    private lateinit var esp: EspView
    private lateinit var menu: MenuView
    override fun onBind(i: Intent?): IBinder? = null
    override fun onCreate() {
        super.onCreate()
        val ch = NotificationChannel("hanto","hanto", NotificationManager.IMPORTANCE_LOW)
        (getSystemService(NOTIFICATION_SERVICE) as NotificationManager).createNotificationChannel(ch)
        startForeground(1, Notification.Builder(this,"hanto").setContentTitle("hanto v2").build())
        Native.attach(GameLauncher.pid())
        wm = getSystemService(WINDOW_SERVICE) as WindowManager
        esp = EspView(this)
        val p1 = WindowManager.LayoutParams(-1,-1, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE or WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN, PixelFormat.TRANSLUCENT)
        p1.gravity = Gravity.TOP or Gravity.START
        wm.addView(esp, p1)
        menu = MenuView(this) { wm.removeView(menu) }
        val p2 = WindowManager.LayoutParams((resources.displayMetrics.widthPixels * 0.9f).toInt(), -2, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, PixelFormat.TRANSLUCENT)
        p2.gravity = Gravity.CENTER
        wm.addView(menu, p2)
    }
    override fun onDestroy() {
        super.onDestroy()
        try { wm.removeView(esp) } catch (_:Exception){}
        try { wm.removeView(menu) } catch (_:Exception){}
        Native.detach()
    }
}