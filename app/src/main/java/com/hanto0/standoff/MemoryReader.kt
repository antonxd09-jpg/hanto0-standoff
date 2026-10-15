package com.hanto0.standoff

import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {

    private const val PKG = "com.axlebolt.standoff2"

    /* offsets from dump.cs */
    private const val OFF_TRANSFORM      = 0x100L  // PlayerController->Transform
    private const val OFF_POSITION       = 0x10L   // Transform->position (Vector3)
    private const val OFF_HIT_CTRL       = 0xA8L   // PlayerController->PlayerHitController
    private const val OFF_HEALTH         = 0x30L   // HitController->_health
    private const val OFF_TEAM           = 0x38L   // ->team

    private var pid: Int = -1
    private var memFile: RandomAccessFile? = null

    /* ---- init ---- */
    fun init(): Boolean {
        pid = getPid(PKG)
        if (pid < 0) return false
        return try {
            memFile = RandomAccessFile("/proc/$pid/mem", "r")
            true
        } catch (e: Exception) { false }
    }

    fun close() { memFile?.close(); memFile = null }

    /* ---- pid ---- */
    private fun getPid(pkg: String): Int {
        File("/proc").listFiles()?.forEach { f ->
            if (!f.isDirectory) return@forEach
            val n = f.name.toIntOrNull() ?: return@forEach
            try {
                val cmd = File("/proc/$n/cmdline").readText().trimEnd('\u0000')
                if (cmd == pkg) return n
            } catch (_: Exception) {}
        }
        return -1
    }

    /* ---- primitive reads ---- */
    fun readInt(addr: Long): Int {
        val buf = ByteArray(4)
        val f = memFile ?: return 0
        return try { f.seek(addr); f.read(buf); ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).int }
        catch (_: Exception) { 0 }
    }

    fun readLong(addr: Long): Long {
        val buf = ByteArray(8)
        val f = memFile ?: return 0L
        return try { f.seek(addr); f.read(buf); ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).long }
        catch (_: Exception) { 0L }
    }

    fun readFloat(addr: Long): Float = java.lang.Float.intBitsToFloat(readInt(addr))

    data class Vec3(val x: Float, val y: Float, val z: Float)

    fun readVec3(addr: Long) = Vec3(readFloat(addr), readFloat(addr+4), readFloat(addr+8))

    /* ---- il2cpp base ---- */
    private fun getLibBase(): Long {
        return try {
            File("/proc/$pid/maps").readLines().firstOrNull {
                it.contains("libil2cpp.so") && it.contains("r-xp")
            }?.split("-")?.get(0)?.toLong(16) ?: 0L
        } catch (_: Exception) { 0L }
    }

    /* ---- entity list ---- */
    data class PlayerData(
        val addr: Long,
        val pos: Vec3,
        val health: Int,
        val team: Int
    )

    fun getPlayers(): List<PlayerData> {
        val base = getLibBase()
        if (base == 0L) return emptyList()

        /* ObjectManager static ptr — find via maps + known il2cpp pattern */
        /* Assumption: object list ptr at base+0x7A1234 — UPDATE with your real offset */
        val OBJ_MGR_STATIC = base + 0x7A1234L  // <-- замени на реальный из offset дампа

        val mgrPtr  = readLong(OBJ_MGR_STATIC)
        if (mgrPtr == 0L) return emptyList()

        val listPtr = readLong(mgrPtr + 0x10)
        if (listPtr == 0L) return emptyList()

        val count   = readInt(listPtr + 0x18)
        val items   = readLong(listPtr + 0x10)

        val result  = mutableListOf<PlayerData>()
        val cap     = minOf(count, 32)

        for (i in 0 until cap) {
            val objPtr = readLong(items + i * 8L)
            if (objPtr == 0L) continue

            val hp = readInt(objPtr + OFF_HIT_CTRL.let { readLong(objPtr + it) + OFF_HEALTH })
            if (hp <= 0) continue

            val transformPtr = readLong(objPtr + OFF_TRANSFORM)
            if (transformPtr == 0L) continue

            val pos  = readVec3(transformPtr + OFF_POSITION)
            val team = readInt(objPtr + OFF_TEAM)

            result.add(PlayerData(objPtr, pos, hp, team))
        }
        return result
    }
}
