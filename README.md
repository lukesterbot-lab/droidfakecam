# FakeCam Controller

An Android application that serves as the controller for the FakeCam Magisk virtual camera module. This app allows you to replace your camera feed with custom video or image content.

## Features

- **Media Selection**: Select video (.mp4) or image (.bmp) files from device storage
- **Live Preview**: Preview selected media with thumbnail support via Glide
- **Settings Control**: 
  - Toggle FakeCam on/off
  - Set custom resolution (width/height)
  - Flip front camera option
  - Audio sync toggle
  - Disable toast notifications
  - Private directories mode
- **File Management**: Automatically copies media to `/sdcard/DCIM/Camera1/` for the module
- **IPC Service**: Binder service for real-time communication with the Magisk module
- **Material Design UI**: Modern, clean interface with bottom navigation
- **In-App Help**: Step-by-step instructions for setup and usage

## Requirements

- Android 8.0+ (API 26+)
- Magisk FakeCam module installed (for virtual camera functionality)
- Storage permissions for media access

## Permissions

- `READ_EXTERNAL_STORAGE` (Android 12 and below)
- `READ_MEDIA_VIDEO` (Android 13+)
- `READ_MEDIA_IMAGES` (Android 13+)
- `MANAGE_EXTERNAL_STORAGE` (for writing to DCIM folder)

## Building

### Prerequisites

- Android Studio Arctic Fox or newer
- JDK 17
- Android SDK with API 34

### Build Steps

1. Clone this repository
2. Open in Android Studio
3. Sync Gradle files
4. Build the project:
   ```bash
   ./gradlew build
   ```
5. Generate APK:
   ```bash
   ./gradlew assembleDebug
   ```

The APK will be generated at `app/build/outputs/apk/debug/app-debug.apk`

## Project Structure

```
app/
├── src/main/
│   ├── java/com/fakecam/controller/
│   │   ├── MainActivity.kt          # Main activity with navigation
│   │   ├── ui/
│   │   │   ├── MediaFragment.kt     # Media selection screen
│   │   │   ├── SettingsFragment.kt  # Settings configuration
│   │   │   └── HelpFragment.kt      # Help and instructions
│   │   ├── service/
│   │   │   └── FakeCamService.kt    # Binder IPC service
│   │   └── utils/
│   │       ├── FileUtils.kt         # File operations
│   │       └── MediaValidator.kt    # File validation
│   ├── aidl/com/fakecam/controller/
│   │   └── IFakeCamService.aidl     # AIDL interface for IPC
│   └── res/
│       ├── layout/                   # XML layouts
│       ├── values/                   # Strings, colors, themes
│       ├── menu/                     # Navigation menu
│       └── drawable/                 # Icons and graphics
├── build.gradle.kts                  # App-level build config
└── proguard-rules.pro               # ProGuard configuration
```

## How It Works

1. **Media Selection**: Users select video or image files from the device
2. **Validation**: Files are validated for format and resolution
3. **Copy to Camera1**: Media is copied to `/sdcard/DCIM/Camera1/`
4. **Module Detection**: The Magisk module reads from this directory
5. **Camera Feed Replacement**: When any app uses the camera, the module provides the selected content instead

## Control Flags

The app manages control files in `/sdcard/DCIM/Camera1/`:

- `virtual.mp4` / `virtual.bmp` - The media content
- `disable.jpg` - Presence disables the virtual camera
- `no_toast.jpg` - Disables module toast notifications
- `settings.conf` - Resolution and other settings

## IPC Service

The app exposes a Binder service (`IFakeCamService`) that other apps or the Magisk module can use:

```kotlin
interface IFakeCamService {
    boolean isModuleActive()
    boolean setMediaPath(String path)
    String getCurrentMediaPath()
    boolean setEnabled(boolean enabled)
    boolean isEnabled()
    boolean setResolution(int width, int height)
    boolean refresh()
}
```

## Dependencies

- AndroidX Core KTX
- Material Design Components
- Glide for image loading
- Navigation Component
- Lifecycle ViewModel/LiveData
- Kotlin Coroutines

## License

This project is provided as-is for educational and personal use.