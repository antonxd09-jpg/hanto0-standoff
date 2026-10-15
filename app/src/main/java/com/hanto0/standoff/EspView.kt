package com.hanto0.standoff

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.view.View

class EspView(context: Context) : View(context) {
    var showBox = true
    var showLines = true
    var showSkeleton = true
    var showHP = true
    var showInfo = true

    private var players: List<MemoryReader.PlayerData> = emptyList()
    private var cam: Projection.CamData? = null

    private val boxPaint = Paint().apply {
        color = Color.RED; style = Paint.Style.STROKE; strokeWidth = 3f
    }
    private val linePaint = Paint().apply {
        color = Color.YELLOW; strokeWidth = 2f
    }
    private val skelPaint = Paint().apply {
        color = Color.WHITE; strokeWidth = 2f
    }
    private val hpPaint = Paint().apply {
        color = Color.GREEN; style = Paint.Style.FILL
    }
    private val hpBgPaint = Paint().apply {
        color = Color.DKGRAY; style = Paint.Style.FILL
    }
    private val textPaint = Paint().apply {
        color = Color.WHITE; textSize = 28f; isAntiAlias = true
    }
    private val aimPaint = Paint().apply {
        color = Color.RED; strokeWidth = 2f
    }
    private val statusPaint = Paint().apply {
        color = Color.GREEN; textSize = 32f; isAntiAlias = true
        setShadowLayer(4f, 2f, 2f, Color.BLACK)
    }

    fun update(p: List<MemoryReader.PlayerData>, c: Projection.CamData) {
        players = p; cam = c
        postInvalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        canvas.drawText("[${MemoryReader.status}]", 20f, 60f, statusPaint)

        val c = cam ?: return
        val cx = width / 2f
        val cy = height / 2f
        canvas.drawLine(cx - 20, cy, cx + 20, cy, aimPaint)
        canvas.drawLine(cx, cy - 20, cx, cy + 20, aimPaint)

        for (p in players) {
            if (p.isLocal) continue

            val head = MemoryReader.Vec3(p.pos.x, p.pos.y + 1.8f, p.pos.z)
            val feet = MemoryReader.Vec3(p.pos.x, p.pos.y, p.pos.z)

            val hs = Projection.worldToScreen(head, c)
            val fs = Projection.worldToScreen(feet, c)
            if (!hs.visible || !fs.visible) continue

            val boxH = fs.y - hs.y
            val boxW = boxH * 0.5f
            val left = hs.x - boxW / 2
            val top = hs.y

            if (showBox) canvas.drawRect(left, top, left + boxW, top + boxH, boxPaint)

            if (showLines) canvas.drawLine(cx, height.toFloat(), hs.x, hs.y, linePaint)

            if (showHP) {
                val ratio = (p.health / 100f).coerceIn(0f, 1f)
                val barX = left - 12f
                canvas.drawRect(barX, top, barX + 6f, top + boxH, hpBgPaint)
                canvas.drawRect(barX, top + boxH * (1 - ratio),
                    barX + 6f, top + boxH, hpPaint)
            }

            if (showSkeleton) {
                val bones = p.bones.map { Projection.worldToScreen(it, c) }
                val pairs = listOf(
                    0 to 1, 1 to 2, 2 to 3,
                    1 to 4, 4 to 5, 5 to 6,
                    1 to 7, 7 to 8, 8 to 9,
                    3 to 10, 3 to 11,
                    3 to 12, 3 to 13,
                    3 to 14, 3 to 15
                )
                for ((a, b) in pairs) {
                    if (a < bones.size && b < bones.size) {
                        val ba = bones[a]; val bb = bones[b]
                        if (ba.visible && bb.visible)
                            canvas.drawLine(ba.x, ba.y, bb.x, bb.y, skelPaint)
                    }
                }
            }

            if (showInfo) {
                val dist = Projection.distance(c.pos, p.pos)
                canvas.drawText("${p.name} [${dist.toInt()}m]",
                    left - 20, top - 12, textPaint)
            }
        }
    }
}
