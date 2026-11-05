package com.hanto.internal

import android.content.Context
import java.io.DataOutputStream
import java.io.File
import java.io.FileOutputStream

object RootUtil {

    fun hasRoot(): Boolean {
        return try {
            val process = Runtime.getRuntime().exec("su -c id")
            val output = process.inputStream.bufferedReader().readText()
            process.waitFor()
            output.contains("uid=0")
        } catch (e: Exception) {
            false
        }
    }

    fun setupAndInject(context: Context): Boolean {
        return try {
            val assets = listOf("libastra.so", "vpnService", "inj.sh")
            val tmpDir = File(context.cacheDir, "hanto")
            tmpDir.mkdirs()

            for (f in assets) {
                val file = File(tmpDir, f)
                context.assets.open(f).use { input ->
                    FileOutputStream(file).use { output ->
                        input.copyTo(output)
                    }
                }
                file.setExecutable(true)
            }

            val process = Runtime.getRuntime().exec("su")
            val os = DataOutputStream(process.outputStream)

            os.writeBytes("cp ${tmpDir.absolutePath}/* /data/local/tmp/\n")
            os.writeBytes("chmod 777 /data/local/tmp/libastra.so\n")
            os.writeBytes("chmod 777 /data/local/tmp/vpnService\n")
            os.writeBytes("chmod 777 /data/local/tmp/inj.sh\n")
            os.writeBytes("sh /data/local/tmp/inj.sh &\n")
            os.writeBytes("exit\n")
            os.flush()
            true
        } catch (e: Exception) {
            false
        }
    }
}
