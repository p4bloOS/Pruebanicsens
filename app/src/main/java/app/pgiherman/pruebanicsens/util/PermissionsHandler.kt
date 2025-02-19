package app.pgiherman.pruebanicsens.util

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.util.Log
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat

class PermissionsHandler (activity: AppCompatActivity, private val context: Context) {

    private val TAG = "PermissionsHandler"

    private val REQUIRED_PERMISSIONS =
        mutableListOf (
            Manifest.permission.CAMERA,
        ).apply {
            if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P) {
                add(Manifest.permission.WRITE_EXTERNAL_STORAGE)
            }
        }.toTypedArray()

    private val activityResultLauncher =
        activity.registerForActivityResult(
            ActivityResultContracts.RequestMultiplePermissions())
        { permissions ->
            // Handle Permission granted/rejected
            permissions.entries.forEach {
                if (it.key in REQUIRED_PERMISSIONS && it.value == true) {
                    Log.i(TAG, "Permission granted")
                } else {
                    Log.w(TAG, "Permission denied")
                }
            }
        }

    fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(context, it) == PackageManager.PERMISSION_GRANTED
    }

    fun requestPermissions() {
        activityResultLauncher.launch(REQUIRED_PERMISSIONS)
    }

}









