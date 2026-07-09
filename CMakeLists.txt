cmake_minimum_required(VERSION 3.25)
project(lusakey LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LUSAKEY_BUILD_TESTS "Build unit tests" ON)
option(LUSAKEY_BUILD_APP "Build the Qt GUI application" OFF)
option(LUSAKEY_BUILD_NMHOST "Build the native-messaging host stub" OFF)

add_subdirectory(libs/core)
add_subdirectory(libs/ipc)

if(LUSAKEY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/core)
    add_subdirectory(tests/ipc)
endif()

if(LUSAKEY_BUILD_APP)
    add_subdirectory(libs/qr)
    add_subdirectory(app)

    # Linux .deb packaging (CPack itself must be include()'d after the
    # install() rules it packages — app/CMakeLists.txt's install(TARGETS
    # lusakey ...) already ran by this point since add_subdirectory(app) is
    # above this block). Windows/macOS are handled by the separate
    # packaging/windows and packaging/macos scripts, not CPack.
    if(UNIX AND NOT APPLE)
        set(CPACK_PACKAGE_NAME "lusakey")
        set(CPACK_PACKAGE_VERSION "0.1.0")
        set(CPACK_PACKAGE_CONTACT "lusakey")
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "lusakey")
        set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libstdc++6")
        include(CPack)
    endif()
endif()

if(LUSAKEY_BUILD_NMHOST)
    add_subdirectory(nmhost)
endif()
