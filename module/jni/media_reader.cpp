/*
 * DroidFakeCam - Media Reader Implementation
 * 
 * Reads video and image files using Android NDK media APIs.
 * Supports:
 * - MP4/H.264 video via MediaCodec
 * - BMP image files
 * 
 * For educational and research purposes only.
 */

#include "media_reader.hpp"
#include <android/log.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

#define LOG_TAG "DroidFakeCam"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// BMP file header structures
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t signature;      // 'BM'
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t dataOffset;
};

struct BMPInfoHeader {
    uint32_t headerSize;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;
    uint32_t imageSize;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};
#pragma pack(pop)

MediaReader::MediaReader()
    : m_ready(false)
    , m_isVideo(false)
    , m_hasAudio(false)
    , m_width(0)
    , m_height(0)
    , m_frameRate(30.0f)
    , m_duration(0)
    , m_currentPosition(0)
    , m_frameFormat(3)  // RGB
    , m_mediaExtractor(nullptr)
    , m_mediaCodec(nullptr)
    , m_trackIndex(-1)
{
}

MediaReader::~MediaReader() {
    close();
}

bool MediaReader::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    close();
    m_path = path;
    
    // Determine file type by extension
    size_t dotPos = path.rfind('.');
    if (dotPos == std::string::npos) {
        LOGE("Cannot determine file type: %s", path.c_str());
        return false;
    }
    
    std::string ext = path.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".mp4" || ext == ".3gp" || ext == ".mkv" || ext == ".webm") {
        return openVideo(path);
    } else if (ext == ".bmp") {
        return loadBmpImage(path);
    } else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
        return loadImage(path);
    }
    
    LOGE("Unsupported file format: %s", ext.c_str());
    return false;
}

void MediaReader::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_mediaCodec) {
        AMediaCodec_stop((AMediaCodec*)m_mediaCodec);
        AMediaCodec_delete((AMediaCodec*)m_mediaCodec);
        m_mediaCodec = nullptr;
    }
    
    if (m_mediaExtractor) {
        AMediaExtractor_delete((AMediaExtractor*)m_mediaExtractor);
        m_mediaExtractor = nullptr;
    }
    
    m_frameBuffer.clear();
    m_imageData.clear();
    m_ready = false;
    m_width = 0;
    m_height = 0;
    m_duration = 0;
    m_currentPosition = 0;
    m_trackIndex = -1;
}

