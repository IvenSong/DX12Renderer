#include "image.h"

#include <limits>
#include <memory>
#include <stdexcept>

#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

namespace {
    image takeDecodedPixels(stbi_uc* pixels, int width, int height, const std::string& sourceName)
    {
        // Free stb's allocation even if validation or the vector allocation throws.
        const std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decoded(pixels, stbi_image_free);
        if (!decoded) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Failed to load image '" + sourceName + "': "
                + (reason ? reason : "unknown decoding error"));
        }
        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Invalid image dimensions: " + sourceName);
        }

        const size_t maxSize = (std::numeric_limits<size_t>::max)();
        if (static_cast<size_t>(width) > maxSize / 4
            || static_cast<size_t>(height) > maxSize / (static_cast<size_t>(width) * 4)) {
            throw std::runtime_error("Image is too large: " + sourceName);
        }

        image result;
        result.mWidth = static_cast<uint32_t>(width);
        result.mHeight = static_cast<uint32_t>(height);
        const size_t byteCount = result.rowPitch() * result.mHeight;
        result.mData.assign(decoded.get(), decoded.get() + byteCount);
        return result;
    }
}

image image::createImage(const std::string& path)
{
    // Keep stb's native top-left origin, matching this project's cube UVs.
    stbi_set_flip_vertically_on_load_thread(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    // stb already returns RGBA: do not apply SoftRenderer's GDI red/blue swap.
    return takeDecodedPixels(pixels, width, height, path);
}

image image::createImageFromMemory(const uint8_t* encodedData, size_t byteCount,
    const std::string& sourceName)
{
    if (!encodedData || byteCount == 0
        || byteCount > static_cast<size_t>((std::numeric_limits<int>::max)())) {
        throw std::invalid_argument("Invalid encoded image buffer: " + sourceName);
    }

    stbi_set_flip_vertically_on_load_thread(false);
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(encodedData, static_cast<int>(byteCount),
        &width, &height, &channels, STBI_rgb_alpha);
    return takeDecodedPixels(pixels, width, height, sourceName);
}
