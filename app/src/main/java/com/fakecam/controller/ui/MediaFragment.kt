package com.fakecam.controller.ui

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.fragment.app.Fragment
import com.bumptech.glide.Glide
import com.fakecam.controller.MainActivity
import com.fakecam.controller.R
import com.fakecam.controller.databinding.FragmentMediaBinding
import com.fakecam.controller.utils.FileUtils
import com.fakecam.controller.utils.MediaValidator
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Fragment for selecting and managing media files (video/image) for the virtual camera.
 */
class MediaFragment : Fragment() {

    private var _binding: FragmentMediaBinding? = null
    private val binding get() = _binding!!

    private var selectedMediaUri: Uri? = null
    private var selectedMediaType: MediaType? = null

    enum class MediaType {
        VIDEO,
        IMAGE
    }

    private val videoPickerLauncher = registerForActivityResult(
        ActivityResultContracts.GetContent()
    ) { uri ->
        uri?.let { handleMediaSelected(it, MediaType.VIDEO) }
    }

    private val imagePickerLauncher = registerForActivityResult(
        ActivityResultContracts.GetContent()
    ) { uri ->
        uri?.let { handleMediaSelected(it, MediaType.IMAGE) }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentMediaBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        setupClickListeners()
        loadCurrentMedia()
    }

    private fun setupClickListeners() {
        binding.selectVideoButton.setOnClickListener {
            if (checkPermissions()) {
                videoPickerLauncher.launch("video/mp4")
            }
        }

        binding.selectImageButton.setOnClickListener {
            if (checkPermissions()) {
                imagePickerLauncher.launch("image/bmp")
            }
        }

        binding.applyButton.setOnClickListener {
            applyMedia()
        }

        binding.clearButton.setOnClickListener {
            clearMedia()
        }
    }

    private fun checkPermissions(): Boolean {
        val activity = activity as? MainActivity ?: return false
        return if (activity.hasStoragePermissions()) {
            true
        } else {
            activity.requestStoragePermissions()
            false
        }
    }

    private fun handleMediaSelected(uri: Uri, type: MediaType) {
        val context = requireContext()
        
        // Validate the file
        val mimeType = context.contentResolver.getType(uri)
        val validationResult = when (type) {
            MediaType.VIDEO -> MediaValidator.validateVideo(context, uri, mimeType)
            MediaType.IMAGE -> MediaValidator.validateImage(context, uri, mimeType)
        }

        if (!validationResult.isValid) {
            showError(validationResult.errorMessage ?: getString(R.string.error_invalid_format))
            return
        }

        selectedMediaUri = uri
        selectedMediaType = type

        // Update UI
        updatePreview(uri, type)
        updateMediaInfo(uri)
        binding.applyButton.isEnabled = true
        binding.noMediaText.visibility = View.GONE
    }

    private fun updatePreview(uri: Uri, type: MediaType) {
        binding.previewImage.visibility = View.VISIBLE
        
        when (type) {
            MediaType.VIDEO -> {
                Glide.with(this)
                    .load(uri)
                    .placeholder(R.drawable.ic_video)
                    .error(R.drawable.ic_video)
                    .centerCrop()
                    .into(binding.previewImage)
            }
            MediaType.IMAGE -> {
                Glide.with(this)
                    .load(uri)
                    .placeholder(R.drawable.ic_image)
                    .error(R.drawable.ic_image)
                    .fitCenter()
                    .into(binding.previewImage)
            }
        }
    }

    private fun updateMediaInfo(uri: Uri) {
        val context = requireContext()
        var fileName = "Unknown"
        var fileSize = 0L

        context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                val sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE)
                if (nameIndex >= 0) {
                    fileName = cursor.getString(nameIndex)
                }
                if (sizeIndex >= 0) {
                    fileSize = cursor.getLong(sizeIndex)
                }
            }
        }

        val sizeText = formatFileSize(fileSize)
        binding.mediaInfoText.text = "$fileName - $sizeText"
        binding.mediaInfoText.visibility = View.VISIBLE
    }

    private fun formatFileSize(size: Long): String {
        return when {
            size >= 1024 * 1024 -> String.format("%.1f MB", size / (1024.0 * 1024.0))
            size >= 1024 -> String.format("%.1f KB", size / 1024.0)
            else -> "$size bytes"
        }
    }

    private fun applyMedia() {
        val uri = selectedMediaUri ?: return
        val type = selectedMediaType ?: return
        val context = requireContext()

        binding.applyButton.isEnabled = false

        CoroutineScope(Dispatchers.Main).launch {
            val result = withContext(Dispatchers.IO) {
                try {
                    val targetFileName = when (type) {
                        MediaType.VIDEO -> "virtual.mp4"
                        MediaType.IMAGE -> "virtual.bmp"
                    }
                    FileUtils.copyMediaToCamera(context, uri, targetFileName)
                } catch (e: Exception) {
                    FileUtils.CopyResult(false, e.message ?: getString(R.string.error_copy_failed))
                }
            }

            binding.applyButton.isEnabled = true

            if (result.success) {
                Toast.makeText(context, R.string.media_applied, Toast.LENGTH_SHORT).show()
            } else {
                showError(getString(R.string.media_apply_failed, result.errorMessage))
            }
        }
    }

    private fun clearMedia() {
        val context = requireContext()

        CoroutineScope(Dispatchers.Main).launch {
            withContext(Dispatchers.IO) {
                FileUtils.clearMedia(context)
            }

            selectedMediaUri = null
            selectedMediaType = null
            binding.previewImage.setImageResource(R.drawable.ic_image_placeholder)
            binding.noMediaText.visibility = View.VISIBLE
            binding.mediaInfoText.visibility = View.GONE
            binding.applyButton.isEnabled = false

            Toast.makeText(context, R.string.media_cleared, Toast.LENGTH_SHORT).show()
        }
    }

    private fun loadCurrentMedia() {
        val context = requireContext()

        CoroutineScope(Dispatchers.Main).launch {
            val currentMedia = withContext(Dispatchers.IO) {
                FileUtils.getCurrentMedia(context)
            }

            if (currentMedia != null) {
                binding.noMediaText.visibility = View.GONE
                binding.mediaInfoText.text = currentMedia.name
                binding.mediaInfoText.visibility = View.VISIBLE
                binding.applyButton.isEnabled = false // Already applied
                
                Glide.with(this@MediaFragment)
                    .load(currentMedia)
                    .placeholder(R.drawable.ic_image_placeholder)
                    .error(R.drawable.ic_image_placeholder)
                    .centerCrop()
                    .into(binding.previewImage)
            }
        }
    }

    private fun showError(message: String) {
        Toast.makeText(requireContext(), message, Toast.LENGTH_LONG).show()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
