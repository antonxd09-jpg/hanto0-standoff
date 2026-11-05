package com.hanto0.loader

object NativeBridge {
    init {
        System.loadLibrary("hanto0")
    }

    external fun setFeatureToggle(key: String, enabled: Boolean): Boolean
    external fun getFeatureToggle(key: String): Boolean
    external fun setSliderValue(key: String, value: Float)
    external fun getArchiveMetadata(): String
}
