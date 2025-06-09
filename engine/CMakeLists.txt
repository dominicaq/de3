cmake_minimum_required(VERSION 3.10)

# Project name
set(PROJECT_NAME "de3")
project(${PROJECT_NAME})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.c"
)

# Collect all header files from src directory
file(GLOB_RECURSE HEADERS
    "src/*.h"
    "src/*.hpp"
)

# Add executable with all source files
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# Include src directory for headers
target_include_directories(${PROJECT_NAME} PRIVATE src)

# Link Windows libraries + DirectX 12 libraries
target_link_libraries(${PROJECT_NAME}
    user32
    gdi32
    kernel32
    dwmapi
    d3d12          # DirectX 12
    dxgi           # DXGI (swap chain, factory, adapters)
    d3dcompiler    # HLSL shader compilation
    dxguid         # DirectX GUIDs
)

# Add compiler definitions for Windows
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
