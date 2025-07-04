#pragma once
#include <cstdint>

// =============================================================================
// Constants
// =============================================================================

constexpr uint32_t VERTEX_SIZE = 6;  // 6 floats per vertex

// Handle type for referencing geometry
using GeometryHandle = uint32_t;
constexpr GeometryHandle INVALID_GEOMETRY_HANDLE = 0;

// Handle type for referencing meshes (Microsoft style)
using MeshHandle = uint32_t;
constexpr MeshHandle INVALID_MESH_HANDLE = 0;

// =============================================================================
// Vertex Formats
// =============================================================================

// Simple vertex formats
struct VertexAttributes {
    float position[3];
    float color[3];
};

// Verify the struct size matches expectations
static_assert(sizeof(VertexAttributes) == 6 * sizeof(float), "VertexAttributes size mismatch");

// =============================================================================
// Microsoft-Style Mesh System
// =============================================================================

// Mesh description for creation (Microsoft engine sample style)
struct MeshDescription {
    const char* name = nullptr;
    const VertexAttributes* vertices = nullptr;
    const uint32_t* indices = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

// Render data for drawing (contains GPU buffer offsets)
struct MeshRenderData {
    uint32_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
};

// Mesh state enumeration
enum class MeshState {
    PendingUpload,    // Mesh created, waiting for GPU upload
    Ready,            // Mesh uploaded and ready for rendering
    PendingDeletion   // Mesh marked for deletion
};
