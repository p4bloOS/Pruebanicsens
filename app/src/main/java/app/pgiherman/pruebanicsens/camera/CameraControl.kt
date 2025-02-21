package app.pgiherman.pruebanicsens.camera

import android.content.Context
import android.os.Environment
import android.util.Log
import android.view.Surface
import fi.iki.elonen.NanoHTTPD
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date

object CameraControl {

    private var externalFilesDir : File? = null
    fun setExternalFilesDir(context: Context) {
        externalFilesDir = context.getExternalFilesDir(null)
    }
    val TAG = "CameraControl"

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

    fun captureImage(isRemote: Boolean) : String {
        val date = Date(System.currentTimeMillis())
        val formatter = SimpleDateFormat("yyyy-MM-dd_HH-mm-ss")
        val timestamp = formatter.format(date)
        var pathName : String
        if (Environment.getExternalStorageState() == Environment.MEDIA_MOUNTED) {
            val fileName = if (isRemote) {
                "REMOTE_${timestamp}.yuv"
            } else {
                "${timestamp}.yuv"
            }
            pathName = File(externalFilesDir, fileName ).absolutePath
            jniCaptureImage(pathName)
        } else {
            throw Exception("No access to external storage")
        }
        return pathName
    }


    class CameraHttpServer(port: Int) : NanoHTTPD(port) {
        private fun handleGetRequest(session: IHTTPSession) : Response {
            val uri = session.uri
            return when (uri) {
                "/pruebanicsens" -> {
                    captureImage(true)
                    val resolutionsList = getResolutions()
                    var firstResolutionRead = false
                    var jsonResponse  = "["
                    for (res in resolutionsList) {
                        if (firstResolutionRead) {
                            jsonResponse += ",\"${res.first}x${res.second}\""
                        } else {
                            jsonResponse += "\"${res.first}x${res.second}\""
                            firstResolutionRead = true
                        }
                    }
                    jsonResponse+="]"
                    newFixedLengthResponse(Response.Status.OK, "application/json", jsonResponse)
                }
                else -> newFixedLengthResponse(Response.Status.NOT_FOUND, "text/plain", "Not Found")
            }
        }
        override fun serve(session: IHTTPSession): Response {
            Log.i(TAG, "Request received")
            return when (session.method) {
                Method.GET -> handleGetRequest(session)
                else -> newFixedLengthResponse(
                    Response.Status.METHOD_NOT_ALLOWED,
                    "text/plain",
                    "Method Not Allowed"
                )
            }
        }
    }
    private val port = 8080
    private val server: CameraHttpServer = CameraHttpServer(port)
    fun startServer() {
        server.start()
        Log.i(TAG, "Server listening at port ${port}...")
    }
    fun stopServer() {
        server.stop()
    }
}