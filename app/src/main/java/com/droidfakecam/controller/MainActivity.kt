package com.droidfakecam.controller

import android.os.Bundle
import android.text.Editable
import android.text.TextWatcher
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.droidfakecam.controller.databinding.ActivityMainBinding
import java.io.BufferedReader
import java.io.InputStreamReader

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val targetDir by lazy { getString(R.string.target_dir) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.targetDirLabel.text = "Target dir: $targetDir"

        binding.copyVideoButton.setOnClickListener {
            copyWithSu(binding.videoPathInput.text.toString(), "virtual.mp4")
        }

        binding.copyImageButton.setOnClickListener {
            copyWithSu(binding.imagePathInput.text.toString(), "1000.bmp")
        }

        binding.enableButton.setOnClickListener {
            deleteControlFile("disable.jpg", "Virtual feed enabled")
        }

        binding.disableButton.setOnClickListener {
            touchControlFile("disable.jpg", "Virtual feed disabled (disable.jpg touched)")
        }

        binding.noToastButton.setOnClickListener {
            toggleControlFile("no_toast.jpg")
        }

        binding.privateDirButton.setOnClickListener {
            toggleControlFile("private_dir.jpg")
        }

        binding.videoPathInput.addTextChangedListener(loggingWatcher)
        binding.imagePathInput.addTextChangedListener(loggingWatcher)
    }

    private fun copyWithSu(sourcePath: String, destName: String) {
        if (sourcePath.isBlank()) {
            showMessage("Provide a source path for $destName")
            return
        }
        ensureCameraDir()
        val escapedSource = sourcePath.trim().replace("'", "\\'")
        val command = "cp '$escapedSource' '$targetDir/$destName'"
        val result = runSu(command)
        appendLog("cp -> $destName | success=${result.success} | ${result.output}")
        toastIfNeeded(result)
    }

    private fun touchControlFile(name: String, successMessage: String? = null) {
        ensureCameraDir()
        val command = "touch '$targetDir/$name'"
        val result = runSu(command)
        appendLog("touch $name | success=${result.success} | ${result.output}")
        toastIfNeeded(result, successMessage)
    }

    private fun deleteControlFile(name: String, successMessage: String? = null) {
        ensureCameraDir()
        val command = "rm -f '$targetDir/$name'"
        val result = runSu(command)
        appendLog("rm $name | success=${result.success} | ${result.output}")
        toastIfNeeded(result, successMessage)
    }

    private fun toggleControlFile(name: String) {
        val checkResult = runSu("[ -f '$targetDir/$name' ]")
        if (checkResult.success) {
            deleteControlFile(name, "$name removed")
        } else {
            touchControlFile(name, "$name created")
        }
    }

    private fun ensureCameraDir() {
        runSu("mkdir -p '$targetDir'")
    }

    private fun runSu(command: String): CommandResult {
        return try {
            val process = ProcessBuilder("su", "-c", command)
                .redirectErrorStream(true)
                .start()
            val output = BufferedReader(InputStreamReader(process.inputStream))
                .use { it.readText() }
            val exitCode = process.waitFor()
            CommandResult(exitCode == 0, output.ifBlank { command })
        } catch (t: Throwable) {
            CommandResult(false, t.message ?: "Unknown error")
        }
    }

    private fun appendLog(message: String) {
        val newContent = buildString {
            if (binding.logView.text.isNotBlank()) {
                append(binding.logView.text)
                append('\n')
            }
            append(message)
        }
        binding.logView.text = newContent
    }

    private fun toastIfNeeded(result: CommandResult, successMessage: String? = null) {
        val msg = if (result.success) successMessage ?: "Command succeeded" else "Command failed: ${result.output}"
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }

    private fun showMessage(message: String) {
        appendLog(message)
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    private val loggingWatcher = object : TextWatcher {
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) = Unit
        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) = Unit
        override fun afterTextChanged(s: Editable?) {
            appendLog("Input changed: ${s?.toString() ?: ""}")
        }
    }
}

data class CommandResult(val success: Boolean, val output: String)
