package app.pgiherman.pruebanicsens.ui

import android.content.DialogInterface
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Menu
import android.view.MenuItem
import android.view.SurfaceHolder
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AlertDialog
import androidx.core.view.isVisible
import app.pgiherman.pruebanicsens.R
import app.pgiherman.pruebanicsens.camera.CameraControl
import app.pgiherman.pruebanicsens.databinding.ActivityMainBinding
import app.pgiherman.pruebanicsens.util.PermissionsHandler
import com.google.android.material.snackbar.Snackbar


class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val viewModel: MainViewModel by viewModels {
        viewModelFactory
    }
    private val cameraControl   : CameraControl = CameraControl
    private val viewModelFactory: MainViewModelFactory = MainViewModelFactory(cameraControl)
    private lateinit var permissionsHandler: PermissionsHandler

    // Display a toast with an exception message
    private fun toastException(e:Exception) {
        Toast.makeText(this@MainActivity, e.message, Toast.LENGTH_LONG).show()
    }

    private fun showResolutionsDialog(resolutionsList: List<Pair<Int, Int>> ) {
        var resolutionsText  = ""
        for (res in resolutionsList) {
            resolutionsText += "${res.first}x${res.second}\n"
        }
        val dialogBuilder = AlertDialog.Builder(this)
        dialogBuilder.setTitle(getString(R.string.avialiable_resolutions))
        dialogBuilder.setMessage(resolutionsText)
        dialogBuilder.setPositiveButton(getString(R.string.close)) {
                dialog: DialogInterface, _: Int ->
            dialog.dismiss()
        }
        val dialog = dialogBuilder.create()
        dialog.show()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.main, menu)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.action_info -> {
                val dialogBuilder = AlertDialog.Builder(this)
                dialogBuilder.setTitle(getString(R.string.about_this_app))
                dialogBuilder.setMessage(getString(R.string.app_description))
                dialogBuilder.setPositiveButton(getString(R.string.accept)) {
                        dialog: DialogInterface, _: Int ->
                    dialog.dismiss()
                }
                val dialog = dialogBuilder.create()
                dialog.show()
                true
            }
            else -> super.onOptionsItemSelected(item)
        }
    }


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
            if (newValue == false && viewModel.runningServer.value == true) {
                binding.swServer.isChecked = false
                viewModel.stopServer()
            }
        }

        // Server text when the server is running
        viewModel.runningServer.observe(this) {newValue ->
            if (newValue) {
                binding.tvServerMsg.text = getString(R.string.server_msg)
            } else {
                binding.tvServerMsg.text = ""
            }
        }



        // Camera preview
        binding.tvOpenCamera.setOnClickListener {
            if (!viewModel.showingPreview.value!!) {
                if (permissionsHandler.allPermissionsGranted()) {
                    val surfaceHolder = binding.svCameraPreview.holder
                    try {
                        viewModel.startPreview(surfaceHolder.surface)
                    } catch (e: Exception) {
                        toastException(e)
                    }
                    surfaceHolder.addCallback(object : SurfaceHolder.Callback {
                        override fun surfaceCreated(holder: SurfaceHolder) {
                            try {
                                viewModel.startPreview(surfaceHolder.surface)
                            } catch (e: Exception) {
                                toastException(e)
                            }
                        }
                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            try {
                                viewModel.stopPreview()
                            } catch (e: Exception) {
                                toastException(e)
                            }
                        }

                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int
                        ) {
                            try {
                                viewModel.stopPreview()
                            } catch (e: Exception) {
                                toastException(e)
                            }
                        }

                    })


                } else {
                    Toast.makeText(this@MainActivity, R.string.permissions_denied, Toast.LENGTH_SHORT).show()
                }
            }
        }

        binding.btnShowResolutions.setOnClickListener{
            showResolutionsDialog(viewModel.getResolutions())
        }

        binding.swServer.setOnCheckedChangeListener{ _, isChecked: Boolean ->
            if (isChecked) {
                viewModel.startServer()
            } else {
                viewModel.stopServer()
            }
        }

        binding.btnCapture.setOnClickListener{
            val imageFilePath : String
            try {
                imageFilePath = viewModel.captureImage(this@MainActivity)
                // Toast.makeText(this@MainActivity,"${getString(R.string.will_be_saved_in)} $imageFilePath", Toast.LENGTH_LONG).show()
                Snackbar.make(binding.root, "${getString(R.string.will_be_saved_in)} $imageFilePath", Snackbar.LENGTH_LONG).show()
            } catch (e: Exception) {
                toastException(e)
            }
        }

    }

}