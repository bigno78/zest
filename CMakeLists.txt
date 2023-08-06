cmake_minimum_required(VERSION 3.15)
project(editor)

set(RAYLIB_PATH "" CACHE PATH "Path to the raylib build.")
set(TREE_SITTER_PATH "" CACHE PATH "Path to the raylib build.")
set(TREE_SITTER_CPP_PATH "" CACHE PATH "Path to the raylib build.")

if(NOT RAYLIB_PATH)
    message(FATAL_ERROR "RAYLIB_PATH must be set")
elseif(NOT TREE_SITTER_PATH)
    message(FATAL_ERROR "TREE_SITTER_PATH must be set")
elseif(NOT TREE_SITTER_CPP_PATH)
    message(TREE_SITTER_CPP_PATH "RAYLIB_PATH must be set")
endif()

add_library(raylib SHARED IMPORTED)
set_target_properties(
    raylib PROPERTIES
    IMPORTED_LOCATION ${RAYLIB_PATH}/lib/raylib.dll
    IMPORTED_IMPLIB ${RAYLIB_PATH}/lib/raylib.lib
    INTERFACE_INCLUDE_DIRECTORIES ${RAYLIB_PATH}/include/
)
target_link_libraries(raylib INTERFACE winmm)


add_library(ts STATIC ${TREE_SITTER_PATH}/lib/src/lib.c)
target_include_directories(ts PRIVATE ${TREE_SITTER_PATH}/lib/src/)
target_include_directories(ts PUBLIC ${TREE_SITTER_PATH}/lib/include/)

add_library(ts_cpp STATIC
    ${TREE_SITTER_CPP_PATH}/src/parser.c
    ${TREE_SITTER_CPP_PATH}/src/scanner.c
)
target_link_libraries(ts_cpp PRIVATE ts)


add_executable(${PROJECT_NAME} src/main.cpp src/tree_sitter.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE src/)
target_link_libraries(${PROJECT_NAME} raylib ts ts_cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# Do not open console on windows
# target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
