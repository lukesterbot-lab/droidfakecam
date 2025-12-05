/*
 * DroidFakeCam - Frame Utilities Implementation
 * 
 * Implements frame manipulation functions for video processing.
 * Uses optimized algorithms for real-time performance.
 * 
 * For educational and research purposes only.
 */

#include "frame_utils.hpp"
#include <android/log.h>
#include <algorithm>
#include <cstring>
#include <cmath>

#define LOG_TAG "DroidFakeCam"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace FrameUtils {

// Clamp value to byte range
inline uint8_t clamp(int val) {
    return static_cast<uint8_t>(std::max(0, std::min(255, val)));
}

bool scaleFrame(const FrameData& src, FrameData& dst, 
                int targetWidth, int targetHeight) {
    if (!src.data || src.size == 0) {
        LOGE("scaleFrame: Invalid source frame");
        return false;
    }
    
    if (src.format != 2 && src.format != 3) {
        LOGE("scaleFrame: Only RGB/RGBA formats supported for scaling");
        return false;
    }
    
    int bpp = (src.format == 2) ? 4 : 3; // bytes per pixel
    
    dst.width = targetWidth;
    dst.height = targetHeight;
    dst.format = src.format;
    dst.stride = targetWidth * bpp;
    dst.size = dst.stride * targetHeight;
    dst.data = new uint8_t[dst.size];
    
    // Bilinear interpolation scaling
    float xRatio = static_cast<float>(src.width) / targetWidth;
    float yRatio = static_cast<float>(src.height) / targetHeight;
    
    for (int y = 0; y < targetHeight; y++) {
        float srcY = y * yRatio;
        int srcY0 = static_cast<int>(srcY);
        int srcY1 = std::min(srcY0 + 1, src.height - 1);
        float yFrac = srcY - srcY0;
        
        for (int x = 0; x < targetWidth; x++) {
            float srcX = x * xRatio;
            int srcX0 = static_cast<int>(srcX);
            int srcX1 = std::min(srcX0 + 1, src.width - 1);
            float xFrac = srcX - srcX0;
            
            int dstIdx = (y * targetWidth + x) * bpp;
            
            for (int c = 0; c < bpp; c++) {
                int srcIdx00 = (srcY0 * src.width + srcX0) * bpp + c;
                int srcIdx01 = (srcY0 * src.width + srcX1) * bpp + c;
                int srcIdx10 = (srcY1 * src.width + srcX0) * bpp + c;
                int srcIdx11 = (srcY1 * src.width + srcX1) * bpp + c;
                
                float val = src.data[srcIdx00] * (1 - xFrac) * (1 - yFrac) +
                           src.data[srcIdx01] * xFrac * (1 - yFrac) +
                           src.data[srcIdx10] * (1 - xFrac) * yFrac +
                           src.data[srcIdx11] * xFrac * yFrac;
                
                dst.data[dstIdx + c] = clamp(static_cast<int>(val));
            }
        }
    }
    
    LOGD("Scaled frame from %dx%d to %dx%d", 
         src.width, src.height, targetWidth, targetHeight);
    return true;
}

bool convertFormat(const FrameData& src, FrameData& dst, int targetFormat) {
    if (!src.data || src.size == 0) {
        LOGE("convertFormat: Invalid source frame");
        return false;
    }
    
    // Same format, just copy
    if (src.format == targetFormat) {
        dst.width = src.width;
        dst.height = src.height;
        dst.format = src.format;
        dst.stride = src.stride;
        dst.size = src.size;
        dst.data = new uint8_t[dst.size];
        memcpy(dst.data, src.data, src.size);
        return true;
    }
    
    dst.width = src.width;
    dst.height = src.height;
    dst.format = targetFormat;
    
    // RGB (3) to NV21 (0)
    if (src.format == 3 && targetFormat == 0) {
        dst.size = calcNv21Size(src.width, src.height);
        dst.data = new uint8_t[dst.size];
        return rgbToNv21(src.data, dst.data, src.width, src.height);
    }
    
    // RGB (3) to YUV420 (1)
    if (src.format == 3 && targetFormat == 1) {
        dst.size = calcYuv420Size(src.width, src.height);
        dst.data = new uint8_t[dst.size];
        return rgbToYuv420(src.data, dst.data, src.width, src.height);
    }
    
    // NV21 (0) to RGB (3)
    if (src.format == 0 && targetFormat == 3) {
        dst.size = calcRgbSize(src.width, src.height);
        dst.stride = src.width * 3;
        dst.data = new uint8_t[dst.size];
        return nv21ToRgb(src.data, dst.data, src.width, src.height);
    }
    
    LOGE("convertFormat: Unsupported conversion %d -> %d", 
         src.format, targetFormat);
    return false;
}

