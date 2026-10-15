package com.hanto0.standoff

import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {
    private const val PKG = "com.axlebolt.standoff2"

    /* dump.cs offsets */
    private const val OFF_PC_TRANSFORM  = 0x100L
    private const val OFF_PC_HIT_CTRL  = 0xA8L
    private const val OFF_PC_CAM       = 0xE8L
    private const val OFF_HC_HEALTH    = 0x30L
    private const val OFF_TR_POSITION  = 0x10L
    private const val OFF_CP_PC        = 0x50L   /* CharacterPlayer->PlayerController */
    private const val OFF_PP_TAG       = 0x40L   /* PhotonPlayer->TagObject */
    private const val OFF_PP_NAME      = 0x20L
    private const val OFF_PP_ISLOCAL   = 0x30L
    private const val OFF_NP_ACTORS    = 0x1C0L  /* NetworkingPeer->mActors (Hashtable) */
    private const val OFF_NP_PLIST     = 0x1D0L  /* mPlayerListCopy */
    private const val OFF_PN_PEER      = 0x18L   /* PhotonNetwork->networkingPeer */
    private const val OFF_CAM_POS      = 0x10L
    private const val OFF_CAM_YAW      = 0x20L
    private const val OFF_CAM_PITCH    = 0x24L

    private var pid       = -1
    private var mem: RandomAccessFile? = null
    private var libBase   = 0L
    private var pnStaticFields = 0L
    private var camAddr   = 0L

    data class Vec3(val x: Float, val y: Float, val z: Float)
    data class PlayerData(
        val addr: Long, val pos: Vec3, val bones: List<Vec3>,
        val health: Int, val team: Int, val name: String, val isLocal: Boolean
    )

    fun init(): Boolean {
        pid = getPid()
        if (pid < 0) return false
        return try {
            /* KernelSU: use su to chmod mem readable */
            Runtime.getRuntime().exec(arrayOf("su", "-c", "chmod 777 /proc/$pid/mem")).waitFor()
            mem     = RandomAccessFile("/proc/$pid/mem", "rw")
            libBase = getLibBase()
            pnStaticFields = findPhotonNetworkStatic()
            true
        } catch (e: Exception) { false }
    }

    fun close() { try { mem?.close() } catch (_: Exception) {}; mem = null }

    private fun getPid(): Int {
        File("/proc").listFiles()?.forEach { f ->
            val n = f.name.toIntOrNull() ?: return@forEach
            try {
                val cmd = File("/proc/$n/cmdline").readBytes()
                    .takeWhile { it != 0.toByte() }
                    .toByteArray().toString(Charsets.UTF_8)
                if (cmd == PKG) return n
            } catch (_: Exception) {}
        }
        return -1
    }

    private fun getLibBase(): Long = try {
        File("/proc/$pid/maps").readLines()
            .firstOrNull { it.contains("libil2cpp.so") && it.contains("r-xp") }
            ?.split("-")?.get(0)?.toLong(16) ?: 0L
    } catch (_: Exception) { 0L }

    /* ── memory ── */
    fun readBytes(addr: Long, len: Int): ByteArray {
        val b = ByteArray(len)
        return try { mem!!.seek(addr); mem!!.read(b); b }
        catch (_: Exception) { b }
    }
    fun readInt(a: Long)   = ByteBuffer.wrap(readBytes(a,4)).order(ByteOrder.LITTLE_ENDIAN).int
    fun readLong(a: Long)  = ByteBuffer.wrap(readBytes(a,8)).order(ByteOrder.LITTLE_ENDIAN).long
    fun readFloat(a: Long) = java.lang.Float.intBitsToFloat(readInt(a))
    fun writeInt(a: Long, v: Int) {
        val b = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(v).array()
        try { mem!!.seek(a); mem!!.write(b) } catch (_: Exception) {}
    }
    fun writeFloat(a: Long, v: Float) = writeInt(a, java.lang.Float.floatToIntBits(v))
    fun readVec3(a: Long)  = Vec3(readFloat(a), readFloat(a+4), readFloat(a+8))
    fun readString16(a: Long): String {
        if (a == 0L) return ""
        return try {
            val len = readInt(a + 0x10).coerceIn(0, 32)
            if (len == 0) return ""
            val b = readBytes(a + 0x14, len * 2)
            String(b, Charsets.UTF_16LE)
        } catch (_: Exception) { "" }
    }

    /* ── PhotonNetwork static fields via il2cpp class scan ── */
    private fun findPhotonNetworkStatic(): Long {
        if (libBase == 0L) return 0L
        return try {
            /* read domain ptr from il2cpp_domain_get branch */
            val fnAddr   = libBase + 0x6327B70L
            val instr    = readInt(fnAddr)
            val offset   = ((instr and 0x3FFFFFF).toLong() shl 2).let {
                if (it and 0x8000000L != 0L) it or (-0x10000000L) else it
            }
            val domainPtr = readLong(fnAddr + offset)
            if (domainPtr == 0L) return 0L

            val asmCount = readInt(domainPtr + 0x18)
            val asmArr   = readLong(domainPtr + 0x10)
            for (i in 0 until minOf(asmCount, 512)) {
                val imgPtr     = readLong(asmArr + i * 8L)
                if (imgPtr == 0L) continue
                val classCount = readInt(imgPtr + 0x24)
                val typeStart  = readInt(imgPtr + 0x28)
                for (j in 0 until minOf(classCount, 16384)) {
                    val klass   = readLong(imgPtr + 0x40 + (typeStart + j) * 8L)
                    if (klass == 0L) continue
                    val namePtr = readLong(klass + 0x198L)
                    if (namePtr == 0L) continue
                    val name    = readCStr(namePtr)
                    if (name == "PhotonNetwork")
                        return readLong(klass + 0x40L)
                }
            }
            0L
        } catch (_: Exception) { 0L }
    }

    private fun readCStr(a: Long): String {
        if (a == 0L) return ""
        val sb = StringBuilder()
        for (i in 0 until 64) {
            val c = readInt(a + i) and 0xFF
            if (c == 0) break
            sb.append(c.toChar())
        }
        return sb.toString()
    }

    /* ── camera ── */
    fun getCamPos():   Vec3  = if (camAddr==0L) Vec3(0f,0f,0f) else readVec3(camAddr+OFF_CAM_POS)
    fun getCamYaw():   Float = if (camAddr==0L) 0f else readFloat(camAddr+OFF_CAM_YAW)
    fun getCamPitch(): Float = if (camAddr==0L) 0f else readFloat(camAddr+OFF_CAM_PITCH)
    fun writeCamAngles(yaw: Float, pitch: Float) {
        if (camAddr==0L) return
        writeFloat(camAddr+OFF_CAM_YAW, yaw)
        writeFloat(camAddr+OFF_CAM_PITCH, pitch)
    }

    /* ── players ── */
    fun getPlayers(): List<PlayerData> {
        if (pnStaticFields == 0L) { pnStaticFields = findPhotonNetworkStatic() }
        if (pnStaticFields == 0L) return emptyList()

        val peerPtr = readLong(pnStaticFields + OFF_PN_PEER)
        if (peerPtr == 0L) return emptyList()

        val listArr = readLong(peerPtr + OFF_NP_PLIST)
        if (listArr == 0L) return emptyList()

        val count = readInt(listArr + 0x18)
        val items = readLong(listArr + 0x20)
        if (count <= 0 || count > 32 || items == 0L) return emptyList()

        val result = mutableListOf<PlayerData>()
        for (i in 0 until count) {
            val ppPtr = readLong(items + i * 8L)
            if (ppPtr == 0L) continue

            val isLocal = readInt(ppPtr + OFF_PP_ISLOCAL) != 0
            val name    = readString16(readLong(ppPtr + OFF_PP_NAME))

            /* PhotonPlayer->TagObject->CharacterPlayer->PlayerController */
            val tagPtr  = readLong(ppPtr + OFF_PP_TAG)
            if (tagPtr == 0L) continue
            val cpPtr   = readLong(tagPtr + 0x10L)
            if (cpPtr == 0L) continue
            val pcPtr   = readLong(cpPtr + OFF_CP_PC)
            if (pcPtr == 0L) continue

            val trPtr   = readLong(pcPtr + OFF_PC_TRANSFORM)
            if (trPtr == 0L) continue
            val pos     = readVec3(trPtr + OFF_TR_POSITION)

            val hcPtr   = readLong(pcPtr + OFF_PC_HIT_CTRL)
            if (hcPtr == 0L) continue
            val hp      = readInt(hcPtr + OFF_HC_HEALTH)
            if (hp <= 0 || hp > 200) continue

            val team    = readInt(pcPtr + 0x38L)

            if (isLocal && camAddr == 0L) {
                val camPtr = readLong(pcPtr + OFF_PC_CAM)
                if (camPtr != 0L) camAddr = camPtr
            }

            result.add(PlayerData(pcPtr, pos, buildBones(pos), hp, team, name, isLocal))
        }
        return result
    }

    private fun buildBones(o: Vec3) = listOf(
        Vec3(o.x,o.y+1.8f,o.z), Vec3(o.x,o.y+1.5f,o.z),
        Vec3(o.x,o.y+1.2f,o.z), Vec3(o.x,o.y+0.9f,o.z),
        Vec3(o.x-.4f,o.y+1.4f,o.z), Vec3(o.x-.4f,o.y+1.1f,o.z), Vec3(o.x-.4f,o.y+0.8f,o.z),
        Vec3(o.x+.4f,o.y+1.4f,o.z), Vec3(o.x+.4f,o.y+1.1f,o.z), Vec3(o.x+.4f,o.y+0.8f,o.z),
        Vec3(o.x-.2f,o.y+0.8f,o.z), Vec3(o.x-.2f,o.y+0.45f,o.z), Vec3(o.x-.2f,o.y,o.z),
        Vec3(o.x+.2f,o.y+0.8f,o.z), Vec3(o.x+.2f,o.y+0.45f,o.z), Vec3(o.x+.2f,o.y,o.z)
    )
}
