package com.hanto0.standoff

import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {
    private const val PKG = "com.axlebolt.standoff2"

    /* ── offsets from dump.cs ── */
    private const val OFF_TRANSFORM  = 0x100L
    private const val OFF_POSITION   = 0x10L
    private const val OFF_HIT_CTRL   = 0xA8L
    private const val OFF_HEALTH     = 0x30L
    private const val OFF_TEAM       = 0x38L
    /* cam offsets — TODO: update from dump.cs PlayerMainCamera */
    private const val OFF_CAM_YAW    = 0x20L
    private const val OFF_CAM_PITCH  = 0x24L

    /* ── object manager static offset in libil2cpp.so ──
       Replace 0x7A1234 with real offset from your il2cpp api dump */
    private const val OBJ_MGR_OFFSET = 0x7A1234L

    private var pid: Int = -1
    private var mem: RandomAccessFile? = null
    private var libBase: Long = 0L
    private var camAddr: Long = 0L

    data class Vec3(val x: Float, val y: Float, val z: Float)

    data class PlayerData(
        val addr: Long,
        val pos: Vec3,
        val bones: List<Vec3>,
        val health: Int,
        val team: Int
    )

    fun init(): Boolean {
        pid = getPid(PKG)
        if (pid < 0) return false
        return try {
            mem     = RandomAccessFile("/proc/$pid/mem", "rw")
            libBase = getLibBase()
            true
        } catch (_: Exception) { false }
    }

    fun close() { mem?.close(); mem = null }

    private fun getPid(pkg: String): Int {
        File("/proc").listFiles()?.forEach { f ->
            val n = f.name.toIntOrNull() ?: return@forEach
            try {
                if (File("/proc/$n/cmdline").readText().trimEnd('\u0000') == pkg) return n
            } catch (_: Exception) {}
        }
        return -1
    }

    private fun getLibBase(): Long = try {
        File("/proc/$pid/maps").readLines()
            .firstOrNull { it.contains("libil2cpp.so") && it.contains("r-xp") }
            ?.split("-")?.get(0)?.toLong(16) ?: 0L
    } catch (_: Exception) { 0L }

    fun readInt(addr: Long): Int {
        val b = ByteArray(4)
        return try { mem!!.seek(addr); mem!!.read(b); ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).int }
        catch (_: Exception) { 0 }
    }

    fun readLong(addr: Long): Long {
        val b = ByteArray(8)
        return try { mem!!.seek(addr); mem!!.read(b); ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).long }
        catch (_: Exception) { 0L }
    }

    fun writeInt(addr: Long, v: Int) {
        val b = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(v).array()
        try { mem!!.seek(addr); mem!!.write(b) } catch (_: Exception) {}
    }

    fun readFloat(addr: Long) = java.lang.Float.intBitsToFloat(readInt(addr))
    fun writeFloat(addr: Long, v: Float) = writeInt(addr, java.lang.Float.floatToIntBits(v))
    fun readVec3(addr: Long)  = Vec3(readFloat(addr), readFloat(addr+4), readFloat(addr+8))

    fun getCamPos(): Vec3 {
        if (camAddr == 0L) return Vec3(0f,0f,0f)
        return readVec3(camAddr + OFF_POSITION)
    }

    fun getCamYaw(): Float {
        if (camAddr == 0L) return 0f
        return readFloat(camAddr + OFF_CAM_YAW)
    }

    fun getCamPitch(): Float {
        if (camAddr == 0L) return 0f
        return readFloat(camAddr + OFF_CAM_PITCH)
    }

    fun writeCamAngles(yaw: Float, pitch: Float) {
        if (camAddr == 0L) return
        writeFloat(camAddr + OFF_CAM_YAW,   yaw)
        writeFloat(camAddr + OFF_CAM_PITCH, pitch)
    }

    fun getPlayers(): List<PlayerData> {
        if (libBase == 0L) return emptyList()
        val mgrPtr  = readLong(libBase + OBJ_MGR_OFFSET); if (mgrPtr==0L) return emptyList()
        val listPtr = readLong(mgrPtr + 0x10);             if (listPtr==0L) return emptyList()
        val count   = readInt(listPtr + 0x18)
        val items   = readLong(listPtr + 0x10)
        val result  = mutableListOf<PlayerData>()
        for (i in 0 until minOf(count, 32)) {
            val obj = readLong(items + i*8L); if (obj==0L) continue
            val hc  = readLong(obj + OFF_HIT_CTRL); if (hc==0L) continue
            val hp  = readInt(hc + OFF_HEALTH);     if (hp<=0) continue
            val tr  = readLong(obj + OFF_TRANSFORM); if (tr==0L) continue
            val pos = readVec3(tr + OFF_POSITION)
            val team= readInt(obj + OFF_TEAM)
            val bones = buildBones(pos)
            if (camAddr == 0L) camAddr = readLong(obj + 0xE8L)
            result.add(PlayerData(obj, pos, bones, hp, team))
        }
        return result
    }

    private fun buildBones(o: Vec3) = listOf(
        Vec3(o.x, o.y+1.8f, o.z), Vec3(o.x, o.y+1.5f, o.z),
        Vec3(o.x, o.y+1.2f, o.z), Vec3(o.x, o.y+0.9f, o.z),
        Vec3(o.x-.4f,o.y+1.4f,o.z), Vec3(o.x-.4f,o.y+1.1f,o.z), Vec3(o.x-.4f,o.y+0.8f,o.z),
        Vec3(o.x+.4f,o.y+1.4f,o.z), Vec3(o.x+.4f,o.y+1.1f,o.z), Vec3(o.x+.4f,o.y+0.8f,o.z),
        Vec3(o.x-.2f,o.y+0.8f,o.z), Vec3(o.x-.2f,o.y+0.45f,o.z), Vec3(o.x-.2f,o.y,o.z),
        Vec3(o.x+.2f,o.y+0.8f,o.z), Vec3(o.x+.2f,o.y+0.45f,o.z), Vec3(o.x+.2f,o.y,o.z)
    )
}