bool flipHorizontal(FrameData& frame) {
    if (!frame.data || frame.size == 0) {
        return false;
    }
    
    int bpp = 3;
    if (frame.format == 2) bpp = 4;  // RGBA
    else if (frame.format == 0 || frame.format == 1) {
        LOGE("flipHorizontal: NV21/YUV420 flip not implemented");
        return false;
    }
    
    for (int y = 0; y < frame.height; y++) {
        for (int x = 0; x < frame.width / 2; x++) {
            int leftIdx = (y * frame.width + x) * bpp;
            int rightIdx = (y * frame.width + (frame.width - 1 - x)) * bpp;
            
            for (int c = 0; c < bpp; c++) {
                std::swap(frame.data[leftIdx + c], frame.data[rightIdx + c]);
            }
        }
    }
    
    LOGD("Flipped frame horizontally");
    return true;
}

bool rotate90CW(const FrameData& src, FrameData& dst) {
    if (!src.data || src.size == 0) {
        return false;
    }
    
    int bpp = 3;
    if (src.format == 2) bpp = 4;
    else if (src.format != 3) {
        LOGE("rotate90CW: Only RGB/RGBA formats supported");
        return false;
    }
    
    // Rotated dimensions are swapped
    dst.width = src.height;
    dst.height = src.width;
    dst.format = src.format;
    dst.stride = dst.width * bpp;
    dst.size = dst.stride * dst.height;
    dst.data = new uint8_t[dst.size];
    
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int srcIdx = (y * src.width + x) * bpp;
            int dstX = src.height - 1 - y;
            int dstY = x;
            int dstIdx = (dstY * dst.width + dstX) * bpp;
            
            for (int c = 0; c < bpp; c++) {
                dst.data[dstIdx + c] = src.data[srcIdx + c];
            }
        }
    }
    
    LOGD("Rotated frame 90° CW: %dx%d -> %dx%d", 
         src.width, src.height, dst.width, dst.height);
    return true;
}

bool rotate90CCW(const FrameData& src, FrameData& dst) {
    if (!src.data || src.size == 0) {
        return false;
    }
    
    int bpp = 3;
    if (src.format == 2) bpp = 4;
    else if (src.format != 3) {
        LOGE("rotate90CCW: Only RGB/RGBA formats supported");
        return false;
    }
    
    dst.width = src.height;
    dst.height = src.width;
    dst.format = src.format;
    dst.stride = dst.width * bpp;
    dst.size = dst.stride * dst.height;
    dst.data = new uint8_t[dst.size];
    
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            int srcIdx = (y * src.width + x) * bpp;
            int dstX = y;
            int dstY = src.width - 1 - x;
            int dstIdx = (dstY * dst.width + dstX) * bpp;
            
            for (int c = 0; c < bpp; c++) {
                dst.data[dstIdx + c] = src.data[srcIdx + c];
            }
        }
    }
    
    LOGD("Rotated frame 90° CCW: %dx%d -> %dx%d",
         src.width, src.height, dst.width, dst.height);
    return true;
}

bool rotate180(FrameData& frame) {
    if (!frame.data || frame.size == 0) {
        return false;
    }
    
    int bpp = 3;
    if (frame.format == 2) bpp = 4;
    else if (frame.format != 3) {
        LOGE("rotate180: Only RGB/RGBA formats supported");
        return false;
    }
    
    int totalPixels = frame.width * frame.height;
    
    for (int i = 0; i < totalPixels / 2; i++) {
        int j = totalPixels - 1 - i;
        for (int c = 0; c < bpp; c++) {
            std::swap(frame.data[i * bpp + c], frame.data[j * bpp + c]);
        }
    }
    
    LOGD("Rotated frame 180°");
    return true;
}

bool applyFrontCameraTransform(const FrameData& src, FrameData& dst) {
    // For front camera: horizontal flip + 90° rotation
    
    // First, copy and flip horizontally
    FrameData flipped;
    flipped.width = src.width;
    flipped.height = src.height;
    flipped.format = src.format;
    flipped.stride = src.stride;
    flipped.size = src.size;
    flipped.data = new uint8_t[flipped.size];
    memcpy(flipped.data, src.data, src.size);
    
    if (!flipHorizontal(flipped)) {
        return false;
    }
    
    // Then rotate 90° clockwise
    return rotate90CW(flipped, dst);
}