bool MediaReader::openVideo(const std::string& path) {
    LOGI("Opening video: %s", path.c_str());
    
    // Create media extractor
    AMediaExtractor* extractor = AMediaExtractor_new();
    if (!extractor) {
        LOGE("Failed to create media extractor");
        return false;
    }
    
    // Open file
    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open file: %s", path.c_str());
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    struct stat st;
    if (fstat(fd, &st) != 0) {
        LOGE("Failed to stat file");
        ::close(fd);
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    media_status_t status = AMediaExtractor_setDataSourceFd(
        extractor, fd, 0, st.st_size);
    ::close(fd);
    
    if (status != AMEDIA_OK) {
        LOGE("Failed to set data source: %d", status);
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    // Find video track
    int numTracks = AMediaExtractor_getTrackCount(extractor);
    LOGD("Found %d tracks", numTracks);
    
    m_trackIndex = -1;
    AMediaFormat* format = nullptr;
    
    for (int i = 0; i < numTracks; i++) {
        format = AMediaExtractor_getTrackFormat(extractor, i);
        const char* mime = nullptr;
        if (AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
            LOGD("Track %d: %s", i, mime);
            
            if (strncmp(mime, "video/", 6) == 0) {
                m_trackIndex = i;
                
                // Get video dimensions
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &m_width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &m_height);
                
                int32_t frameRate = 0;
                if (AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate)) {
                    m_frameRate = static_cast<float>(frameRate);
                }
                
                int64_t duration = 0;
                if (AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &duration)) {
                    m_duration = duration;
                }
                
                LOGI("Video: %dx%d @ %.1f fps, duration: %lld us",
                     m_width, m_height, m_frameRate, (long long)m_duration);
                break;
            } else if (strncmp(mime, "audio/", 6) == 0) {
                m_hasAudio = true;
            }
        }
        AMediaFormat_delete(format);
        format = nullptr;
    }
    
    if (m_trackIndex < 0) {
        LOGE("No video track found");
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    // Select video track
    AMediaExtractor_selectTrack(extractor, m_trackIndex);
    
    // Create decoder
    const char* mime = nullptr;
    AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);
    
    AMediaCodec* codec = AMediaCodec_createDecoderByType(mime);
    if (!codec) {
        LOGE("Failed to create decoder for %s", mime);
        AMediaFormat_delete(format);
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    // Configure decoder
    status = AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
    AMediaFormat_delete(format);
    
    if (status != AMEDIA_OK) {
        LOGE("Failed to configure decoder: %d", status);
        AMediaCodec_delete(codec);
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    // Start decoder
    status = AMediaCodec_start(codec);
    if (status != AMEDIA_OK) {
        LOGE("Failed to start decoder: %d", status);
        AMediaCodec_delete(codec);
        AMediaExtractor_delete(extractor);
        return false;
    }
    
    m_mediaExtractor = extractor;
    m_mediaCodec = codec;
    m_isVideo = true;
    m_ready = true;
    
    // Decode first frame
    decodeVideoFrame();
    
    LOGI("Video opened successfully");
    return true;
}

bool MediaReader::decodeVideoFrame() {
    if (!m_mediaExtractor || !m_mediaCodec) {
        return false;
    }
    
    AMediaExtractor* extractor = (AMediaExtractor*)m_mediaExtractor;
    AMediaCodec* codec = (AMediaCodec*)m_mediaCodec;
    
    // Timeout for codec operations
    const int64_t kTimeoutUs = 10000; // 10ms
    
    bool gotFrame = false;
    
    while (!gotFrame) {
        // Try to get an input buffer
        ssize_t inputIndex = AMediaCodec_dequeueInputBuffer(codec, kTimeoutUs);
        if (inputIndex >= 0) {
            size_t bufferSize = 0;
            uint8_t* inputBuffer = AMediaCodec_getInputBuffer(
                codec, inputIndex, &bufferSize);
            
            if (inputBuffer) {
                // Read sample from extractor
                ssize_t sampleSize = AMediaExtractor_readSampleData(
                    extractor, inputBuffer, bufferSize);
                
                if (sampleSize >= 0) {
                    int64_t presentationTime = AMediaExtractor_getSampleTime(extractor);
                    uint32_t flags = 0;
                    
                    AMediaCodec_queueInputBuffer(
                        codec, inputIndex, 0, sampleSize, 
                        presentationTime, flags);
                    
                    AMediaExtractor_advance(extractor);
                } else {
                    // End of stream - loop back to beginning
                    AMediaExtractor_seekTo(extractor, 0, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
                    AMediaCodec_queueInputBuffer(
                        codec, inputIndex, 0, 0, 0, 0);
                }
            }
        }
        
        // Try to get an output buffer
        AMediaCodecBufferInfo bufferInfo;
        ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(
            codec, &bufferInfo, kTimeoutUs);
        
        if (outputIndex >= 0) {
            if (bufferInfo.size > 0) {
                size_t outSize = 0;
                uint8_t* outputBuffer = AMediaCodec_getOutputBuffer(
                    codec, outputIndex, &outSize);
                
                if (outputBuffer && bufferInfo.size > 0) {
                    // Store the decoded frame
                    m_frameBuffer.resize(bufferInfo.size);
                    memcpy(m_frameBuffer.data(), outputBuffer, bufferInfo.size);
                    m_currentPosition = bufferInfo.presentationTimeUs;
                    gotFrame = true;
                    
                    LOGD("Decoded frame at %lld us, size=%d",
                         (long long)m_currentPosition, bufferInfo.size);
                }
            }
            
            AMediaCodec_releaseOutputBuffer(codec, outputIndex, false);
        } else if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
            if (format) {
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &m_width);
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &m_height);
                
                int32_t colorFormat = 0;
                AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat);
                
                LOGD("Output format changed: %dx%d, color=%d",
                     m_width, m_height, colorFormat);
                
                AMediaFormat_delete(format);
            }
        }
    }
    
    return gotFrame;
}

