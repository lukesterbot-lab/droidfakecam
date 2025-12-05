/*
 * DroidFakeCam - Frame Utilities Header
 * 
 * Provides utilities for frame manipulation including:
 * - Resolution scaling/matching
 * - Color space conversion
 * - Front camera flipping (horizontal + rotation)
 * 
 * For educational and research purposes only.
 */

#pragma once

#include <cstdint>
#include <cstddef>

// Frame data structure
struct FrameData {
    uint8_t* data;
    size_t size;
    int width;
    int height;
    int format;  // 0=NV21, 1=YUV420, 2=RGBA, 3=RGB
    int stride;
    int64_t timestamp;
    
    FrameData() : data(nullptr), size(0), width(0), height(0), 
                  format(0), stride(0), timestamp(0) {}
    
    ~FrameData() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
    }
    
    // Prevent copying
    FrameData(const FrameData&) = delete;
    FrameData& operator=(const FrameData&) = delete;
    
    // Allow moving
    FrameData(FrameData&& other) noexcept {
        data = other.data;
        size = other.size;
        width = other.width;
        height = other.height;
        format = other.format;
        stride = other.stride;
        timestamp = other.timestamp;
        other.data = nullptr;
        other.size = 0;
    }
    
    FrameData& operator=(FrameData&& other) noexcept {
        if (this != &other) {
            if (data) delete[] data;
            data = other.data;
            size = other.size;
            width = other.width;
            height = other.height;
            format = other.format;
            stride = other.stride;
            timestamp = other.timestamp;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
};

namespace FrameUtils {

// Scale frame to target resolution
bool scaleFrame(const FrameData& src, FrameData& dst, 
                int targetWidth, int targetHeight);

// Convert frame format
bool convertFormat(const FrameData& src, FrameData& dst, int targetFormat);

// Flip frame horizontally (for front camera)
bool flipHorizontal(FrameData& frame);

// Rotate frame by 90 degrees clockwise
bool rotate90CW(const FrameData& src, FrameData& dst);

// Rotate frame by 90 degrees counter-clockwise
bool rotate90CCW(const FrameData& src, FrameData& dst);

// Rotate frame by 180 degrees
bool rotate180(FrameData& frame);

// Apply front camera transformation (horizontal flip + 90° rotation)
bool applyFrontCameraTransform(const FrameData& src, FrameData& dst);

// Match frame resolution to target (scale + pad if needed)
bool matchResolution(const FrameData& src, FrameData& dst,
                     int targetWidth, int targetHeight,
                     bool maintainAspect = true);

// Convert RGB to NV21 (common Android camera format)
bool rgbToNv21(const uint8_t* rgb, uint8_t* nv21, 
               int width, int height);

// Convert NV21 to RGB
bool nv21ToRgb(const uint8_t* nv21, uint8_t* rgb,
               int width, int height);

// Convert RGB to YUV420
bool rgbToYuv420(const uint8_t* rgb, uint8_t* yuv420,
                 int width, int height);

// Calculate buffer size for NV21 format
inline size_t calcNv21Size(int width, int height) {
    return width * height * 3 / 2;
}

// Calculate buffer size for YUV420 format
inline size_t calcYuv420Size(int width, int height) {
    return width * height * 3 / 2;
}

// Calculate buffer size for RGB format
inline size_t calcRgbSize(int width, int height) {
    return width * height * 3;
}

// Calculate buffer size for RGBA format
inline size_t calcRgbaSize(int width, int height) {
    return width * height * 4;
}

} // namespace FrameUtils
