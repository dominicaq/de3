#pragma once

#include <cstdint>

// =============================================================================
// Constants
// =============================================================================

using MeshHandle = uint32_t;
constexpr MeshHandle INVALID_MESH_HANDLE = 0;

using ShaderHandle = uint32_t;
constexpr ShaderHandle INVALID_SHADER_HANDLE = 0;

using TextureHandle = uint32_t;
constexpr TextureHandle INVALID_TEXTURE_HANDLE = 0;

using MaterialHandle = uint32_t;
constexpr MaterialHandle INVALID_MATERIAL_HANDLE = 0;

constexpr uint32_t VERTEX_SIZE = 6;  // 6 floats per vertex

// =============================================================================
// Vertex Formats
// =============================================================================

struct VertexAttributes {
    float position[3];
    float color[3];
};

// Verify the struct size matches expectations
static_assert(sizeof(VertexAttributes) == 6 * sizeof(float), "VertexAttributes size mismatch");

// =============================================================================
// Mesh System
// =============================================================================

struct CPUMesh {
    const VertexAttributes* vertices = nullptr;
    const uint32_t* indices = nullptr;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

struct MeshView {
    uint32_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
};

enum class MeshState {
    PendingUpload,
    Ready,
    PendingDeletion
};

// =============================================================================
// Material System
// =============================================================================

struct Material {
    ShaderHandle shader = INVALID_SHADER_HANDLE;
    TextureHandle albedo = INVALID_TEXTURE_HANDLE;
    TextureHandle normal = INVALID_TEXTURE_HANDLE;
    TextureHandle mrao = INVALID_TEXTURE_HANDLE;
    TextureHandle emissive = INVALID_TEXTURE_HANDLE;
    TextureHandle height = INVALID_TEXTURE_HANDLE;
};
