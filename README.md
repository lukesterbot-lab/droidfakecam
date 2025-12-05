# DroidFakeCam

A virtual camera injection solution for Android using Magisk, without requiring LSPosed or Xposed framework.

## ⚠️ WARNING
**DO NOT USE FOR ANY ILLEGAL PURPOSE! YOU ARE RESPONSIBLE FOR ALL CONSEQUENCES OF USING THIS SOFTWARE!**

## Overview

DroidFakeCam consists of two components:
1. **Magisk Module**: Handles system-level camera injection setup
2. **Android App**: Provides configuration UI and settings management

This project is inspired by [android_virtual_cam](https://github.com/w2016561536/android_virtual_cam) but redesigned to work with Magisk instead of Xposed/LSPosed.

## Features

- Replace camera preview with custom video
- Replace captured photos with custom images
- No Xposed/LSPosed required - works with Magisk only
- Easy configuration through companion app
- Support for both Camera1 and Camera2 APIs
- Per-app video assignment support
- Sound playback control

## Requirements

- Android 5.0+ (API 21+)
- Magisk v20.4 or higher
- Root access

## Installation

### Magisk Module
1. Download the `droidfakecam-magisk.zip` from releases
2. Open Magisk Manager
3. Go to Modules → Install from storage
4. Select the downloaded zip file
5. Reboot your device

### Android App
1. Download the `droidfakecam.apk` from releases
2. Install the APK
3. Grant storage permissions when prompted

## Usage

### Basic Setup
1. Install both the Magisk module and the app
2. Reboot your device
3. Open the DroidFakeCam app
4. Place your replacement video as `virtual.mp4` in `/sdcard/DCIM/Camera1/`
5. Open any camera app - it will display your video instead

### Video Requirements
- Format: MP4 (H.264 codec recommended)
- Resolution: Should match what the target app requests (shown in toast notification)
- The video will loop automatically

### Photo Replacement
- Place a photo named `1000.bmp` in `/sdcard/DCIM/Camera1/`
- Resolution should match the camera's capture resolution
- Supports other formats renamed to .bmp

### Configuration Files
Create these files in `/sdcard/DCIM/Camera1/` to control behavior:
- `disable.jpg` - Temporarily disable the module
- `no-silent.jpg` - Enable video sound playback
- `no_toast.jpg` - Disable toast notifications
- `force_show.jpg` - Force show permission warnings
- `private_dir.jpg` - Use private directory per app

## Directory Structure

```
/sdcard/DCIM/Camera1/
├── virtual.mp4      # Replacement video
├── 1000.bmp         # Replacement photo (optional)
├── disable.jpg      # Disable module (optional)
├── no-silent.jpg    # Enable sound (optional)
├── no_toast.jpg     # Disable toasts (optional)
├── force_show.jpg   # Force warnings (optional)
└── private_dir.jpg  # Private dir mode (optional)
```

## Building from Source

### Prerequisites
- Android Studio Arctic Fox or later
- Android SDK 34
- JDK 11 or later

### Build Magisk Module
```bash
cd magisk_module
zip -r droidfakecam-magisk.zip *
```

### Build Android App
```bash
./gradlew assembleRelease
```

## Project Structure

```
droidfakecam/
├── magisk_module/           # Magisk module files
│   ├── META-INF/           # Magisk installer scripts
│   ├── module.prop         # Module metadata
│   ├── customize.sh        # Installation script
│   ├── service.sh          # Boot service script
│   ├── post-fs-data.sh     # Early init script
│   └── system.prop         # System properties
├── app/                     # Android app
│   ├── src/main/
│   │   ├── java/           # Java source code
│   │   ├── res/            # Resources
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── build.gradle            # Root build configuration
└── settings.gradle
```

## Troubleshooting

### Black Screen / Camera Fails
- Ensure `virtual.mp4` exists in the correct directory
- Check video format is compatible (H.264 MP4)
- Some system camera apps may not be hookable

### Blurred/Distorted Image
- Adjust video resolution to match camera preview size
- Use video editing software to match aspect ratio

### Module Not Working
- Ensure Magisk is properly installed
- Check module status in Magisk Manager
- Reboot after installing the module

## Credits

- Original concept: [android_virtual_cam](https://github.com/w2016561536/android_virtual_cam) by w2016561536
- Hook method: [CameraHook](https://github.com/wangwei1237/CameraHook)
- H.264 decoding: [Android-VideoToImages](https://github.com/zhantong/Android-VideoToImages)

## License

MIT License - See [LICENSE](LICENSE) file for details