package app.pgiherman.pruebanicsens.ui

import android.content.Context
import android.view.Surface
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import app.pgiherman.pruebanicsens.camera.CameraControl


class MainViewModelFactory(private val cameraControl: CameraControl) : ViewModelProvider.Factory
{
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MainViewModel::class.java)) {
            return MainViewModel(cameraControl) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

class MainViewModel(private val cameraControl: CameraControl) : ViewModel() {

    private val _showingPreview = MutableLiveData<Boolean>(false)
    val showingPreview : LiveData<Boolean> get() = _showingPreview

    private val _runningServer = MutableLiveData<Boolean>(false)
    val runningServer : LiveData<Boolean> get() = _runningServer

    fun startServer() {
        // ARRANCAR SERVIDOR
        _runningServer.value = true
    }

    fun stopServer() {
        // DETENER SERVIDOR
        _runningServer.value = false
    }

    fun startPreview(surface: Surface) {
        try {
            CameraControl.startPreview(surface)
            _showingPreview.value = true
        } catch (e: Exception) {
            _showingPreview.value = false
            throw e
        }
    }

    fun stopPreview() {
        if (_showingPreview.value == true) {
            CameraControl.stopPreview()
        }
        _showingPreview.value = false
    }

    fun getResolutions() =
        CameraControl.getResolutions()

    fun captureImage(context: Context) : String =
        CameraControl.captureImage(context)

}