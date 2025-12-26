cmake_minimum_required(VERSION 3.18.1)
project("vexgame")

# Add shared code directory
set(SHARED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../shared)

# Source files
add_library(vexgame SHARED
    vex_game_jni.c
    ${SHARED_DIR}/vex.c
)

# Include directories
target_include_directories(vexgame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHARED_DIR}
)

# Link libraries
target_link_libraries(vexgame
    android
    log
    m
)


