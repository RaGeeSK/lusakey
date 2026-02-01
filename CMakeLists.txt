cmake_minimum_required(VERSION 3.20)
project(lusakey LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lusakey
    src/main.cpp
    src/app.cpp
    src/ui_controls.cpp
    src/crypto.cpp
    src/vault.cpp
    src/password_gen.cpp
    src/resources.rc
)

target_include_directories(lusakey PRIVATE src)
target_compile_definitions(lusakey PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(lusakey PRIVATE
    gdiplus
    bcrypt
    comctl32
    uxtheme
    dwmapi
)
