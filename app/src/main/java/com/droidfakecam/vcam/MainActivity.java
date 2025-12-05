package com.droidfakecam.vcam;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.widget.Button;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

/**
 * Main activity for DroidFakeCam configuration app.
 * This app provides a UI to configure the virtual camera settings
 * that work with the Magisk module.
 */
public class MainActivity extends Activity {

    private static final String TAG = "DroidFakeCam";
    private static final int PERMISSION_REQUEST_CODE = 1;
    private static final String CAMERA1_DIR = "/DCIM/Camera1";

    private Switch switchDisable;
    private Switch switchForceShow;
    private Switch switchPlaySound;
    private Switch switchPrivateDir;
    private Switch switchDisableToast;
    private TextView statusText;
    private TextView pathText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        initializeViews();
        setupListeners();
        checkAndRequestPermissions();
    }

    @Override
    protected void onResume() {
        super.onResume();
        syncStatusWithFiles();
        updateModuleStatus();
    }

    private void initializeViews() {
        switchDisable = findViewById(R.id.switch_disable);
        switchForceShow = findViewById(R.id.switch_force_show);
        switchPlaySound = findViewById(R.id.switch_play_sound);
        switchPrivateDir = findViewById(R.id.switch_private_dir);
        switchDisableToast = findViewById(R.id.switch_disable_toast);
        statusText = findViewById(R.id.text_status);
        pathText = findViewById(R.id.text_path);

        // Update path display
        String path = getCamera1Path();
        pathText.setText(getString(R.string.current_path, path));
    }

    private void setupListeners() {
        Button btnOpenFolder = findViewById(R.id.btn_open_folder);
        btnOpenFolder.setOnClickListener(v -> openCamera1Folder());

        Button btnSelectVideo = findViewById(R.id.btn_select_video);
        btnSelectVideo.setOnClickListener(v -> selectVideo());

        Button btnGithub = findViewById(R.id.btn_github);
        btnGithub.setOnClickListener(v -> openGithub());

        switchDisable.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (buttonView.isPressed()) {
                toggleConfigFile("disable.jpg", isChecked);
            }
        });

        switchForceShow.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (buttonView.isPressed()) {
                toggleConfigFile("force_show.jpg", isChecked);
            }
        });

        switchPlaySound.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (buttonView.isPressed()) {
                toggleConfigFile("no-silent.jpg", isChecked);
            }
        });

        switchPrivateDir.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (buttonView.isPressed()) {
                toggleConfigFile("private_dir.jpg", isChecked);
            }
        });

        switchDisableToast.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (buttonView.isPressed()) {
                toggleConfigFile("no_toast.jpg", isChecked);
            }
        });
    }

    private String getCamera1Path() {
        return Environment.getExternalStorageDirectory().getAbsolutePath() + CAMERA1_DIR;
    }

    private void toggleConfigFile(String filename, boolean create) {
        if (!hasStoragePermission()) {
            requestStoragePermission();
            syncStatusWithFiles();
            return;
        }

        File dir = new File(getCamera1Path());
        if (!dir.exists()) {
            dir.mkdirs();
        }

        File file = new File(dir, filename);
        try {
            if (create) {
                if (!file.exists()) {
                    file.createNewFile();
                }
            } else {
                if (file.exists()) {
                    file.delete();
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Error toggling config file: " + filename, e);
            Toast.makeText(this, R.string.error_file_operation, Toast.LENGTH_SHORT).show();
        }

        syncStatusWithFiles();
    }

    private void syncStatusWithFiles() {
        String path = getCamera1Path();
        File dir = new File(path);

        if (!dir.exists()) {
            dir.mkdirs();
        }

        switchDisable.setChecked(new File(dir, "disable.jpg").exists());
        switchForceShow.setChecked(new File(dir, "force_show.jpg").exists());
        switchPlaySound.setChecked(new File(dir, "no-silent.jpg").exists());
        switchPrivateDir.setChecked(new File(dir, "private_dir.jpg").exists());
        switchDisableToast.setChecked(new File(dir, "no_toast.jpg").exists());

        // Check for virtual.mp4
        File virtualVideo = new File(dir, "virtual.mp4");
        if (virtualVideo.exists()) {
            statusText.setText(R.string.status_video_found);
            statusText.setTextColor(getColor(android.R.color.holo_green_dark));
        } else {
            statusText.setText(R.string.status_video_not_found);
            statusText.setTextColor(getColor(android.R.color.holo_red_dark));
        }
    }

    private void updateModuleStatus() {
        // Check if Magisk module is installed by checking system property
        String moduleEnabled = System.getProperty("ro.droidfakecam.enabled", "false");
        boolean isModuleActive = "true".equals(moduleEnabled) || checkMagiskModuleInstalled();

        TextView moduleStatusText = findViewById(R.id.text_module_status);
        if (isModuleActive) {
            moduleStatusText.setText(R.string.module_status_active);
            moduleStatusText.setTextColor(getColor(android.R.color.holo_green_dark));
        } else {
            moduleStatusText.setText(R.string.module_status_inactive);
            moduleStatusText.setTextColor(getColor(android.R.color.holo_orange_dark));
        }
    }

    private boolean checkMagiskModuleInstalled() {
        // Check if the module directory exists
        File moduleDir = new File("/data/adb/modules/droidfakecam");
        return moduleDir.exists();
    }

    private void openCamera1Folder() {
        String path = getCamera1Path();
        File dir = new File(path);
        if (!dir.exists()) {
            dir.mkdirs();
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        Uri uri = Uri.parse("content://com.android.externalstorage.documents/document/primary:" + CAMERA1_DIR.substring(1));
        intent.setDataAndType(uri, "vnd.android.document/directory");
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        try {
            startActivity(intent);
        } catch (Exception e) {
            // Fallback: try to open with file manager
            Intent fileManager = new Intent(Intent.ACTION_GET_CONTENT);
            fileManager.setType("*/*");
            fileManager.addCategory(Intent.CATEGORY_OPENABLE);
            Toast.makeText(this, getString(R.string.folder_hint, path), Toast.LENGTH_LONG).show();
        }
    }

    private void selectVideo() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("video/*");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(intent, 2);
    }

    private void openGithub() {
        Intent intent = new Intent(Intent.ACTION_VIEW, 
            Uri.parse("https://github.com/lukesterbot-lab/droidfakecam"));
        startActivity(intent);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 2 && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            Toast.makeText(this, getString(R.string.video_selected_hint, getCamera1Path()), 
                Toast.LENGTH_LONG).show();
        }
    }

    private void checkAndRequestPermissions() {
        if (!hasStoragePermission()) {
            requestStoragePermission();
        }
    }

    private boolean hasStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
        }
        return true;
    }

    private void requestStoragePermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.permission_title)
                .setMessage(R.string.permission_message_all_files)
                .setPositiveButton(R.string.grant, (dialog, which) -> {
                    Intent intent = new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION);
                    startActivity(intent);
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.permission_title)
                .setMessage(R.string.permission_message_storage)
                .setPositiveButton(R.string.grant, (dialog, which) -> {
                    requestPermissions(new String[]{
                        Manifest.permission.READ_EXTERNAL_STORAGE,
                        Manifest.permission.WRITE_EXTERNAL_STORAGE
                    }, PERMISSION_REQUEST_CODE);
                })
                .setNegativeButton(R.string.cancel, null)
                .show();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean granted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    granted = false;
                    break;
                }
            }
            if (granted) {
                syncStatusWithFiles();
            } else {
                Toast.makeText(this, R.string.permission_denied, Toast.LENGTH_SHORT).show();
            }
        }
    }
}
