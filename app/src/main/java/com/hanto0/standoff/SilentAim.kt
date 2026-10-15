package com.hanto0.standoff

import kotlin.math.*

object SilentAim {
    var enabled = true
    private const val FOV = 30f
    private const val SMOOTH = 0.35f

    data class Angles(val yaw: Float, val pitch: Float)

    fun calc(
        camPos: MemoryReader.Vec3,
        camYaw: Float,
        camPitch: Float,
        players: List<MemoryReader.PlayerData>
    ): Angles? {
        if (!enabled) return null

        var best: MemoryReader.PlayerData? = null
        var bestDist = Float.MAX_VALUE

        for (p in players) {
            if (p.isLocal) continue
            if (p.health <= 0) continue

            val head = MemoryReader.Vec3(p.pos.x, p.pos.y + 1.6f, p.pos.z)
            val d = Projection.distance(camPos, head)
            if (d > 200f) continue

            val dx = head.x - camPos.x
            val dy = head.y - camPos.y
            val dz = head.z - camPos.z

            val targetYaw = Math.toDegrees(atan2(dx, dz).toDouble()).toFloat()
            val horiz = sqrt(dx * dx + dz * dz)
            val targetPitch = Math.toDegrees(-atan2(dy, horiz).toDouble()).toFloat()

            val yawDiff = normalizeAngle(targetYaw - camYaw)
            val pitchDiff = normalizeAngle(targetPitch - camPitch)

            if (abs(yawDiff) > FOV || abs(pitchDiff) > FOV) continue

            val angleDist = sqrt(yawDiff * yawDiff + pitchDiff * pitchDiff)
            if (angleDist < bestDist) {
                bestDist = angleDist
                best = p
            }
        }

        val target = best ?: return null
        val head = MemoryReader.Vec3(target.pos.x, target.pos.y + 1.6f, target.pos.z)

        val dx = head.x - camPos.x
        val dy = head.y - camPos.y
        val dz = head.z - camPos.z

        val targetYaw = Math.toDegrees(atan2(dx, dz).toDouble()).toFloat()
        val horiz = sqrt(dx * dx + dz * dz)
        val targetPitch = Math.toDegrees(-atan2(dy, horiz).toDouble()).toFloat()

        val newYaw = camYaw + normalizeAngle(targetYaw - camYaw) * SMOOTH
        val newPitch = camPitch + normalizeAngle(targetPitch - camPitch) * SMOOTH

        return Angles(newYaw, newPitch)
    }

    private fun normalizeAngle(a: Float): Float {
        var x = a
        while (x > 180f) x -= 360f
        while (x < -180f) x += 360f
        return x
    }
}
