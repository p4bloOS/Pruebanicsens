package app.pgiherman.pruebanicsens.camera

import android.view.Surface

object CameraControl {



    private external fun stringFromJNI(): String
    private external fun jniStartPreview(surface: Surface?) : Int
    private external fun jniStopPreview() : Int

    init {
        System.loadLibrary("pruebanicsens-lib")
    }

    fun startPreview(surface: Surface) {
        try {
            val result = jniStartPreview(surface)
            if (result != 0) {
                throw Exception("jniStartPreview() has returned a non-zero error code")
            }
        } catch (e: Exception) {
            throw Exception("Exception ocurred inside jniStartPreview(): " + e.message)
        }
    }

    fun stopPreview() {
        try {
            val result = jniStopPreview()
            if (result != 0) {
                throw Exception("jniStopPreview() has returned a non-zero error code")
            }
        } catch (e: Exception) {
            throw Exception("Exception ocurred inside jniStopPreview(): " + e.message)
        }
    }
}