package com.hanto0.standoff

import kotlin.math.*

object Projection {
    data class CamData(
        val pos: MemoryReader.Vec3,
        val yaw: Float,
        val pitch: Float,
        val fov: Float,
        val screenW: Int,
        val screenH: Int
    )

    data class ScreenPoint(val x: Float, val y: Float, val visible: Boolean)

    fun worldToScreen(world: MemoryReader.Vec3, cam: CamData): ScreenPoint {
        val dx = world.x - cam.pos.x
        val dy = world.y - cam.pos.y
        val dz = world.z - cam.pos.z

        val yawRad = Math.toRadians(cam.yaw.toDouble())
        val pitchRad = Math.toRadians(cam.pitch.toDouble())

        val cosYaw = cos(yawRad)
        val sinYaw = sin(yawRad)
        val cosPitch = cos(pitchRad)
        val sinPitch = sin(pitchRad)

        val x1 = dx * cosYaw - dz * sinYaw
        val z1 = dx * sinYaw + dz * cosYaw
        val y2 = dy * cosPitch - z1 * sinPitch
        val z2 = dy * sinPitch + z1 * cosPitch

        if (z2 <= 0.01) return ScreenPoint(0f, 0f, false)

        val fovRad = Math.toRadians(cam.fov.toDouble())
        val focal = (cam.screenH / 2.0) / tan(fovRad / 2.0)

        val sx = (cam.screenW / 2.0 + (x1 / z2) * focal).toFloat()
        val sy = (cam.screenH / 2.0 - (y2 / z2) * focal).toFloat()

        return ScreenPoint(sx, sy, true)
    }

    fun distance(a: MemoryReader.Vec3, b: MemoryReader.Vec3): Float {
        val dx = a.x - b.x; val dy = a.y - b.y; val dz = a.z - b.z
        return sqrt(dx * dx + dy * dy + dz * dz)
    }
}
