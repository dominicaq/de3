#pragma once
#include <cstdint>

// Handle type for referencing geometry
using GeometryHandle = uint32_t;
constexpr GeometryHandle INVALID_GEOMETRY_HANDLE = 0;

// Simple vertex formats
struct VertexAttributes {
    float position[3];
    float color[4];
};

// Input geometry data from the application
struct GeometryData {
    const void* vertexData;
    uint32_t vertexCount;
    uint32_t vertexStride;

    const uint32_t* indexData;
    uint32_t indexCount;

    // Optional metadata
    const char* name = nullptr;
};

// Description of registered geometry (metadata only)
struct GeometryDescription {
    GeometryHandle handle = INVALID_GEOMETRY_HANDLE;

    // Location in shared buffers
    uint32_t vertexOffset = 0;      // Offset in vertices, not bytes
    uint32_t vertexCount = 0;
    uint32_t indexOffset = 0;       // Offset in indices, not bytes
    uint32_t indexCount = 0;
    uint32_t vertexStride = 0;      // Size of each vertex in bytes

    // State
    bool isValid = false;

    // Optional metadata
    const char* name = nullptr;
};
