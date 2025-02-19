package app.pgiherman.pruebanicsens.ui

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import app.pgiherman.pruebanicsens.camera.CameraControl


class MainViewModelFactory constructor(private val cameraControl: CameraControl) : ViewModelProvider.Factory
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

    fun startPreview() {
        // ABRIR CÁMARA Y MOSTRAR PREVIEW
        _showingPreview.value = true
    }

}