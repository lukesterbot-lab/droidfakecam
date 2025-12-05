# DroidFakeCam

A Magisk/Zygisk module for Android virtual camera feed injection. This module hooks into the Android camera subsystem to replace live camera frames with custom video or image content.

**⚠️ DISCLAIMER: This project is for educational and research purposes only. Use responsibly and in compliance with applicable laws.**

## Features

- 🎬 Replace camera feed with custom video (MP4)
- 🖼️ Replace photo capture with custom images (BMP)
- 📱 Works with Camera2 API and NDK camera libraries
- 🔄 Automatic resolution matching and scaling
- 📷 Front camera transformation (horizontal flip + rotation)
- 🎵 Audio sync support for video with sound
- ⚙️ Easy configuration via control files
- 📝 Debug logging via logcat

## Requirements

- **Android 8.0+** (API 26+)
- **Rooted device** with Magisk 24.0+
- **Zygisk enabled** in Magisk settings
- Android NDK r23+ (for building)

## Installation

### Pre-built Module

1. Download the latest `DroidFakeCam-vX.X.X.zip` from releases
2. Open **Magisk Manager**
3. Go to **Modules** → **Install from storage**
4. Select the downloaded ZIP file
5. Reboot your device

### Build from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/droidfakecam.git
cd droidfakecam

# Set NDK path (if not already set)
export ANDROID_NDK=/path/to/android-ndk

# Build the module
chmod +x build.sh
./build.sh

# The module ZIP will be in out/DroidFakeCam-v1.0.0.zip
```

## Usage

### Setting Up Media Files

Place your custom media files in `/sdcard/DCIM/Camera1/`:

| File | Description |
|------|-------------|
| `virtual.mp4` | Video file for live camera feed |
| `1000.bmp` | Image file for photo capture |

### Control Files

Create empty files in `/sdcard/DCIM/Camera1/` to control behavior:

| File | Effect |
|------|--------|
| `disable.jpg` | Disable virtual camera (use real camera) |
| `no_toast.jpg` | Suppress debug log messages |
| `private_dir.jpg` | Use app-specific media directories |

### App-Specific Configuration

When `private_dir.jpg` exists, the module looks for media in app-specific directories:

```
/sdcard/DCIM/Camera1/com.example.app/virtual.mp4
/sdcard/DCIM/Camera1/com.example.app/1000.bmp
```

### Debugging

View debug logs using logcat:

```bash
adb logcat -s DroidFakeCam
```

## Module Structure

```
droidfakecam/
├── module/
│   ├── module.prop        # Module metadata
│   ├── system.prop        # System properties
│   ├── customize.sh       # Installation script
│   ├── service.sh         # Boot-time service
│   ├── zygisk/            # Native libraries
│   │   ├── arm64-v8a.so
│   │   ├── armeabi-v7a.so
│   │   ├── x86.so
│   │   └── x86_64.so
│   └── jni/               # Native source code
│       ├── zygisk_main.cpp
│       ├── camera_hook.cpp
│       ├── frame_utils.cpp
│       ├── media_reader.cpp
│       └── ...
├── build.sh               # Build script
└── README.md
```

## Supported Apps

The module is designed to work with any app that uses Android's Camera2 API or NDK camera libraries. Tested with:

- Stock Android camera apps
- Google Camera
- Third-party camera apps
- Video calling apps (Zoom, Teams, Meet, etc.)
- Social media apps (WhatsApp, Instagram, TikTok, etc.)

## Technical Details

### Hooking Mechanism

The module uses Zygisk to inject into app processes at startup. It hooks:

1. **Camera2 NDK API** (`libcamera2ndk.so`)
   - `ACameraOutputTarget_create`
   - `ACameraCaptureSession_capture`

2. **Media NDK API** (`libmediandk.so`)
   - `AImageReader_acquireNextImage`
   - `AImage_getPlaneData`

### Frame Processing

- Frames are decoded from source video/image
- Resolution is matched to camera output size
- Color format is converted (RGB → NV21/YUV420)
- Front camera applies horizontal flip + 90° rotation

### Supported Formats

- **Video**: MP4, 3GP, MKV, WebM (H.264/H.265)
- **Image**: BMP (24-bit/32-bit uncompressed)

## Building

### Prerequisites

- Linux or macOS (Windows via WSL)
- Android NDK r23 or later
- bash shell

### Build Commands

```bash
# Build for all architectures
./build.sh

# Build for specific architecture
./build.sh arm64
./build.sh arm
./build.sh x86
./build.sh x86_64

# Clean build artifacts
./build.sh clean
```

### Using CMake (Alternative)

```bash
mkdir build && cd build

# For arm64-v8a
cmake -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-26 \
      ../module/jni

make
```

## Troubleshooting

### Module not loading

1. Ensure Zygisk is enabled in Magisk settings
2. Check if module is enabled in Magisk → Modules
3. Verify the module ZIP structure is correct

### Virtual camera not working

1. Check if media files exist in `/sdcard/DCIM/Camera1/`
2. Ensure media files are in supported formats
3. Check logcat for error messages

### Permission issues

```bash
# Ensure proper permissions for media directory
adb shell chmod -R 755 /sdcard/DCIM/Camera1/
```

### Video playback issues

- Use H.264 encoded MP4 for best compatibility
- Keep resolution reasonable (1080p or lower)
- Ensure video file is not corrupted

## License

This project is provided for educational and research purposes. Use at your own risk.

## Credits

- Inspired by the original virtual camera concept
- Built on the Zygisk framework by topjohnwu
- Uses Android NDK media APIs

## Changelog

### v1.0.0

- Initial release
- Zygisk-based camera hooking
- Video and image file support
- Resolution matching and frame transformation
- Configuration via control files