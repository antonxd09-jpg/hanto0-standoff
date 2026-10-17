package com.hanto0.standoff

import android.util.Log
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {
    private const val TAG = "hanto0_esp"
    private const val PKG = "com.axlebolt.standoff2"

    // offsets для версии 1.0.0
    private const val OFF_PC_TRANSFORM = 0x100L
    private const val OFF_PC_HIT_CTRL  = 0xA8L
    private const val OFF_PC_CAM       = 0xE8L
    private const val OFF_HC_HEALTH    = 0x30L
    private const val OFF_TR_POSITION  = 0x10L
    private const val OFF_CP_PC        = 0x50L
    private const val OFF_PP_TAG       = 0x40L
    private const val OFF_PP_NAME      = 0x20L
    private const val OFF_PP_ISLOCAL   = 0x30L
    private const val OFF_NP_PLIST     = 0x1D0L
    private const val OFF_PN_PEER      = 0x18L
    private const val OFF_CAM_POS      = 0x10L
    private const val OFF_CAM_YAW      = 0x20L
    private const val OFF_CAM_PITCH    = 0x24L

    // оффсет PhotonNetwork для версии 1.0.0
    private const val OFF_PHOTON_FN    = 0x6B12A40L

    private var pid     = -1
    private var mem: RandomAccessFile? = null
    private var libBase = 0L
    private var pnSF    = 0L
    private var camAddr = 0L

    data class Vec3(val x: Float, val y: Float, val z: Float)
    data class PlayerData(
        val addr: Long, val pos: Vec3, val bones: List<Vec3>,
        val health: Int, val team: Int, val name: String, val isLocal: Boolean
    )

    fun init(): Boolean {
        pid = getPid()

        Log.d(TAG, "pid=$pid")
        if (pid < 0) return false
        return try {
            val su = Runtime.getRuntime().exec(arrayOf("su", "-c", "chmod 777 /proc/$pid/mem"))
            su.waitFor()
            mem     = RandomAccessFile("/proc/$pid/mem", "rw")
            libBase = getLibBase()
            Log.d(TAG, "libBase=0x${libBase.toString(16)}")
            pnSF    = findPhotonNetworkStatic()
            Log.d(TAG, "pnSF=0x${pnSF.toString(16)}")
            true
        } catch (e: Exception) { Log.e(TAG, "init fail: $e"); false }


    }

    
fun close() { try { mem?.close() } catch (_: Exception) {}; mem = null }


    private fun getPid(): Int {
        File("/proc").listFiles()?.forEach { f ->
            val n = f.name.toIntOrNull() ?: return@forEach
            try {
                val cmd = File("/proc/$n/cmdline").readBytes()
                    .takeWhile { it != 0.toByte() }.toByteArray().toString(Charsets.UTF_8)
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

    fun readBytes(addr: Long, len: Int): ByteArray {
        val b = ByteArray(len)
        return try { mem!!.seek(addr); mem!!.read(b); b } catch (_: Exception) { b }
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
            String(readBytes(a + 0x14, len * 2), Charsets.UTF_16LE)
        } catch (_: Exception) { "" }
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

    private fun findPhotonNetworkStatic(): Long {
        if (libBase == 0L) return 0L
        return try {
            // прямой оффсет для 1.0.0
            val staticAddr = libBase + OFF_PHOTON_FN
            val result = readLong(staticAddr)
            Log.d(TAG, "photon static direct=0x${result.toString(16)}")
            if (result != 0L) return result

            // fallback — сканим по имени класса
            scanForPhotonByName()
        } catch (e: Exception) {
            Log.e(TAG, "findPN fail: $e")
            scanForPhotonByName()
        }
    }

    private fun scanForPhotonByName(): Long {
        return try {
            val domain  = readLong(libBase + 0x6B12A40L)
            if (domain == 0L) return 0L
            val asmCount = readInt(domain + 0x18)
            val asmArr   = readLong(domain + 0x10)
            for (i in 0 until minOf(asmCount, 512)) {
                val img = readLong(asmArr + i * 8L); if (img == 0L) continue
                val cc  = readInt(img + 0x24)
                val ts  = readInt(img + 0x28)
                for (j in 0 until minOf(cc, 16384)) {
                    val klass = readLong(img + 0x40 + (ts + j) * 8L); if (klass == 0L) continue
                    val np    = readLong(klass + 0x198L); if (np == 0L) continue
                    if (readCStr(np) == "PhotonNetwork")
                        return readLong(klass + 0x40L)
                }
            }
            0L
        } catch (e: Exception) { 0L }
    }

    fun getCamPos():   Vec3  = if (camAddr==0L) Vec3(0f,0f,0f) else readVec3(camAddr+OFF_CAM_POS)
    fun getCamYaw():   Float = if (camAddr==0L) 0f else readFloat(camAddr+OFF_CAM_YAW)
    fun getCamPitch(): Float = if (camAddr==0L) 0f else readFloat(camAddr+OFF_CAM_PITCH)
    fun writeCamAngles(yaw: Float, pitch: Float) {
        if (camAddr==0L) return
        writeFloat(camAddr+OFF_CAM_YAW, yaw)
        writeFloat(camAddr+OFF_CAM_PITCH, pitch)
    }

    fun getPlayers(): List<PlayerData> {
        if (pnSF == 0L) { pnSF = findPhotonNetworkStatic() }
        if (pnSF == 0L) { Log.w(TAG, "pnSF still 0"); return emptyList() }
        val peerPtr = readLong(pnSF + OFF_PN_PEER)
        if (peerPtr == 0L) return emptyList()
        val listArr = readLong(peerPtr + OFF_NP_PLIST)
        if (listArr == 0L) return emptyList()
        val count = readInt(listArr + 0x18)
        val items = readLong(listArr + 0x20)
        if (count <= 0 || count > 32 || items == 0L) return emptyList()
        val result = mutableListOf<PlayerData>()
        for (i in 0 until count) {
            val ppPtr = readLong(items + i * 8L); if (ppPtr == 0L) continue
            val isLocal = readInt(ppPtr + OFF_PP_ISLOCAL) != 0
            val name    = readString16(readLong(ppPtr + OFF_PP_NAME))
            val tagPtr  = readLong(ppPtr + OFF_PP_TAG); if (tagPtr == 0L) continue
            val cpPtr   = readLong(tagPtr + 0x10L);     if (cpPtr == 0L) continue
            val pcPtr   = readLong(cpPtr + OFF_CP_PC);  if (pcPtr == 0L) continue
            val trPtr   = readLong(pcPtr + OFF_PC_TRANSFORM); if (trPtr == 0L) continue
            val pos     = readVec3(trPtr + OFF_TR_POSITION)
            val hcPtr   = readLong(pcPtr + OFF_PC_HIT_CTRL); if (hcPtr == 0L) continue
            val hp      = readInt(hcPtr + OFF_HC_HEALTH)
            if (hp <= 0 || hp > 200) continue
            val team    = readInt(pcPtr + 0x38L)
            if (isLocal && camAddr == 0L) {
                val cp = readLong(pcPtr + OFF_PC_CAM)
                if (cp != 0L) { camAddr = cp }
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
