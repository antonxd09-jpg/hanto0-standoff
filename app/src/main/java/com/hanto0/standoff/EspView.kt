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
        color = Color.RED; style = Paint.Style.STROKE; strokeWidth = 2f
    }
    private val linePaint = Paint().apply {
        color = Color.YELLOW; strokeWidth = 1.5f
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
        color = Color.WHITE; textSize = 24f; isAntiAlias = true
    }
    private val aimPaint = Paint().apply {
        color = Color.RED; strokeWidth = 2f
    }

    fun update(p: List<MemoryReader.PlayerData>, c: Projection.CamData) {
        players = p; cam = c
        postInvalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val c = cam ?: return

        val cx = width / 2f
        val cy = height / 2f
        canvas.drawLine(cx - 15, cy, cx + 15, cy, aimPaint)
        canvas.drawLine(cx, cy - 15, cx, cy + 15, aimPaint)

        for (p in players) {
            if (p.isLocal) continue

            val head = MemoryReader.Vec3(p.pos.x, p.pos.y + 1.8f, p.pos.z)
            val feet = MemoryReader.Vec3(p.pos.x, p.pos.y, p.pos.z)

            val headScreen = Projection.worldToScreen(head, c)
            val feetScreen = Projection.worldToScreen(feet, c)

            if (!headScreen.visible || !feetScreen.visible) continue

            val boxH = feetScreen.y - headScreen.y
            val boxW = boxH * 0.45f
            val left = headScreen.x - boxW / 2
            val top = headScreen.y

            if (showBox) {
                canvas.drawRect(left, top, left + boxW, top + boxH, boxPaint)
            }

            if (showLines) {
                canvas.drawLine(cx, height.toFloat(), headScreen.x,
                    headScreen.y, linePaint)
            }

            if (showHP) {
                val hpRatio = p.health / 100f
                val barW = 4f
                val barX = left - 8f
                canvas.drawRect(barX, top, barX + barW, top + boxH, hpBgPaint)
                canvas.drawRect(barX, top + boxH * (1 - hpRatio),
                    barX + barW, top + boxH, hpPaint)
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
                        if (ba.visible && bb.visible) {
                            canvas.drawLine(ba.x, ba.y, bb.x, bb.y, skelPaint)
                        }
                    }
                }
            }

            if (showInfo) {
                val dist = Projection.distance(c.pos, p.pos)
                canvas.drawText("${p.name} [${dist.toInt()}m]",
                    left, top - 10, textPaint)
            }
        }
    }
}
