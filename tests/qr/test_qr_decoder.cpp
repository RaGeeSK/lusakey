#include <catch2/catch_test_macros.hpp>

#include "lusakey/qr/qr_decoder.h"

using lusakey::core::qr::decode;
using lusakey::core::qr::GrayscaleImage;

// libs/qr had zero test coverage before this file (see AGENTS.md) — these
// cases exercise decode()'s documented error/empty-result paths without
// needing a real QR-code image fixture or an encoder dependency. A true
// encode->decode round-trip is deferred (would need either a writer feature
// on the vendored zxing-cpp port, currently built with default-features
// off, or a committed test-fixture image) — verified manually instead, via
// AGENTS.md's screenshot-based UI smoke test recipe.

TEST_CASE("qr::decode throws on an empty image", "[qr]") {
    GrayscaleImage image;
    REQUIRE_THROWS_AS(decode(image), std::runtime_error);
}

TEST_CASE("qr::decode throws when width/height are non-positive", "[qr]") {
    GrayscaleImage image;
    image.pixels = {0, 0, 0, 0};
    image.width = 0;
    image.height = 4;
    REQUIRE_THROWS_AS(decode(image), std::runtime_error);
}

TEST_CASE("qr::decode throws when the pixel buffer size doesn't match width*height", "[qr]") {
    GrayscaleImage image;
    image.width = 4;
    image.height = 4;
    image.pixels.resize(4); // should be 16
    REQUIRE_THROWS_AS(decode(image), std::runtime_error);
}

TEST_CASE("qr::decode returns nullopt for a well-formed image with no QR code in it", "[qr]") {
    GrayscaleImage image;
    image.width = 64;
    image.height = 64;
    image.pixels.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height), 255);
    REQUIRE_FALSE(decode(image).has_value());
}
