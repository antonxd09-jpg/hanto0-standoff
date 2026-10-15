package com.hanto0.standoff

import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
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
    private var libBase: Long = 0L
    private var su: Process? = null
    private var suIn: OutputStream? = null
    private var suOut: BufferedReader? = null
    private var counter = 0
    private var cachedPhotonSF: Long = 0L

    @Volatile var status: String = "not init"

    data class Vec3(val x: Float, val y: Float, val z: Float)
    data class PlayerData(
        val addr: Long, val pos: Vec3, val bones: List<Vec3>,
        val health: Int, val team: Int, val name: String, val isLocal: Boolean
    )

    fun init(): Boolean {
        if (!openSu()) { status = "NO ROOT"; return false }
        pid = findPid()
        if (pid < 0) { status = "game not running"; return false }
        libBase = findLibBase()
        if (libBase == 0L) { status = "libbase fail"; return false }
        status = "OK pid=$pid"
        return true
    }

    fun close() {
        try { suIn?.write("exit\n".toByteArray()); suIn?.flush() } catch (_: Exception) {}
        try { su?.destroy() } catch (_: Exception) {}
        su = null; suIn = null; suOut = null
    }

    private fun openSu(): Boolean {
        val paths = listOf("su", "/system/bin/su", "/system/xbin/su", "/sbin/su")
        for (p in paths) {
            try {
                val proc = ProcessBuilder(p).redirectErrorStream(true).start()
                val inS = proc.outputStream
                val outS = BufferedReader(InputStreamReader(proc.inputStream))
                inS.write("id\n".toByteArray()); inS.flush()
                val line = outS.readLine() ?: continue
                if (line.contains("uid=0")) {
                    su = proc; suIn = inS; suOut = outS
                    return true
                } else { proc.destroy() }
            } catch (_: Exception) {}
        }
        return false
    }

    private fun sh(cmd: String): String {
        val ins = suIn ?: return ""
        val outs = suOut ?: return ""
        return try {
            val id = (++counter).toString()
            val marker = "__E_${id}__"
            ins.write("($cmd) 2>/dev/null; echo $marker\n".toByteArray())
            ins.flush()
            val sb = StringBuilder()
            var first = true
            while (true) {
                val line = outs.readLine() ?: break
                if (line == marker) break
                if (!first) sb.append('\n')
                sb.append(line); first = false
            }
            sb.toString()
        } catch (_: Exception) { "" }
    }

    private fun findPid(): Int {
        val script = "for d in /proc/[0-9]*; do n=\${d#/proc/}; c=\$(cat \$d/cmdline 2>/dev/null | tr -d '\\000'); if [ \"\$c\" = \"$PKG\" ]; then echo \$n; break; fi; done"
        val out = sh(script).trim()
        return out.lines().firstOrNull()?.toIntOrNull() ?: -1
    }

    private fun findLibBase(): Long {
        val out = sh("grep libil2cpp.so /proc/$pid/maps 2>/dev/null | head -1").trim()
        if (out.isEmpty()) return 0L
        return out.substringBefore("-").trim().toLongOrNull(16) ?: 0L
    }

    private fun readBytes(addr: Long, size: Int): ByteArray {
        if (pid < 0 || size <= 0) return ByteArray(0)
        val hex = sh("dd if=/proc/$pid/mem bs=1 skip=$addr count=$size 2>/dev/null | od -An -tx1 | tr -d ' \\n'").trim()
        if (hex.length < size * 2) return ByteArray(0)
        val b = ByteArray(size)
        for (i in 0 until size) {
            b[i] = ((hex[i * 2].digitToIntOrNull(16) ?: 0) shl 4 or
                    (hex[i * 2 + 1].digitToIntOrNull(16) ?: 0)).toByte()
        }
        return b
    }

    private fun writeBytes(addr: Long, data: ByteArray) {
        if (pid < 0 || data.isEmpty()) return
        val sb = StringBuilder()
        for (b in data) sb.append("\\x%02x".format(b.toInt() and 0xFF))
        sh("echo -ne '$sb' | dd of=/proc/$pid/mem bs=1 seek=$addr conv=notrunc 2>/dev/null")
    }

    fun readInt(a: Long): Int {
        val b = readBytes(a, 4)
        if (b.size < 4) return 0
        return ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).int
    }

    fun readLong(a: Long): Long {
        val b = readBytes(a, 8)
        if (b.size < 8) return 0L
        return ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).long
    }

    fun readFloat(a: Long) = java.lang.Float.intBitsToFloat(readInt(a))

    fun writeInt(a: Long, v: Int) {
        val b = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(v).array()
        writeBytes(a, b)
    }

    fun writeFloat(a: Long, v: Float) = writeInt(a, java.lang.Float.floatToIntBits(v))

    fun readVec3(a: Long) = Vec3(readFloat(a), readFloat(a + 4), readFloat(a + 8))

    fun readString(a: Long): String {
        if (a == 0L) return ""
        val len = readInt(a + 0x10)
        if (len <= 0 || len > 48) return ""
        val b = readBytes(a + 0x14, len * 2)
        if (b.isEmpty()) return ""
        return try { String(b, Charsets.UTF_16LE) } catch (_: Exception) { "" }
    }

    private fun getPhotonNetworkStaticFields(): Long {
        if (libBase == 0L) return 0L
        if (cachedPhotonSF != 0L) return cachedPhotonSF
        cachedPhotonSF = scanClassByName("PhotonNetwork")
        return cachedPhotonSF
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
        val b = readBytes(addr, 48)
        if (b.isEmpty()) return ""
        val sb = StringBuilder()
        for (c in b) { if (c.toInt() == 0) break; sb.append(c.toChar()) }
        return sb.toString()
    }

    private var camControllerAddr: Long = 0L

    fun getCamPos(): Vec3 {
        val c = camControllerAddr
        return if (c == 0L) Vec3(0f, 0f, 0f) else readVec3(c + CAM_POS)
    }

    fun getCamYaw(): Float {
        val c = camControllerAddr
        return if (c == 0L) 0f else readFloat(c + CAM_YAW)
    }

    fun getCamPitch(): Float {
        val c = camControllerAddr
        return if (c == 0L) 0f else readFloat(c + CAM_PITCH)
    }

    fun writeCamAngles(yaw: Float, pitch: Float) {
        val c = camControllerAddr
        if (c == 0L) return
        writeFloat(c + CAM_YAW, yaw)
        writeFloat(c + CAM_PITCH, pitch)
    }

    fun getPlayers(): List<PlayerData> {
        val sf = getPhotonNetworkStaticFields()
        if (sf == 0L) { status = "no PhotonNetwork"; return emptyList() }
        val peerPtr = readLong(sf + PN_NETWORKING_PEER)
        if (peerPtr == 0L) { status = "no peer"; return emptyList() }
        val listPtr = readLong(peerPtr + NP_PLAYER_LIST_COPY)
        if (listPtr == 0L) { status = "no list"; return emptyList() }
        val count = readInt(listPtr + 0x18)
        val items = readLong(listPtr + 0x20)
        if (items == 0L || count <= 0) { status = "count=$count"; return emptyList() }
        val result = mutableListOf<PlayerData>()
        for (i in 0 until minOf(count, 16)) {
            val ppPtr = readLong(items + i * 8L)
            if (ppPtr == 0L) continue
            val isLocal = readInt(ppPtr + PP_IS_LOCAL) != 0
            val namePtr = readLong(ppPtr + PP_NAME)
            val pName = readString(namePtr)
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
            result.add(PlayerData(pcPtr, pos, buildBones(pos), hp, team, pName, isLocal))
        }
        status = "players=${result.size} listPtr=0x${listPtr.toString(16)}"
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
