package app.pgiherman.pruebanicsens.ui

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.viewModels
import androidx.core.view.isVisible
import androidx.lifecycle.Observer
import app.pgiherman.pruebanicsens.R
import app.pgiherman.pruebanicsens.camera.CameraControl
import app.pgiherman.pruebanicsens.databinding.ActivityMainBinding
import app.pgiherman.pruebanicsens.util.PermissionsHandler


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val viewModel: MainViewModel by viewModels {
        viewModelFactory
    }
    private val cameraControl   : CameraControl = CameraControl()
    private val viewModelFactory: MainViewModelFactory = MainViewModelFactory(cameraControl)
    private lateinit var permissionsHandler: PermissionsHandler




    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        permissionsHandler = PermissionsHandler(this, baseContext)

        permissionsHandler.requestPermissions()

        // Enable or disabled actions depending on the preview
        viewModel.showingPreview.observe(this) { newValue ->
            binding.btnCapture.isEnabled = newValue
            binding.tvOpenCamera.isVisible = !newValue
            binding.btnShowResolutions.isEnabled = newValue
            binding.swServer.isEnabled = newValue
        }

        binding.tvOpenCamera.setOnClickListener {
            if (!viewModel.showingPreview.value!!) {
                if (permissionsHandler.allPermissionsGranted()) {
                    viewModel.startPreview()
                } else {
                    Toast.makeText(this@MainActivity, R.string.permissions_denied, Toast.LENGTH_SHORT).show()
                }
            }
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