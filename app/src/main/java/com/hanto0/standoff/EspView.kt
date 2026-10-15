package com.hanto0.standoff

import android.content.Context
import android.graphics.*
import android.view.View

class EspView(context: Context) : View(context) {

    var players: List<MemoryReader.PlayerData> = emptyList()
    var camData: Projection.CamData? = null
    var showBox      = true
    var showLines    = true
    var showSkeleton = true

    private val paintRed  = Paint().apply { color=Color.RED;  strokeWidth=3f; style=Paint.Style.STROKE; isAntiAlias=true }
    private val paintCyan = Paint().apply { color=Color.CYAN; strokeWidth=3f; style=Paint.Style.STROKE; isAntiAlias=true }
    private val paintLine = Paint().apply { color=Color.YELLOW; strokeWidth=2f; isAntiAlias=true }
    private val paintText = Paint().apply { color=Color.WHITE; textSize=28f; isAntiAlias=true }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val cam = camData ?: return
        val cx = width / 2f; val cy = height.toFloat()

        for (p in players) {
            val head = MemoryReader.Vec3(p.pos.x, p.pos.y + 1.8f, p.pos.z)
            val feet = p.pos

            val sHead = Projection.worldToScreen(head, cam) ?: continue
            val sFeet = Projection.worldToScreen(feet, cam) ?: continue

            val paint = if (p.team == 1) paintRed else paintCyan
            val h = abs(sFeet.y - sHead.y)
            val w = h / 3f

            if (showBox) {
                canvas.drawRect(sHead.x - w, sHead.y, sHead.x + w, sFeet.y, paint)
            }
            if (showLines) {
                canvas.drawLine(cx, cy, sHead.x, sHead.y + h/2, paintLine)
            }

            canvas.drawText("HP:${p.health}", sHead.x - w, sHead.y - 5f, paintText)
        }
    }

    fun update(list: List<MemoryReader.PlayerData>, cam: Projection.CamData?) {
        players = list
        camData = cam
        invalidate()
    }
}
