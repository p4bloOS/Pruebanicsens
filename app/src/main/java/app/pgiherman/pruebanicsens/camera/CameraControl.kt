package app.pgiherman.pruebanicsens.camera

import android.content.Context
import android.os.Environment
import android.view.Surface
import android.widget.Toast
import java.io.File
import java.text.SimpleDateFormat
import java.time.LocalDateTime
import java.util.Date

object CameraControl {



    private external fun jniStartPreview(surface: Surface?) : Int
    private external fun jniStopPreview() : Int
    private external fun jniGetResolutions(): IntArray
    private external fun jniCaptureImage(fileName: String)

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

    fun getResolutions() : List<Pair<Int, Int>> {
        val resolutions : MutableList<Pair<Int, Int>> = mutableListOf()
        val result: IntArray
        try {
            result = jniGetResolutions()
        } catch (e: Exception) {
            throw Exception("Exception ocurred inside jniGetResolutions(): " + e.message)
        }
        for (i in result.indices step 2) {
            resolutions.add(Pair(result[i], result[i+1]))
        }
        return resolutions
    }

    fun captureImage(context: Context) : String {
        val date = Date(System.currentTimeMillis())
        val formatter = SimpleDateFormat("yyyy-MM-dd_HH-mm-ss")
        val timestamp = formatter.format(date)
        val fileName : String
        if (Environment.getExternalStorageState() == Environment.MEDIA_MOUNTED) {
            fileName = File(context.getExternalFilesDir(null), "${timestamp}.yuv").absolutePath
            jniCaptureImage(fileName)
        } else {
            throw Exception("No access to external storage")
        }
        return fileName
    }


}