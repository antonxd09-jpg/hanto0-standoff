package com.hanto0.standoff

import android.graphics.PointF
import kotlin.math.*

object Projection {

    data class CamData(
        val pos: MemoryReader.Vec3,
        val yaw: Float,
        val pitch: Float,
        val fov: Float,
        val sw: Int,
        val sh: Int
    )

    fun worldToScreen(world: MemoryReader.Vec3, cam: CamData): PointF? {
        val dx = world.x - cam.pos.x
        val dy = world.y - cam.pos.y
        val dz = world.z - cam.pos.z

        val yr = Math.toRadians(cam.yaw.toDouble())
        val pr = Math.toRadians(cam.pitch.toDouble())

        val camX = dx * cos(yr) - dz * sin(yr)
        val camZ = dx * sin(yr) + dz * cos(yr)
        val camY = dy * cos(pr) - camZ * sin(pr)
        val camD = dy * sin(pr) + camZ * cos(pr)

        if (camD < 0.1) return null

        val half = (cam.fov * 0.5).let { tan(Math.toRadians(it)) }
        val scale = (cam.sw * 0.5) / half

        val sx = (cam.sw * 0.5 + camX / camD * scale).toFloat()
        val sy = (cam.sh * 0.5 - camY / camD * scale).toFloat()

        return PointF(sx, sy)
    }
}
