package com.fakecam.controller.utils

import android.content.Context
import android.net.Uri
import android.os.Environment
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream

/**
 * Utility class for file operations related to FakeCam.
 * Handles copying media files to the Camera1 directory and managing control flags.
 */
object FileUtils {

    private const val CAMERA_DIR = "DCIM/Camera1"
    private const val VIRTUAL_VIDEO = "virtual.mp4"
    private const val VIRTUAL_IMAGE = "virtual.bmp"
    private const val DISABLE_FLAG = "disable.jpg"
    private const val NO_TOAST_FLAG = "no_toast.jpg"
    private const val SETTINGS_FILE = "settings.conf"

    data class CopyResult(val success: Boolean, val errorMessage: String = "")

    /**
     * Get the Camera1 directory path where FakeCam reads files
     */
    fun getCameraDirectory(): File {
        val dcimDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
        return File(dcimDir, "Camera1")
    }

    /**
     * Ensure the Camera1 directory exists
     */
    fun ensureCameraDirectoryExists(): Boolean {
        val dir = getCameraDirectory()
        return if (!dir.exists()) {
            dir.mkdirs()
        } else {
            true
        }
    }

    /**
     * Copy media file to Camera1 directory for FakeCam module
     */
    fun copyMediaToCamera(context: Context, sourceUri: Uri, targetFileName: String): CopyResult {
        return try {
            if (!ensureCameraDirectoryExists()) {
                return CopyResult(false, "Could not create output directory")
            }

            val targetFile = File(getCameraDirectory(), targetFileName)
            
            // Clear existing media files first
            clearMediaFiles()

            context.contentResolver.openInputStream(sourceUri)?.use { inputStream ->
                FileOutputStream(targetFile).use { outputStream ->
                    inputStream.copyTo(outputStream)
                }
            } ?: return CopyResult(false, "Could not open source file")

            CopyResult(true)
        } catch (e: SecurityException) {
            CopyResult(false, "Permission denied: ${e.message}")
        } catch (e: Exception) {
            CopyResult(false, "Copy failed: ${e.message}")
        }
    }

    /**
     * Clear all media files from Camera1 directory
     */
    private fun clearMediaFiles() {
        val dir = getCameraDirectory()
        if (dir.exists()) {
            File(dir, VIRTUAL_VIDEO).delete()
            File(dir, VIRTUAL_IMAGE).delete()
        }
    }

    /**
     * Clear media and return to normal camera
     */
    fun clearMedia(context: Context): Boolean {
        return try {
            clearMediaFiles()
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Get the current media file if exists
     */
    fun getCurrentMedia(context: Context): File? {
        val dir = getCameraDirectory()
        if (!dir.exists()) return null

        val videoFile = File(dir, VIRTUAL_VIDEO)
        if (videoFile.exists()) return videoFile

        val imageFile = File(dir, VIRTUAL_IMAGE)
        if (imageFile.exists()) return imageFile

        return null
    }

    /**
     * Set enable/disable state by creating/removing the disable flag file
     */
    fun setEnabled(context: Context, enabled: Boolean): Boolean {
        return try {
            if (!ensureCameraDirectoryExists()) return false

            val disableFile = File(getCameraDirectory(), DISABLE_FLAG)
            if (enabled) {
                // Remove disable flag to enable
                if (disableFile.exists()) {
                    disableFile.delete()
                }
            } else {
                // Create disable flag to disable
                if (!disableFile.exists()) {
                    disableFile.createNewFile()
                }
            }
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Check if FakeCam is enabled (no disable flag exists)
     */
    fun isEnabled(context: Context): Boolean {
        val disableFile = File(getCameraDirectory(), DISABLE_FLAG)
        return !disableFile.exists()
    }

    /**
     * Set no toast flag
     */
    fun setNoToast(context: Context, noToast: Boolean): Boolean {
        return try {
            if (!ensureCameraDirectoryExists()) return false

            val noToastFile = File(getCameraDirectory(), NO_TOAST_FLAG)
            if (noToast) {
                if (!noToastFile.exists()) {
                    noToastFile.createNewFile()
                }
            } else {
                if (noToastFile.exists()) {
                    noToastFile.delete()
                }
            }
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Save settings to a configuration file
     */
    fun saveSettings(context: Context, settings: Map<String, String>): Boolean {
        return try {
            if (!ensureCameraDirectoryExists()) return false

            val settingsFile = File(getCameraDirectory(), SETTINGS_FILE)
            settingsFile.writeText(
                settings.entries.joinToString("\n") { "${it.key}=${it.value}" }
            )
            true
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Load settings from configuration file
     */
    fun loadSettings(context: Context): Map<String, String> {
        return try {
            val settingsFile = File(getCameraDirectory(), SETTINGS_FILE)
            if (!settingsFile.exists()) return emptyMap()

            settingsFile.readLines()
                .filter { it.contains("=") }
                .associate { line ->
                    val parts = line.split("=", limit = 2)
                    parts[0].trim() to (parts.getOrNull(1)?.trim() ?: "")
                }
        } catch (e: Exception) {
            emptyMap()
        }
    }
}
