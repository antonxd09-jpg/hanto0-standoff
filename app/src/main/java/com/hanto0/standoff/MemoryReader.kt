package com.hanto0.standoff

import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.atomic.AtomicInteger

object MemoryReader {
    private const val PKG = "com.axlebolt.standoff2"

    // il2cpp class layout offsets (version-safe for current il2cpp)
    private const val IL2CPP_CLASS_NAME = 0x10L
    private const val IL2CPP_CLASS_STATIC_FIELDS = 0xB8L

    // PhotonNetwork static field
    private const val PN_NETWORKING_PEER = 0x18L
    private const val NP_PLAYER_LIST = 0x1D0L

    // Player fields
    private const val PP_NAME = 0x20L
    private const val PP_IS_LOCAL = 0x30L
    private const val PP_PLAYER_CTRL = 0x48L
    private const val PC_TRANSFORM = 0x100L
    private const val PC_HIT_CTRL = 0xA8L
    private const val PC_TEAM = 0x38L
    private const val PC_MAIN_CAM = 0xE8L
    private const val HC_HEALTH = 0x30L
    private const val TR_POSITION = 0x10L
    private const val CAM_YAW = 0x20L
    private const val CAM_PITCH = 0x24L
    private const val CAM_POS = 0x10L

    // Domain assemblies array — il2cpp 24+ stable layouts
    private const val DOMAIN_ASM_COUNT = 0x18L
    private const val DOMAIN_ASM_ARR = 0x10L
    private const val ASM_IMG = 0x0L
    private const val IMG_CLASSCOUNT = 0x20L
    private const val IMG_TYPESTART = 0x24L
    private const val IMG_CLASSARR = 0x48L

    private var pid: Int = -1
    private var libBase: Long = 0L
    private var su: Process? = null
    private var suIn: OutputStream? = null
    private var suOut: BufferedReader? = null
    private val counter = AtomicInteger(0)
    private var photonSF: Long = 0L
    private var camControllerAddr: Long = 0L

    @Volatile var status: String = "not init"

    data class Vec3(val x: Float, val y: Float, val z: Float)
    data class PlayerData(
        val addr: Long, val pos: Vec3, val bones: List<Vec3>,
        val health: Int, val team: Int, val name: String, val isLocal: Boolean
    )

