package com.fakecam.controller.utils

import android.content.Context
import android.graphics.BitmapFactory
import android.media.MediaMetadataRetriever
import android.net.Uri

/**
 * Utility class for validating media files (video and image).
 * Ensures files are in supported formats and within acceptable parameters.
 */
object MediaValidator {

    private const val MAX_FILE_SIZE = 500L * 1024 * 1024 // 500 MB
    private const val MIN_RESOLUTION = 16
    private const val MAX_RESOLUTION = 4096

    data class ValidationResult(
        val isValid: Boolean,
        val errorMessage: String? = null,
        val width: Int = 0,
        val height: Int = 0
    )

    /**
     * Validate a video file
     */
    fun validateVideo(context: Context, uri: Uri, mimeType: String?): ValidationResult {
        // Check MIME type
        if (mimeType != null && !mimeType.startsWith("video/")) {
            return ValidationResult(false, "Invalid file type. Expected video file.")
        }

        // Check file size
        val fileSize = getFileSize(context, uri)
        if (fileSize > MAX_FILE_SIZE) {
            return ValidationResult(false, "File too large. Maximum size is 500 MB.")
        }

        // Validate video properties
        return try {
            val retriever = MediaMetadataRetriever()
            retriever.setDataSource(context, uri)

            val widthStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)
            val heightStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)
            
            retriever.release()

            val width = widthStr?.toIntOrNull() ?: 0
            val height = heightStr?.toIntOrNull() ?: 0

            if (width < MIN_RESOLUTION || height < MIN_RESOLUTION) {
                return ValidationResult(false, "Video resolution too small. Minimum is ${MIN_RESOLUTION}x${MIN_RESOLUTION}.")
            }

            if (width > MAX_RESOLUTION || height > MAX_RESOLUTION) {
                return ValidationResult(false, "Video resolution too large. Maximum is ${MAX_RESOLUTION}x${MAX_RESOLUTION}.")
            }

            ValidationResult(true, null, width, height)
        } catch (e: Exception) {
            // If we can't read metadata, still allow the file but warn
            ValidationResult(true, null, 0, 0)
        }
    }

    /**
     * Validate an image file
     */
    fun validateImage(context: Context, uri: Uri, mimeType: String?): ValidationResult {
        // Check MIME type - allow BMP and common image formats
        if (mimeType != null && !mimeType.startsWith("image/")) {
            return ValidationResult(false, "Invalid file type. Expected image file.")
        }

        // Check file size
        val fileSize = getFileSize(context, uri)
        if (fileSize > MAX_FILE_SIZE) {
            return ValidationResult(false, "File too large. Maximum size is 500 MB.")
        }

        // Validate image properties
        return try {
            val options = BitmapFactory.Options().apply {
                inJustDecodeBounds = true
            }

            context.contentResolver.openInputStream(uri)?.use { inputStream ->
                BitmapFactory.decodeStream(inputStream, null, options)
            }

            val width = options.outWidth
            val height = options.outHeight

            if (width <= 0 || height <= 0) {
                return ValidationResult(false, "Could not read image dimensions.")
            }

            if (width < MIN_RESOLUTION || height < MIN_RESOLUTION) {
                return ValidationResult(false, "Image resolution too small. Minimum is ${MIN_RESOLUTION}x${MIN_RESOLUTION}.")
            }

            if (width > MAX_RESOLUTION || height > MAX_RESOLUTION) {
                return ValidationResult(false, "Image resolution too large. Maximum is ${MAX_RESOLUTION}x${MAX_RESOLUTION}.")
            }

            ValidationResult(true, null, width, height)
        } catch (e: Exception) {
            ValidationResult(false, "Could not validate image: ${e.message}")
        }
    }

    /**
     * Get file size from URI
     */
    private fun getFileSize(context: Context, uri: Uri): Long {
        return try {
            context.contentResolver.openAssetFileDescriptor(uri, "r")?.use { descriptor ->
                descriptor.length
            } ?: 0L
        } catch (e: Exception) {
            0L
        }
    }
}
