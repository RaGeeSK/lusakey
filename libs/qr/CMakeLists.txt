# lusakey-qr: thin wrapper over zxing-cpp for decoding a QR code (an
# otpauth:// URI, for this app) from a raw grayscale image buffer. Only built
# when the GUI app is (LUSAKEY_BUILD_APP) — the app is the only consumer, via
# its "import TOTP QR from image file" flow; nmhost has no reason to link it.

find_package(ZXing CONFIG REQUIRED)

add_library(lusakey-qr STATIC
    src/qr_decoder.cpp
)

target_include_directories(lusakey-qr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(lusakey-qr PUBLIC ZXing::ZXing)
target_compile_features(lusakey-qr PUBLIC cxx_std_20)