bool matchResolution(const FrameData& src, FrameData& dst,
                     int targetWidth, int targetHeight,
                     bool maintainAspect) {
    if (!src.data || src.size == 0) {
        return false;
    }
    
    if (src.width == targetWidth && src.height == targetHeight) {
        // Already matching, just copy
        dst.width = src.width;
        dst.height = src.height;
        dst.format = src.format;
        dst.stride = src.stride;
        dst.size = src.size;
        dst.data = new uint8_t[dst.size];
        memcpy(dst.data, src.data, src.size);
        return true;
    }
    
    if (!maintainAspect) {
        return scaleFrame(src, dst, targetWidth, targetHeight);
    }
    
    // Calculate aspect-ratio-preserving dimensions
    float srcAspect = static_cast<float>(src.width) / src.height;
    float dstAspect = static_cast<float>(targetWidth) / targetHeight;
    
    int scaleWidth, scaleHeight;
    
    if (srcAspect > dstAspect) {
        // Source is wider, fit to width
        scaleWidth = targetWidth;
        scaleHeight = static_cast<int>(targetWidth / srcAspect);
    } else {
        // Source is taller, fit to height
        scaleHeight = targetHeight;
        scaleWidth = static_cast<int>(targetHeight * srcAspect);
    }
    
    // Scale the frame
    FrameData scaled;
    if (!scaleFrame(src, scaled, scaleWidth, scaleHeight)) {
        return false;
    }
    
    // Create output with padding
    int bpp = (src.format == 2) ? 4 : 3;
    dst.width = targetWidth;
    dst.height = targetHeight;
    dst.format = src.format;
    dst.stride = targetWidth * bpp;
    dst.size = dst.stride * targetHeight;
    dst.data = new uint8_t[dst.size];
    
    // Fill with black
    memset(dst.data, 0, dst.size);
    
    // Calculate offset for centering
    int offsetX = (targetWidth - scaleWidth) / 2;
    int offsetY = (targetHeight - scaleHeight) / 2;
    
    // Copy scaled frame into center
    for (int y = 0; y < scaleHeight; y++) {
        int srcRow = y * scaleWidth * bpp;
        int dstRow = ((y + offsetY) * targetWidth + offsetX) * bpp;
        memcpy(dst.data + dstRow, scaled.data + srcRow, scaleWidth * bpp);
    }
    
    LOGD("Matched resolution %dx%d -> %dx%d (scaled to %dx%d, padded)",
         src.width, src.height, targetWidth, targetHeight,
         scaleWidth, scaleHeight);
    
    return true;
}

bool rgbToNv21(const uint8_t* rgb, uint8_t* nv21, int width, int height) {
    if (!rgb || !nv21 || width <= 0 || height <= 0) {
        return false;
    }
    
    int ySize = width * height;
    int uvSize = width * height / 2;
    
    uint8_t* yPlane = nv21;
    uint8_t* uvPlane = nv21 + ySize;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int rgbIdx = (j * width + i) * 3;
            int r = rgb[rgbIdx];
            int g = rgb[rgbIdx + 1];
            int b = rgb[rgbIdx + 2];
            
            // RGB to Y
            int y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            yPlane[j * width + i] = clamp(y);
            
            // RGB to UV (sample every 2x2 block)
            if (j % 2 == 0 && i % 2 == 0) {
                int u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                int v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                
                int uvIdx = (j / 2) * width + i;
                uvPlane[uvIdx] = clamp(v);      // V first in NV21
                uvPlane[uvIdx + 1] = clamp(u);  // Then U
            }
        }
    }
    
    return true;
}

bool nv21ToRgb(const uint8_t* nv21, uint8_t* rgb, int width, int height) {
    if (!nv21 || !rgb || width <= 0 || height <= 0) {
        return false;
    }
    
    int ySize = width * height;
    const uint8_t* yPlane = nv21;
    const uint8_t* uvPlane = nv21 + ySize;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int y = yPlane[j * width + i];
            int uvIdx = (j / 2) * width + (i & ~1);
            int v = uvPlane[uvIdx] - 128;
            int u = uvPlane[uvIdx + 1] - 128;
            
            // YUV to RGB
            int c = y - 16;
            int r = (298 * c + 409 * v + 128) >> 8;
            int g = (298 * c - 100 * u - 208 * v + 128) >> 8;
            int b = (298 * c + 516 * u + 128) >> 8;
            
            int rgbIdx = (j * width + i) * 3;
            rgb[rgbIdx] = clamp(r);
            rgb[rgbIdx + 1] = clamp(g);
            rgb[rgbIdx + 2] = clamp(b);
        }
    }
    
    return true;
}

bool rgbToYuv420(const uint8_t* rgb, uint8_t* yuv420, int width, int height) {
    if (!rgb || !yuv420 || width <= 0 || height <= 0) {
        return false;
    }
    
    int ySize = width * height;
    int uSize = width * height / 4;
    
    uint8_t* yPlane = yuv420;
    uint8_t* uPlane = yuv420 + ySize;
    uint8_t* vPlane = uPlane + uSize;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            int rgbIdx = (j * width + i) * 3;
            int r = rgb[rgbIdx];
            int g = rgb[rgbIdx + 1];
            int b = rgb[rgbIdx + 2];
            
            // RGB to Y
            int y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
            yPlane[j * width + i] = clamp(y);
            
            // RGB to UV (sample every 2x2 block)
            if (j % 2 == 0 && i % 2 == 0) {
                int u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
                int v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
                
                int uvIdx = (j / 2) * (width / 2) + (i / 2);
                uPlane[uvIdx] = clamp(u);
                vPlane[uvIdx] = clamp(v);
            }
        }
    }
    
    return true;
}

} // namespace FrameUtils
