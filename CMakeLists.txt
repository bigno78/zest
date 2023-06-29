cmake_minimum_required(VERSION 3.15)
project(editor)

set(RAYLIB_PATH "" CACHE PATH "Path to the raylib build.")

add_library(raylib SHARED IMPORTED)
set_target_properties(
    raylib PROPERTIES
    IMPORTED_LOCATION ${RAYLIB_PATH}/lib/raylib.dll
    IMPORTED_IMPLIB ${RAYLIB_PATH}/lib/raylib.lib
    INTERFACE_INCLUDE_DIRECTORIES ${RAYLIB_PATH}/include/
)
target_link_libraries(raylib INTERFACE winmm)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} raylib)
