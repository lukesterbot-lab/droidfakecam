package com.fakecam.controller.ui

import android.content.Context
import android.content.SharedPreferences
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.core.widget.doAfterTextChanged
import androidx.fragment.app.Fragment
import com.fakecam.controller.R
import com.fakecam.controller.databinding.FragmentSettingsBinding
import com.fakecam.controller.utils.FileUtils
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Fragment for configuring FakeCam settings like resolution, camera options, and flags.
 */
class SettingsFragment : Fragment() {

    private var _binding: FragmentSettingsBinding? = null
    private val binding get() = _binding!!

    private lateinit var prefs: SharedPreferences

    companion object {
        private const val PREFS_NAME = "fakecam_settings"
        private const val KEY_ENABLED = "enabled"
        private const val KEY_WIDTH = "width"
        private const val KEY_HEIGHT = "height"
        private const val KEY_FLIP = "flip_front_camera"
        private const val KEY_AUDIO_SYNC = "audio_sync"
        private const val KEY_NO_TOAST = "no_toast"
        private const val KEY_PRIVATE_DIRS = "private_dirs"

        private const val DEFAULT_WIDTH = 1920
        private const val DEFAULT_HEIGHT = 1080
        private const val MIN_RESOLUTION = 1
        private const val MAX_RESOLUTION = 4096
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentSettingsBinding.inflate(inflater, container, false)
        return binding.root
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        prefs = requireContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        loadSettings()
        setupListeners()
    }

    private fun loadSettings() {
        binding.enableSwitch.isChecked = prefs.getBoolean(KEY_ENABLED, true)
        binding.widthInput.setText(prefs.getInt(KEY_WIDTH, DEFAULT_WIDTH).toString())
        binding.heightInput.setText(prefs.getInt(KEY_HEIGHT, DEFAULT_HEIGHT).toString())
        binding.flipSwitch.isChecked = prefs.getBoolean(KEY_FLIP, false)
        binding.audioSyncSwitch.isChecked = prefs.getBoolean(KEY_AUDIO_SYNC, false)
        binding.noToastSwitch.isChecked = prefs.getBoolean(KEY_NO_TOAST, false)
        binding.privateDirsSwitch.isChecked = prefs.getBoolean(KEY_PRIVATE_DIRS, false)
    }

    private fun setupListeners() {
        binding.enableSwitch.setOnCheckedChangeListener { _, isChecked ->
            saveAndApplySettings()
            updateEnableState(isChecked)
        }

        binding.flipSwitch.setOnCheckedChangeListener { _, _ -> saveAndApplySettings() }
        binding.audioSyncSwitch.setOnCheckedChangeListener { _, _ -> saveAndApplySettings() }
        binding.noToastSwitch.setOnCheckedChangeListener { _, _ -> saveAndApplySettings() }
        binding.privateDirsSwitch.setOnCheckedChangeListener { _, _ -> saveAndApplySettings() }

        binding.widthInput.doAfterTextChanged { validateResolution() }
        binding.heightInput.doAfterTextChanged { validateResolution() }

        binding.saveButton.setOnClickListener {
            if (validateAndSaveResolution()) {
                saveAndApplySettings()
                Toast.makeText(requireContext(), R.string.settings_saved, Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun validateResolution(): Boolean {
        val widthText = binding.widthInput.text?.toString() ?: ""
        val heightText = binding.heightInput.text?.toString() ?: ""

        val width = widthText.toIntOrNull() ?: 0
        val height = heightText.toIntOrNull() ?: 0

        val isValid = width in MIN_RESOLUTION..MAX_RESOLUTION && height in MIN_RESOLUTION..MAX_RESOLUTION
        binding.saveButton.isEnabled = isValid

        return isValid
    }

    private fun validateAndSaveResolution(): Boolean {
        val widthText = binding.widthInput.text?.toString() ?: ""
        val heightText = binding.heightInput.text?.toString() ?: ""

        val width = widthText.toIntOrNull()
        val height = heightText.toIntOrNull()

        if (width == null || height == null || 
            width !in MIN_RESOLUTION..MAX_RESOLUTION || 
            height !in MIN_RESOLUTION..MAX_RESOLUTION) {
            Toast.makeText(requireContext(), R.string.invalid_resolution, Toast.LENGTH_SHORT).show()
            return false
        }

        return true
    }

    private fun saveAndApplySettings() {
        val width = binding.widthInput.text?.toString()?.toIntOrNull() ?: DEFAULT_WIDTH
        val height = binding.heightInput.text?.toString()?.toIntOrNull() ?: DEFAULT_HEIGHT

        prefs.edit().apply {
            putBoolean(KEY_ENABLED, binding.enableSwitch.isChecked)
            putInt(KEY_WIDTH, width)
            putInt(KEY_HEIGHT, height)
            putBoolean(KEY_FLIP, binding.flipSwitch.isChecked)
            putBoolean(KEY_AUDIO_SYNC, binding.audioSyncSwitch.isChecked)
            putBoolean(KEY_NO_TOAST, binding.noToastSwitch.isChecked)
            putBoolean(KEY_PRIVATE_DIRS, binding.privateDirsSwitch.isChecked)
            apply()
        }

        // Apply settings to file system (control flags)
        applySettingsToFileSystem()
    }

    private fun updateEnableState(enabled: Boolean) {
        CoroutineScope(Dispatchers.Main).launch {
            withContext(Dispatchers.IO) {
                FileUtils.setEnabled(requireContext(), enabled)
            }
        }
    }

    private fun applySettingsToFileSystem() {
        val context = requireContext()

        CoroutineScope(Dispatchers.Main).launch {
            withContext(Dispatchers.IO) {
                // Apply enable/disable state
                FileUtils.setEnabled(context, binding.enableSwitch.isChecked)

                // Apply no toast setting
                FileUtils.setNoToast(context, binding.noToastSwitch.isChecked)

                // Save settings to a config file that the module can read
                val settings = mapOf(
                    "width" to (binding.widthInput.text?.toString() ?: DEFAULT_WIDTH.toString()),
                    "height" to (binding.heightInput.text?.toString() ?: DEFAULT_HEIGHT.toString()),
                    "flip" to binding.flipSwitch.isChecked.toString(),
                    "audio_sync" to binding.audioSyncSwitch.isChecked.toString(),
                    "private_dirs" to binding.privateDirsSwitch.isChecked.toString()
                )
                FileUtils.saveSettings(context, settings)
            }
        }
    }

    override fun onDestroyView() {
        super.onDestroyView()
        _binding = null
    }
}
