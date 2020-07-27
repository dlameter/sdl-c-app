cmake_minimum_required(VERSION 3.10);

#Set project name
project(SDL_Weather_app VERSION 1.0)

configure_file(AppConfig.h.in AppConfig.h);

add_executable(SDL_Weather_app main.c);

target_include_directories(SDL_Weather_app PUBLIC
    "${PROJECT_BINARY_DIR}"
    )
