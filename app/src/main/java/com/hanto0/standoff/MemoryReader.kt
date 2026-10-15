package com.hanto0.standoff

import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {
    private const val PKG = "com.axlebolt.standoff2"
    private const val IL2CPP_CLASS_STATIC_FIELDS = 0x40L
    private const val IL2CPP_CLASS_NAME = 0x198L
    private const val RVA_DOMAIN_GET = 0x6327B70L
    private const val PN_NETWORKING_PEER = 0x18L
    private const val NP_PLAYER_LIST_COPY = 0x1D0L
    private const val PP_NAME = 0x20L
    private const val PP_IS_LOCAL = 0x30L
    private const val CP_PLAYER_CONTROLLER = 0x50L
    private const val PC_TRANSFORM = 0x100L
    private const val PC_HIT_CTRL = 0xA8L
    private const val PC_TEAM = 0x38L
    private const val HC_HEALTH = 0x30L
    private const val TR_POSITION = 0x10L
    private const val PC_MAIN_CAM = 0xE8L
    private const val CAM_YAW = 0x20L
    private const val CAM_PITCH = 0x24L
    private const val CAM_POS = 0x10L

    private var pid: Int = -1
    private var mem: RandomAccessFile? = null
    private var libBase: Long = 0L

    data class Vec3(val x: Float, val y: Float, val z: Float)
    data class PlayerData(
        val addr: Long, val pos: Vec3, val bones: List<Vec3>,
        val health: Int, val team: Int, val name: String, val isLocal: Boolean
    )

    fun init(): Boolean {
        pid = getPid(PKG)
        if (pid < 0) return false
        return try {
            mem = RandomAccessFile("/proc/$pid/mem", "rw")
            libBase = getLibBase()
            true
        } catch (_: Exception) { false }
    }

    fun close() { mem?.close(); mem = null }

    private fun getPid(pkg: String): Int {
        File("/proc").listFiles()?.forEach { f ->
            val n = f.name.toIntOrNull() ?: return@forEach
            try {
                if (File("/proc/$n/cmdline").readText()
                        .trimEnd('\u0000') == pkg) return n
            } catch (_: Exception) {}
        }
        return -1
    }

    private fun getLibBase(): Long = try {
        File("/proc/$pid/maps").readLines()
            .firstOrNull { it.contains("libil2cpp.so") && it.contains("r-xp") }
            ?.split("-")?.get(0)?.toLong(16) ?: 0L
    } catch (_: Exception) { 0L }

    fun readInt(a: Long): Int {
        val b = ByteArray(4)
        return try {
            mem!!.seek(a); mem!!.read(b)
            ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).int
        } catch (_: Exception) { 0 }
    }

    fun readLong(a: Long): Long {
        val b = ByteArray(8)
        return try {
            mem!!.seek(a); mem!!.read(b)
            ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).long
        } catch (_: Exception) { 0L }
    }

    fun readFloat(a: Long) = java.lang.Float.intBitsToFloat(readInt(a))

    fun writeInt(a: Long, v: Int) {
        val b = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
            .putInt(v).array()
        try { mem!!.seek(a); mem!!.write(b) } catch (_: Exception) {}
    }

    fun writeFloat(a: Long, v: Float) = writeInt(a, java.lang.Float.floatToIntBits(v))

    fun readVec3(a: Long) = Vec3(readFloat(a), readFloat(a + 4), readFloat(a + 8))

    fun readString(a: Long): String {
        if (a == 0L) return ""
        return try {
            val len = readInt(a + 0x10)
            if (len <= 0 || len > 64) return ""
            val bytes = ByteArray(len * 2)
            mem!!.seek(a + 0x14); mem!!.read(bytes)
            String(bytes, Charsets.UTF_16LE)
        } catch (_: Exception) { "" }
    }

    private fun getPhotonNetworkStaticFields(): Long {
        if (libBase == 0L) return 0L
        return scanClassByName("PhotonNetwork")
    }

    private fun scanClassByName(name: String): Long {
        val domainBranch = libBase + RVA_DOMAIN_GET
        val branchInstr = readInt(domainBranch)
        val offset = ((branchInstr and 0x3FFFFFF).toLong() shl 2).let {
            if (it and 0x8000000L != 0L) it or -0x10000000L else it
        }
        val domainPtr = readLong(domainBranch + offset)
        if (domainPtr == 0L) return 0L
        val asmCount = readInt(domainPtr + 0x18)
        val asmArr = readLong(domainPtr + 0x10)
        for (i in 0 until minOf(asmCount, 256)) {
            val imgPtr = readLong(asmArr + i * 8L)
            if (imgPtr == 0L) continue
            val classCount = readInt(imgPtr + 0x24)
            val typeStart = readInt(imgPtr + 0x28)
            for (j in 0 until minOf(classCount, 8192)) {
                val klassPtr = readLong(imgPtr + 0x40 + (typeStart + j) * 8L)
                if (klassPtr == 0L) continue
                val namePtr = readLong(klassPtr + IL2CPP_CLASS_NAME)
                if (namePtr == 0L) continue
                val className = readCString(namePtr)
                if (className == name)
                    return readLong(klassPtr + IL2CPP_CLASS_STATIC_FIELDS)
            }
        }
        return 0L
    }

    private fun readCString(addr: Long): String {
        if (addr == 0L) return ""
        return try {
            val buf = StringBuilder()
            var i = 0
            while (i < 64) {
                val c = readInt(addr + i) and 0xFF
                if (c == 0) break
                buf.append(c.toChar()); i++
            }
            buf.toString()
        } catch (_: Exception) { "" }
    }

    private var camControllerAddr: Long = 0L

    fun getCamPos(): Vec3 {
        val c = camControllerAddr
        if (c == 0L) return Vec3(0f, 0f, 0f)
        return readVec3(c + CAM_POS)
    }

    fun getCamYaw(): Float {
        val c = camControllerAddr
        if (c == 0L) return 0f
        return readFloat(c + CAM_YAW)
    }

    fun getCamPitch(): Float {
        val c = camControllerAddr
        if (c == 0L) return 0f
        return readFloat(c + CAM_PITCH)
    }

    fun writeCamAngles(yaw: Float, pitch: Float) {
        val c = camControllerAddr
        if (c == 0L) return
        writeFloat(c + CAM_YAW, yaw)
        writeFloat(c + CAM_PITCH, pitch)
    }

    fun getPlayers(): List<PlayerData> {
        val sf = getPhotonNetworkStaticFields()
        if (sf == 0L) return emptyList()
        val peerPtr = readLong(sf + PN_NETWORKING_PEER)
        if (peerPtr == 0L) return emptyList()
        val listPtr = readLong(peerPtr + NP_PLAYER_LIST_COPY)
        if (listPtr == 0L) return emptyList()
        val count = readInt(listPtr + 0x18)
        val items = readLong(listPtr + 0x20)
        val result = mutableListOf<PlayerData>()
        for (i in 0 until minOf(count, 16)) {
            val ppPtr = readLong(items + i * 8L)
            if (ppPtr == 0L) continue
            val isLocal = readInt(ppPtr + PP_IS_LOCAL) != 0
            val pName = readString(readLong(ppPtr + PP_NAME))
            val cpPtr = readLong(ppPtr + 0x48L)
            if (cpPtr == 0L) continue
            val pcPtr = readLong(cpPtr + CP_PLAYER_CONTROLLER)
            if (pcPtr == 0L) continue
            val trPtr = readLong(pcPtr + PC_TRANSFORM)
            if (trPtr == 0L) continue
            val pos = readVec3(trPtr + TR_POSITION)
            val hcPtr = readLong(pcPtr + PC_HIT_CTRL)
            if (hcPtr == 0L) continue
            val hp = readInt(hcPtr + HC_HEALTH)
            if (hp <= 0) continue
            val team = readInt(pcPtr + PC_TEAM)
            if (camControllerAddr == 0L && isLocal) {
                camControllerAddr = readLong(pcPtr + PC_MAIN_CAM)
            }
            result.add(PlayerData(pcPtr, pos, buildBones(pos),
                hp, team, pName, isLocal))
        }
        return result
    }

    private fun buildBones(o: Vec3) = listOf(
        Vec3(o.x, o.y + 1.8f, o.z), Vec3(o.x, o.y + 1.5f, o.z),
        Vec3(o.x, o.y + 1.2f, o.z), Vec3(o.x, o.y + 0.9f, o.z),
        Vec3(o.x - .4f, o.y + 1.4f, o.z), Vec3(o.x - .4f, o.y + 1.1f, o.z),
        Vec3(o.x - .4f, o.y + 0.8f, o.z), Vec3(o.x + .4f, o.y + 1.4f, o.z),
        Vec3(o.x + .4f, o.y + 1.1f, o.z), Vec3(o.x + .4f, o.y + 0.8f, o.z),
        Vec3(o.x - .2f, o.y + 0.8f, o.z), Vec3(o.x - .2f, o.y + 0.45f, o.z),
        Vec3(o.x - .2f, o.y, o.z), Vec3(o.x + .2f, o.y + 0.8f, o.z),
        Vec3(o.x + .2f, o.y + 0.45f, o.z), Vec3(o.x + .2f, o.y, o.z)
    )
}
