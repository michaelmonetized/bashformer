cmake_minimum_required(VERSION 3.18.1)
project("platformergame")

# Add shared code directory
set(SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../shared)

# Source files
add_library(platformergame SHARED
    platformer_game_jni.c
    ${SHARED_DIR}/platformer_sdl.c
)

# Include directories
target_include_directories(platformergame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_DIR}
)

# Link libraries
target_link_libraries(platformergame
    android
    log
    m
)


