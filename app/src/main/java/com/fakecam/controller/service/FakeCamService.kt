package com.fakecam.controller.service

import android.app.Service
import android.content.Intent
import android.os.IBinder
import com.fakecam.controller.IFakeCamService
import com.fakecam.controller.utils.FileUtils
import java.io.File

/**
 * Service for IPC communication with the FakeCam Magisk module.
 * Provides a Binder interface for other apps or the module to communicate with this controller.
 * 
 * The Magisk module can bind to this service to:
 * - Get the current media path
 * - Check if the virtual camera is enabled
 * - Set resolution settings
 * - Refresh content
 */
class FakeCamService : Service() {

    private val binder = FakeCamServiceImpl()

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    /**
     * Implementation of the AIDL interface
     */
    private inner class FakeCamServiceImpl : IFakeCamService.Stub() {

        override fun isModuleActive(): Boolean {
            // Check if the Camera1 directory exists and has content
            val dir = FileUtils.getCameraDirectory()
            return dir.exists() && (dir.listFiles()?.isNotEmpty() == true)
        }

        override fun setMediaPath(path: String?): Boolean {
            if (path.isNullOrEmpty()) return false
            
            return try {
                val sourceFile = File(path)
                if (!sourceFile.exists()) return false
                
                val targetFileName = when {
                    path.endsWith(".mp4", ignoreCase = true) -> "virtual.mp4"
                    path.endsWith(".bmp", ignoreCase = true) -> "virtual.bmp"
                    else -> return false
                }
                
                FileUtils.ensureCameraDirectoryExists()
                val targetFile = File(FileUtils.getCameraDirectory(), targetFileName)
                sourceFile.copyTo(targetFile, overwrite = true)
                true
            } catch (e: Exception) {
                false
            }
        }

        override fun getCurrentMediaPath(): String {
            val media = FileUtils.getCurrentMedia(this@FakeCamService)
            return media?.absolutePath ?: ""
        }

        override fun setEnabled(enabled: Boolean): Boolean {
            return FileUtils.setEnabled(this@FakeCamService, enabled)
        }

        override fun isEnabled(): Boolean {
            return FileUtils.isEnabled(this@FakeCamService)
        }

        override fun setResolution(width: Int, height: Int): Boolean {
            if (width < 1 || height < 1 || width > 4096 || height > 4096) {
                return false
            }
            
            val settings = mapOf(
                "width" to width.toString(),
                "height" to height.toString()
            )
            return FileUtils.saveSettings(this@FakeCamService, settings)
        }

        override fun refresh(): Boolean {
            // Trigger a refresh by updating the settings file timestamp
            return try {
                val settings = FileUtils.loadSettings(this@FakeCamService).toMutableMap()
                settings["last_refresh"] = System.currentTimeMillis().toString()
                FileUtils.saveSettings(this@FakeCamService, settings)
                true
            } catch (e: Exception) {
                false
            }
        }
    }

    override fun onCreate() {
        super.onCreate()
        // Ensure the Camera1 directory exists when service starts
        FileUtils.ensureCameraDirectoryExists()
    }
}
