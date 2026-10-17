package com.hanto0.standoff

import android.util.Log
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

object MemoryReader {
    private const val TAG = "hanto0_esp"
    private const val PKG = "com.axlebolt.standoff2"

    // RVA из il2cpp api 1.0.0.txt
    private const val RVA_DOMAIN_GET_BRANCH = 0x634279CL  // branch target of il2cpp_domain_get

    // PhotonNetwork instance field offsets (из dump.cs)
    private const val OFF_PN_PEER      = 0x18L   // NetworkingPeer networkingPeer
    private const val OFF_NP_PLIST     = 0x1D0L  // mPlayerListCopy
    private const val OFF_PP_TAG       = 0x40L   // TagObject
    private const val OFF_PP_NAME      = 0x20L   // name
    private const val OFF_PP_ISLOCAL   = 0x30L   // isLocal
    private const val OFF_CP_PC        = 0x50L   // PlayerController
    private const val OFF_PC_TRANSFORM = 0x100L
    private const val OFF_PC_HIT_CTRL  = 0xA8L
    private const val OFF_PC_CAM       = 0xE8L
    private const val OFF_HC_HEALTH    = 0x30L
    private const val OFF_TR_POSITION  = 0x10L
    private const val OFF_CAM_POS      = 0x10L
    private const val OFF_CAM_YAW      = 0x20L
    private const val OFF_CAM_PITCH    = 0x24L

    // Il2CppClass offsets (из il2cpp api)
    private const val IL2CPP_CLASS_NAME         = 0x198L
    private const val IL2CPP_CLASS_STATIC_FIELDS= 0x40L  // wait - actually need to verify
    // Actually from il2cpp api: static_fields is at different offset
    // Using 0xB8 for il2cpp 27.x which Unity 2021 uses
    private const val IL2CPP_CLASS_STATIC_FIELDS_ACTUAL = 0xB8L

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
            if (libBase == 0L) return false
            pnSF = findPhotonNetworkStatic()
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

    private fun getLibBase(): Long {
        try {
            val lines = File("/proc/$pid/maps").readLines()
            for (line in lines) {
                if (line.contains("libil2cpp.so") && line.contains("r-xp")) {
                    return line.split("-")[0].toLong(16)
                }
            }
            // fallback: first executable mapping of libil2cpp
            for (line in lines) {
                if (line.contains("libil2cpp.so")) {
                    return line.split("-")[0].toLong(16)
                }
            }
        } catch (e: Exception) { Log.e(TAG, "getLibBase fail: $e") }
        return 0L
    }

    // Decode ARM64 ADRP instruction: returns page address
    private fun decodeAdrp(insn: Int, pc: Long): Long {
        val immlo = (insn shr 29) and 0x3
        val immhi = (insn shr 5) and 0x7FFFF
        val imm = ((immhi.toLong() shl 2) or immlo.toLong()) shl 12
        val pcPage = pc and -0x1000L
        return pcPage + imm
    }

    // Decode ARM64 LDR (unsigned offset) 64-bit: returns byte offset
    private fun decodeLdrUoff64(insn: Int): Long {
        val imm12 = (insn shr 10) and 0xFFF
        return imm12.toLong() * 8L  // scale=8 for 64-bit LDR
    }

    // Find the global Il2CppDomain* by decoding ADRP+LDR at il2cpp_domain_get branch target
    private fun findDomainPtr(): Long {
        return try {
            val pc = libBase + RVA_DOMAIN_GET_BRANCH
            // scan up to 8 instructions for ADRP+LDR pair
            for (i in 0 until 8) {
                val addr = pc + i * 4L
                val insn0 = readInt(addr)
                // check ADRP: bits[28:24] == 10000, bit[31] == 1
                if ((insn0 and 0x9F000000.toInt()) == 0x90000000.toInt()) {
                    val insn1 = readInt(addr + 4)
                    // check LDR 64-bit unsigned offset: top 10 bits = 1111100001
                    if ((insn1 and 0xFFC00000.toInt()) == 0xF9400000.toInt()) {
                        val page = decodeAdrp(insn0, addr)
                        val off  = decodeLdrUoff64(insn1)
                        val domPtrAddr = page + off
                        Log.d(TAG, "domPtrAddr=0x${domPtrAddr.toString(16)}")
                        val dom = readLong(domPtrAddr)
                        Log.d(TAG, "domain=0x${dom.toString(16)}")
                        if (dom != 0L) return dom
                    }
                }
            }
            Log.w(TAG, "ADRP+LDR not found at domain_get")
            0L
        } catch (e: Exception) { Log.e(TAG, "findDomainPtr fail: $e"); 0L }
    }

