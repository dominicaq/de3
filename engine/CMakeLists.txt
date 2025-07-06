cmake_minimum_required(VERSION 3.10)

# Project name
set(PROJECT_NAME "de3")
project(${PROJECT_NAME})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# External dependencies
add_library(D3D12MemoryAllocator STATIC
    "../external/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
)

target_include_directories(D3D12MemoryAllocator PUBLIC
    "../external/D3D12MemoryAllocator/include"
)

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

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    interface
    "../external/D3D12MemoryAllocator/include"
    "../external/entt-3.15.0/single_include"
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    user32
    gdi32
    kernel32
    dwmapi
    d3d12
    dxgi
    d3dcompiler
    dxguid
    D3D12MemoryAllocator
)

# Compiler definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
