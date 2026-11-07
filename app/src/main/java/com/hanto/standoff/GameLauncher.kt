package com.hanto.standoff
import android.content.Context
object GameLauncher {
    const val PKG = "com.axlebolt.standoff2"
    fun start(ctx: Context) {
        Shell.run("am force-stop $PKG")
        Shell.run("monkey -p $PKG -c android.intent.category.LAUNCHER 1")
    }
    fun pid(): Int = Shell.run("pidof $PKG").trim().toIntOrNull() ?: 0
}
object Shell {
    fun run(cmd: String): String = try {
        val p = Runtime.getRuntime().exec(arrayOf("su","-c",cmd))
        val out = p.inputStream.bufferedReader().readText()
        p.waitFor(); out
    } catch (e: Exception) { "" }
}