bool MediaReader::loadBmpImage(const std::string& path) {
    LOGI("Loading BMP image: %s", path.c_str());
    
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        LOGE("Failed to open BMP file: %s", path.c_str());
        return false;
    }
    
    // Read file header
    BMPFileHeader fileHeader;
    if (fread(&fileHeader, sizeof(fileHeader), 1, file) != 1) {
        LOGE("Failed to read BMP file header");
        fclose(file);
        return false;
    }
    
    // BMP file signature constant
    static constexpr uint16_t BMP_SIGNATURE = 0x4D42;  // 'BM'
    
    // Validate signature
    if (fileHeader.signature != BMP_SIGNATURE) {
        LOGE("Invalid BMP signature: 0x%04X", fileHeader.signature);
        fclose(file);
        return false;
    }
    
    // Read info header
    BMPInfoHeader infoHeader;
    if (fread(&infoHeader, sizeof(infoHeader), 1, file) != 1) {
        LOGE("Failed to read BMP info header");
        fclose(file);
        return false;
    }
    
    m_width = infoHeader.width;
    m_height = std::abs(infoHeader.height);
    bool bottomUp = infoHeader.height > 0;
    
    LOGD("BMP: %dx%d, %d bpp, compression=%d",
         m_width, m_height, infoHeader.bitsPerPixel, infoHeader.compression);
    
    if (infoHeader.bitsPerPixel != 24 && infoHeader.bitsPerPixel != 32) {
        LOGE("Unsupported BMP bit depth: %d", infoHeader.bitsPerPixel);
        fclose(file);
        return false;
    }
    
    if (infoHeader.compression != 0) {
        LOGE("Compressed BMP not supported");
        fclose(file);
        return false;
    }
    
    // Seek to pixel data
    fseek(file, fileHeader.dataOffset, SEEK_SET);
    
    // Calculate row stride (BMP rows are padded to 4-byte boundaries)
    int bytesPerPixel = infoHeader.bitsPerPixel / 8;
    int rowSize = ((m_width * bytesPerPixel + 3) / 4) * 4;
    
    // Allocate and read pixel data
    std::vector<uint8_t> rowBuffer(rowSize);
    m_imageData.resize(m_width * m_height * 3);  // Store as RGB
    m_frameFormat = 3;  // RGB
    
    for (int y = 0; y < m_height; y++) {
        if (fread(rowBuffer.data(), 1, rowSize, file) != (size_t)rowSize) {
            LOGE("Failed to read BMP row %d", y);
            fclose(file);
            return false;
        }
        
        // Determine output row (flip if bottom-up)
        int outY = bottomUp ? (m_height - 1 - y) : y;
        uint8_t* outRow = m_imageData.data() + outY * m_width * 3;
        
        for (int x = 0; x < m_width; x++) {
            int inIdx = x * bytesPerPixel;
            int outIdx = x * 3;
            
            // BMP is BGR, convert to RGB
            outRow[outIdx] = rowBuffer[inIdx + 2];     // R
            outRow[outIdx + 1] = rowBuffer[inIdx + 1]; // G
            outRow[outIdx + 2] = rowBuffer[inIdx];     // B
        }
    }
    
    fclose(file);
    
    m_isVideo = false;
    m_ready = true;
    
    LOGI("BMP loaded successfully: %dx%d", m_width, m_height);
    return true;
}

bool MediaReader::loadImage(const std::string& path) {
    // For JPG/PNG, we would need additional libraries like libjpeg or libpng
    // For simplicity, only BMP is fully supported
    // This is a placeholder for future extension
    
    LOGE("JPG/PNG loading not implemented: %s", path.c_str());
    LOGE("Please convert your image to BMP format");
    return false;
}

bool MediaReader::getNextFrame(FrameData& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_ready) {
        return false;
    }
    
    if (m_isVideo) {
        // Decode next video frame
        if (!decodeVideoFrame()) {
            return false;
        }
        
        // Return the decoded frame
        frame.width = m_width;
        frame.height = m_height;
        frame.format = 1;  // YUV420 from decoder
        frame.stride = m_width;
        frame.size = m_frameBuffer.size();
        frame.data = new uint8_t[frame.size];
        memcpy(frame.data, m_frameBuffer.data(), frame.size);
        frame.timestamp = m_currentPosition;
        
        return true;
    }
    
    // For image, return the same frame
    return getPhotoFrame(frame);
}

bool MediaReader::getPhotoFrame(FrameData& frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_ready || m_imageData.empty()) {
        return false;
    }
    
    frame.width = m_width;
    frame.height = m_height;
    frame.format = m_frameFormat;
    frame.stride = m_width * 3;
    frame.size = m_imageData.size();
    frame.data = new uint8_t[frame.size];
    memcpy(frame.data, m_imageData.data(), frame.size);
    frame.timestamp = 0;
    
    return true;
}

bool MediaReader::seek(int64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_ready || !m_isVideo || !m_mediaExtractor) {
        return false;
    }
    
    AMediaExtractor* extractor = (AMediaExtractor*)m_mediaExtractor;
    AMediaExtractor_seekTo(extractor, timestampUs, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC);
    m_currentPosition = timestampUs;
    
    return true;
}

void MediaReader::reset() {
    seek(0);
}
