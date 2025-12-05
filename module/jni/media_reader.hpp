/*
 * DroidFakeCam - Media Reader Header
 * 
 * Provides functionality to read video and image files for frame injection.
 * Supports MP4 video and BMP image formats.
 * 
 * For educational and research purposes only.
 */

#pragma once

#include "frame_utils.hpp"
#include <string>
#include <vector>
#include <mutex>

class MediaReader {
public:
    MediaReader();
    ~MediaReader();
    
    // Open media file (video or image)
    bool open(const std::string& path);
    
    // Close and release resources
    void close();
    
    // Check if ready
    bool isReady() const { return m_ready; }
    
    // Get media info
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getFrameRate() const { return m_frameRate; }
    int64_t getDuration() const { return m_duration; }
    bool hasAudio() const { return m_hasAudio; }
    bool isVideo() const { return m_isVideo; }
    
    // Get next frame (loops for video)
    bool getNextFrame(FrameData& frame);
    
    // Get photo frame
    bool getPhotoFrame(FrameData& frame);
    
    // Seek to position (for video)
    bool seek(int64_t timestampUs);
    
    // Reset to beginning
    void reset();
    
    // Get current position
    int64_t getCurrentPosition() const { return m_currentPosition; }

private:
    bool m_ready;
    bool m_isVideo;
    bool m_hasAudio;
    
    std::string m_path;
    int m_width;
    int m_height;
    float m_frameRate;
    int64_t m_duration;  // microseconds
    int64_t m_currentPosition;
    
    // Decoded frame data
    std::vector<uint8_t> m_frameBuffer;
    int m_frameFormat;  // Format of stored frame
    
    // Video decoder state (using MediaCodec via NDK)
    void* m_mediaExtractor;  // AMediaExtractor*
    void* m_mediaCodec;      // AMediaCodec*
    int m_trackIndex;
    
    // For image files
    std::vector<uint8_t> m_imageData;
    
    std::mutex m_mutex;
    
    // Internal methods
    bool openVideo(const std::string& path);
    bool openImage(const std::string& path);
    bool decodeVideoFrame();
    bool loadBmpImage(const std::string& path);
    bool loadImage(const std::string& path);
};
