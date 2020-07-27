cmake_minimum_required(VERSION 3.10)

#Set project name
project(SDL_Weather_App VERSION 1.0)

configure_file(AppConfig.h.in AppConfig.h)

add_executable(${PROJECT_NAME} main.c)

include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
pkg_search_module(SDL2TTF REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS} 
    ${SDL2IMAGE_INCLUDE_DIRS}
    ${SDL2TTF_INCLUDE_DIRS}
    )
target_link_libraries(${PROJECT_NAME} 
    ${SDL2_LIBRARIES} 
    ${SDL2IMAGE_LIBRARIES}
    ${SDL2TTD_LIBRARIES}
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_BINARY_DIR}"
    )