    private fun findPhotonNetworkStatic(): Long {
        val domain = findDomainPtr()
        if (domain == 0L) {
            Log.e(TAG, "domain is 0")
            return 0L
        }
        return try {
            // Il2CppDomain->domain_assemblies: GSList* at offset 0x10 (pointer) + count at 0x18
            // Try walking assembly list
            val asmCount = readInt(domain + 0x18)
            val asmArr   = readLong(domain + 0x10)
            Log.d(TAG, "asmCount=$asmCount asmArr=0x${asmArr.toString(16)}")
            if (asmArr == 0L || asmCount <= 0 || asmCount > 1024) return fallbackWalkGSList(domain)

            for (i in 0 until minOf(asmCount, 512)) {
                val asmPtr = readLong(asmArr + i * 8L)
                if (asmPtr == 0L) continue
                // Il2CppAssembly->image at offset 0x0 or 0x8
                val img = readLong(asmPtr)
                if (img == 0L) continue
                val namePtr = readLong(img)
                if (namePtr == 0L) continue
                val imgName = readCStr(namePtr)
                if (!imgName.contains("CSharp") && !imgName.contains("Assembly")) continue

                val typeCount = readInt(img + 0x18)
                val typeStart = readInt(img + 0x1C)
                Log.d(TAG, "img=$imgName typeCount=$typeCount typeStart=$typeStart")
                if (typeCount <= 0 || typeCount > 50000) continue

                // Il2CppImage->metadataHandle / typeStart points into g_Il2CppTypeDefinitions
                // Try: klass table at img+0x40 with (typeStart+j)*8 index
                for (j in 0 until minOf(typeCount, 32768)) {
                    val klass = readLong(img + 0x40L + (typeStart + j) * 8L)
                    if (klass == 0L) continue
                    val np = readLong(klass + IL2CPP_CLASS_NAME)
                    if (np == 0L) continue
                    val name = readCStr(np)
                    if (name == "PhotonNetwork") {
                        Log.d(TAG, "found PhotonNetwork klass=0x${klass.toString(16)}")
                        // Try both possible static_fields offsets
                        var sf = readLong(klass + IL2CPP_CLASS_STATIC_FIELDS_ACTUAL)
                        if (sf == 0L) sf = readLong(klass + IL2CPP_CLASS_STATIC_FIELDS)
                        Log.d(TAG, "static_fields=0x${sf.toString(16)}")
                        return sf
                    }
                }
            }
            Log.w(TAG, "PhotonNetwork not found in assembly walk")
            0L
        } catch (e: Exception) { Log.e(TAG, "findPN fail: $e"); 0L }
    }

    // Fallback: walk GSList linked list for domain assemblies
    private fun fallbackWalkGSList(domain: Long): Long {
        return try {
            var node = readLong(domain + 0x10)
            var limit = 0
            while (node != 0L && limit < 512) {
                val asmPtr = readLong(node)
                val next   = readLong(node + 0x8)
                if (asmPtr != 0L) {
                    val img = readLong(asmPtr)
                    if (img != 0L) {
                        val namePtr = readLong(img)
                        if (namePtr != 0L) {
                            val n = readCStr(namePtr)
                            if (n.contains("CSharp") || n.contains("Assembly")) {
                                val tc = readInt(img + 0x18)
                                val ts = readInt(img + 0x1C)
                                for (j in 0 until minOf(tc, 32768)) {
                                    val klass = readLong(img + 0x40L + (ts + j) * 8L)
                                    if (klass == 0L) continue
                                    val np = readLong(klass + IL2CPP_CLASS_NAME); if (np == 0L) continue
                                    if (readCStr(np) == "PhotonNetwork") {
                                        var sf = readLong(klass + IL2CPP_CLASS_STATIC_FIELDS_ACTUAL)
                                        if (sf == 0L) sf = readLong(klass + IL2CPP_CLASS_STATIC_FIELDS)
                                        return sf
                                    }
                                }
                            }
                        }
                    }
                }
                node = next
                limit++
            }
            0L
        } catch (e: Exception) { 0L }
    }

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
        return try {
            val peerPtr = readLong(pnSF + OFF_PN_PEER)
            if (peerPtr == 0L) { Log.w(TAG, "peerPtr=0"); return emptyList() }
            val listArr = readLong(peerPtr + OFF_NP_PLIST)
            if (listArr == 0L) { Log.w(TAG, "listArr=0"); return emptyList() }
            val count = readInt(listArr + 0x18)
            val items = readLong(listArr + 0x20)
            Log.d(TAG, "playerCount=$count items=0x${items.toString(16)}")
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
                    if (cp != 0L) { camAddr = cp; Log.d(TAG, "camAddr=0x${camAddr.toString(16)}") }
                }
                result.add(PlayerData(pcPtr, pos, buildBones(pos), hp, team, name, isLocal))
            }
            result
        } catch (e: Exception) { Log.e(TAG, "getPlayers fail: $e"); emptyList() }
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
