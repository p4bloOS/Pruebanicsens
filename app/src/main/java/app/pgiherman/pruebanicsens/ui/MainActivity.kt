package app.pgiherman.pruebanicsens.ui

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import androidx.activity.viewModels
import androidx.core.view.isVisible
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProvider
import app.pgiherman.pruebanicsens.camera.CameraControl
import app.pgiherman.pruebanicsens.databinding.ActivityMainBinding


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val viewModel: MainViewModel by viewModels {
        viewModelFactory
    }
    private val cameraControl   : CameraControl = CameraControl()
    private val viewModelFactory: MainViewModelFactory = MainViewModelFactory(cameraControl)




    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Enable or disabled actions depending on the preview
        viewModel.showingPreview.observe(this, Observer {
            newValue ->
                binding.btnCapture.isEnabled = newValue
                binding.tvOpenCamera.isVisible = !newValue
                binding.btnShowResolutions.isEnabled = newValue
                binding.swServer.isEnabled = newValue
        })

        binding.tvOpenCamera.setOnClickListener {
            viewModel.startPreview()
        }




        // Example of a call to a native method
        // binding.sampleText.text = stringFromJNI()
    }

    /**
     * A native method that is implemented by the 'pruebanicsens' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'pruebanicsens' library on application startup.
        init {
            System.loadLibrary("pruebanicsens")
        }
    }
}