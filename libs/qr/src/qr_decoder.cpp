// Confirmed building as-is against vcpkg's nu-book-zxing-cpp@2.3.0 port
// (ReaderOptions/setFormats/ImageView/ReadBarcode) on Windows/MSVC — see
// AGENTS.md. Not yet verified at runtime against a real QR image, only
// compiled; if a future zxing-cpp version renames these again, the logic
// here (decode one QR code from a grayscale buffer, return its text)
// shouldn't need to change, only the exact API names.

#include "lusakey/qr/qr_decoder.h"

#include <ZXing/ReadBarcode.h>

#include <stdexcept>

namespace lusakey::core::qr {

std::optional<std::string> decode(const GrayscaleImage& image) {
    if (image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        throw std::runtime_error("qr::decode: empty or malformed image");
    }
    if (image.pixels.size() != static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height)) {
        throw std::runtime_error("qr::decode: pixel buffer size does not match width*height");
    }

    const ZXing::ImageView view(image.pixels.data(), image.width, image.height, ZXing::ImageFormat::Lum);

    ZXing::ReaderOptions options;
    options.setFormats(ZXing::BarcodeFormat::QRCode);

    const auto result = ZXing::ReadBarcode(view, options);
    if (!result.isValid()) {
        return std::nullopt;
    }
    return result.text();
}

} // namespace lusakey::core::qr
