#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Adapted from SoftRenderer: owned, tightly packed RGBA8 pixels with a top-left origin.
// This loader has no dependency on the software renderer's math or BGRA framebuffer.
class image {
public:
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    std::vector<uint8_t> mData;

    size_t rowPitch() const noexcept { return static_cast<size_t>(mWidth) * 4; }

    // Paths use UTF-8. Loading errors throw std::runtime_error with the source name.
    static image createImage(const std::string& path);

    // The input contains an encoded image (e.g. PNG/JPEG), and byteCount is its file size.
    static image createImageFromMemory(const uint8_t* encodedData, size_t byteCount,
        const std::string& sourceName = "<memory>");
};
