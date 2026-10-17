package com.hanto0.standoff

import android.util.Log

object ZeninLoader {
    private const val TAG = "hanto0_esp"
    private var loaded = false
    private var entryDone = false

    external fun Entry()

    fun load(): Boolean {
        if (loaded) return true
        return try {
            System.loadLibrary("hantoInternal")
            loaded = true
            Log.d(TAG, "libhantoInternal loaded OK")
            true
        } catch (e: Throwable) {
            Log.e(TAG, "load fail: $e")
            false
        }
    }

    fun startEntry() {
        if (!loaded || entryDone) return
        Thread {
            var attempts = 0
            while (attempts < 60) {
                try {
                    Log.d(TAG, "Entry() attempt $attempts")
                    Entry()
                    entryDone = true
                    Log.d(TAG, "Entry() success")
                    break
                } catch (e: Throwable) {
                    Log.e(TAG, "Entry() fail attempt $attempts: $e")
                }
                attempts++
                Thread.sleep(2000)
            }
        }.start()
    }
}
