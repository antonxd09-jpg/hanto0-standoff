package com.hanto0.standoff

import android.content.Context
import android.graphics.*
import android.view.View

data class Entity(
    val x: Float, val y: Float,
    val w: Float, val h: Float,
    val team: Int,
    val bones: List<Pair<Float,Float>> = emptyList()
)

class EspView(context: Context) : View(context) {

    var entities: List<Entity> = emptyList()
    var showBox      = true
    var showLines    = true
    var showSkeleton = true

    private val paintRed = Paint().apply {
        color = Color.RED; strokeWidth = 3f; style = Paint.Style.STROKE; isAntiAlias = true
    }
    private val paintCyan = Paint().apply {
        color = Color.CYAN; strokeWidth = 3f; style = Paint.Style.STROKE; isAntiAlias = true
    }
    private val paintGreen = Paint().apply {
        color = Color.GREEN; strokeWidth = 2f; style = Paint.Style.STROKE; isAntiAlias = true
    }
    private val paintLine = Paint().apply {
        color = Color.YELLOW; strokeWidth = 2f; isAntiAlias = true
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val cx = width / 2f
        val cy = height.toFloat()

        for (e in entities) {
            val paint = if (e.team == 1) paintRed else paintCyan

            if (showBox) {
                canvas.drawRect(e.x, e.y, e.x + e.w, e.y + e.h, paint)
            }

            if (showLines) {
                canvas.drawLine(cx, cy, e.x + e.w / 2f, e.y + e.h, paintLine)
            }

            if (showSkeleton && e.bones.size >= 2) {
                val segments = listOf(
                    0 to 1, 1 to 2, 2 to 3,
                    1 to 4, 4 to 5, 5 to 6,
                    1 to 7, 7 to 8, 8 to 9,
                    3 to 10, 10 to 11, 11 to 12,
                    3 to 13, 13 to 14, 14 to 15
                )
                for ((a, b) in segments) {
                    if (a < e.bones.size && b < e.bones.size) {
                        canvas.drawLine(
                            e.bones[a].first, e.bones[a].second,
                            e.bones[b].first, e.bones[b].second,
                            paintGreen
                        )
                    }
                }
            }
        }
    }

    fun update(list: List<Entity>) {
        entities = list
        invalidate()
    }
}
