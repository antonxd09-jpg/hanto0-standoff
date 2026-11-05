package com.hanto.internal

import android.content.Context
import java.io.DataOutputStream
import java.io.File
import java.io.FileOutputStream

object RootUtil {
    fun hasRoot(): Boolean {
        return try {
            val p = Runtime.getRuntime().exec("su -c id")
            val out = p.inputStream.bufferedReader().readText()
            p.waitFor()
            out.contains("uid=0")
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
            }
            
            val process = Runtime.getRuntime().exec("su")
            val os = DataOutputStream(process.outputStream)
            
            // Копируем и даем права
            os.writeBytes("cp ${tmpDir.absolutePath}/* /data/local/tmp/\n")
            os.writeBytes("chmod 777 /data/local/tmp/libastra.so\n")
            os.writeBytes("chmod 777 /data/local/tmp/vpnService\n")
            os.writeBytes("chmod 777 /data/local/tmp/inj.sh\n")
            
            // nohup отвязывает процесс от приложения, чтобы он не умер при finish()
            os.writeBytes("nohup sh /data/local/tmp/inj.sh > /dev/null 2>&1 &\n")
            
            os.writeBytes("exit\n")
            os.flush()
            os.close()
            process.waitFor()
            true
        } catch (e: Exception) {
            e.printStackTrace()
            false
        }
    }
}