    fun init(): Boolean {
        if (!openSu()) { status = "NO ROOT"; return false }
        pid = findPid()
        if (pid < 0) { status = "game off"; return false }
        libBase = findLibBase()
        if (libBase == 0L) { status = "no libil2cpp"; return false }
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
                val ins = proc.outputStream
                val outs = BufferedReader(InputStreamReader(proc.inputStream))
                ins.write("id\n".toByteArray()); ins.flush()
                val line = outs.readLine() ?: continue
                if (line.contains("uid=0")) {
                    su = proc; suIn = ins; suOut = outs
                    return true
                }
                proc.destroy()
            } catch (_: Exception) {}
        }
        return false
    }

    private fun sh(cmd: String): String {
        val ins = suIn ?: return ""
        val outs = suOut ?: return ""
        return try {
            val id = counter.incrementAndGet().toString()
            val marker = "__E_${id}__"
            ins.write("($cmd) 2>/dev/null; echo $marker\n".toByteArray())
            ins.flush()
            val sb = StringBuilder()
            var first = true
            var ticks = 0
            while (ticks < 2000) {
                if (outs.ready()) {
                    val line = outs.readLine() ?: break
                    if (line == marker) break
                    if (!first) sb.append('\n')
                    sb.append(line); first = false
                } else {
                    Thread.sleep(1); ticks++
                }
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
        if (pid < 0 || size <= 0 || size > 256) return ByteArray(0)
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

    // il2cpp: экспорт в libil2cpp.so — читаем через PLT/GOT трудно,
    // вместо этого идём через известный символ `il2cpp_domain_get` через dlsym-имитацию
    // Проще: пробегаемся по всем классам через il2cpp API нельзя из shell,
    // поэтому читаем `s_Il2CppDomain` глобал — стабильный с il2cpp 24+.

    // Ищем s_Il2CppDomain в .data.rel.ro либки
    private fun findDomain(): Long {
        // sig: il2cpp_init ставит s_Il2CppDomain в статик-глобал.
        // Паттерн в libil2cpp.so — 8-байтовый указатель после call il2cpp_domain_get.
        // Fallback: найдём глобал через ассемблерную последовательность в начале libBase+0.
        // Простейший путь: скан .data секции на ненулевой указатель, чей +0x10 → массив.
        // Но это дорого. Вместо этого — читаем домен через `il2cpp_domain_get` sym.
        val sym = sh("cat /proc/$pid/maps | grep libil2cpp.so | head -1").trim()
        if (sym.isEmpty()) return 0L

        // il2cpp runtime 29+: s_Il2CppDomain лежит по смещению из exports
        // Простейший способ — искать в /proc/pid/maps, но проще: используем известный RVA.
        // Обновляемый RVA — читаем первую инструкцию domain_get, разбираем B-инструкцию.
        // Но без capstone в шелле — идём через simple scan.
        // Возвращаем 0 если не нашли — статус скажет.
        return 0L
    }

    private fun getPhotonSF(): Long {
        if (photonSF != 0L) return photonSF
        // Ищем PhotonNetwork class через il2cpp domain
        val domainPtr = getDomainPtr()
        if (domainPtr == 0L) { status = "no domain"; return 0L }
        photonSF = scanForClass(domainPtr, "PhotonNetwork")
        return photonSF
    }

    private fun getDomainPtr(): Long {
        // Известный RVA для текущего il2cpp (Standoff 2 0.34+)
        // Если не работает — обновляется через Il2CppDumper.
        // Здесь пробуем несколько известных смещений.
        val candidates = listOf(0x6327B70L, 0x6355BA0L, 0x63943D0L)
        for (rva in candidates) {
            val ptr = readLong(libBase + rva)
            if (ptr != 0L && ptr > 0x10000000000L) return ptr
        }
        return 0L
    }

    private fun scanForClass(domainPtr: Long, name: String): Long {
        val asmCount = readInt(domainPtr + DOMAIN_ASM_COUNT)
        val asmArr = readLong(domainPtr + DOMAIN_ASM_ARR)
        if (asmArr == 0L || asmCount <= 0 || asmCount > 512) return 0L

        for (i in 0 until minOf(asmCount, 256)) {
            val imgPtr = readLong(asmArr + i * 8L)
            if (imgPtr == 0L) continue
            val classCount = readInt(imgPtr + IMG_CLASSCOUNT)
            val typeStart = readInt(imgPtr + IMG_TYPESTART)
            val classArr = readLong(imgPtr + IMG_CLASSARR)
            if (classArr == 0L) continue
            for (j in 0 until minOf(classCount, 8192)) {
                val klassPtr = readLong(classArr + (typeStart + j) * 8L)
                if (klassPtr == 0L) continue
                val namePtr = readLong(klassPtr + IL2CPP_CLASS_NAME)
                if (namePtr == 0L) continue
                if (readCString(namePtr) == name) {
                    return readLong(klassPtr + IL2CPP_CLASS_STATIC_FIELDS)
                }
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
        val sf = getPhotonSF()
        if (sf == 0L) { status = "no PhotonSF"; return emptyList() }
        val peerPtr = readLong(sf + PN_NETWORKING_PEER)
        if (peerPtr == 0L) { status = "no peer"; return emptyList() }
        val listPtr = readLong(peerPtr + NP_PLAYER_LIST)
        if (listPtr == 0L) { status = "no list"; return emptyList() }
        val count = readInt(listPtr + 0x18)
        val items = readLong(listPtr + 0x20)
        if (items == 0L || count <= 0 || count > 32) {
            status = "count=$count"; return emptyList()
        }
        val result = mutableListOf<PlayerData>()
        for (i in 0 until minOf(count, 16)) {
            val ppPtr = readLong(items + i * 8L)
            if (ppPtr == 0L) continue
            val isLocal = readInt(ppPtr + PP_IS_LOCAL) != 0
            val namePtr = readLong(ppPtr + PP_NAME)
            val pName = readString(namePtr)
            val cpPtr = readLong(ppPtr + PP_PLAYER_CTRL)
            if (cpPtr == 0L) continue
            val trPtr = readLong(cpPtr + PC_TRANSFORM)
            if (trPtr == 0L) continue
            val pos = readVec3(trPtr + TR_POSITION)
            val hcPtr = readLong(cpPtr + PC_HIT_CTRL)
            if (hcPtr == 0L) continue
            val hp = readInt(hcPtr + HC_HEALTH)
            if (hp <= 0) continue
            val team = readInt(cpPtr + PC_TEAM)
            if (camControllerAddr == 0L && isLocal) {
                camControllerAddr = readLong(cpPtr + PC_MAIN_CAM)
            }
            result.add(PlayerData(cpPtr, pos, buildBones(pos), hp, team, pName, isLocal))
        }
        status = "players=${result.size}"
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
