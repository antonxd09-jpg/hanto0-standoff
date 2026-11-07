package com.hanto.standoff
import android.content.Context
import android.graphics.*
import android.view.SurfaceHolder
import android.view.SurfaceView

class EspView(ctx: Context) : SurfaceView(ctx), SurfaceHolder.Callback, Runnable {
    private val paint = Paint().apply { style = Paint.Style.STROKE; strokeWidth = 3f; isAntiAlias = true }
    private val fill = Paint().apply { style = Paint.Style.FILL }
    private val txt = Paint().apply { color = Color.WHITE; textSize = 28f; isAntiAlias = true }
    private var running = false
    init { holder.addCallback(this) }
    override fun surfaceCreated(h: SurfaceHolder) { running = true; Thread(this).start() }
    override fun surfaceChanged(h: SurfaceHolder, f: Int, w: Int, hh: Int) {}
    override fun surfaceDestroyed(h: SurfaceHolder) { running = false }
    override fun run() {
        while (running) {
            val c = holder.lockCanvas() ?: continue
            try { draw(c) } finally { holder.unlockCanvasAndPost(c) }
            Thread.sleep(16)
        }
    }
    private fun draw(c: Canvas) {
        c.drawColor(0, PorterDuff.Mode.CLEAR)
        val data = Native.getEntities()
        if (data.isEmpty()) return
        val s = 12; var i = 0
        while (i + s <= data.size) {
            val hx=data[i]; val hy=data[i+1]; val fx=data[i+2]; val fy=data[i+3]
            val hp=data[i+4]; val ammo=data[i+5]; val team=data[i+6].toInt()
            paint.color = if (team==0) Color.rgb(255,80,80) else Color.rgb(80,200,255)
            val bw = (fx-hx)*0.6f
            c.drawRect(hx-bw, hy, fx+bw, fy, paint)
            fill.color = Color.argb(200,0,255,0)
            val hpPx = (fy-hy)*(hp.coerceIn(0f,100f)/100f)
            c.drawRect(hx-bw-6, fy-hpPx, hx-bw-2, fy, fill)
            c.drawText("$ammo", hx, hy-10, txt)
            i += s
        }
    }
}