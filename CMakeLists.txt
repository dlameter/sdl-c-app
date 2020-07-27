cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#Set project name
project(SDL_Weather_App VERSION 1.0)

configure_file(AppConfig.h.in AppConfig.h)

add_executable(${PROJECT_NAME} main.c)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

pkg_check_modules(JSON-c REQUIRED IMPORTED_TARGET json-c)
pkg_check_modules(LIBCURL REQUIRED IMPORTED_TARGET libcurl)

target_link_libraries(${PROJECT_NAME} 
    SDL2::Main
    SDL2::Image
    SDL2::TTF
    SDL2::Mixer
    PkgConfig::JSON-c
    PkgConfig::LIBCURL
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}"
    )
