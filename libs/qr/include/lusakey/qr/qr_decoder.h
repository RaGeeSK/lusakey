#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lusakey::core::qr {

// Raw 8-bit grayscale image data, row-major, no padding — the format
// zxing-cpp's ImageView consumes directly. Deliberately NOT a QImage: this
// library has no GUI-toolkit or image-codec dependency of its own (only
// zxing-cpp), so the app layer converts whatever it loaded (a QImage, a
// decoded PNG, ...) into this before calling decode().
struct GrayscaleImage {
    std::vector<std::uint8_t> pixels; // width * height bytes
    int width = 0;
    int height = 0;
};

// Decodes the first QR code found in `image` and returns its text content
// (for this app's use case, expected to be an otpauth:// URI). Returns
// std::nullopt if no QR code is found. Throws std::runtime_error if `image`
// is empty/malformed.
std::optional<std::string> decode(const GrayscaleImage& image);

} // namespace lusakey::core::qr
