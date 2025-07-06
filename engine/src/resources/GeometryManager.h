#pragma once

#include <d3d12.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>

#include "GeometryTypes.h"
#include "LinearAllocator.h"
#include "renderer/dx12/resources/Buffer.h"
#include "renderer/dx12/core/CommandList.h"
#include "D3D12MemAlloc.h"

// =============================================================================
// Geometry Manager (Microsoft Engine Sample Style)
// =============================================================================

class GeometryManager {
public:
    // Constructor/Destructor
    explicit GeometryManager(D3D12MA::Allocator* allocator);
    ~GeometryManager();

    // Prevent copying
    GeometryManager(const GeometryManager&) = delete;
    GeometryManager& operator=(const GeometryManager&) = delete;

    // Create a mesh from description (returns immediately with handle)
    MeshHandle CreateMesh(const MeshDescription& desc);

    // Destroy a mesh (cleanup happens automatically)
    void DestroyMesh(MeshHandle handle);

    // Check if mesh is ready for rendering
    bool IsMeshReady(MeshHandle handle) const;

    // Get render data for a mesh (returns nullptr if not ready)
    const MeshRenderData* GetMeshRenderData(MeshHandle handle) const;

    // Frame management - call once per frame
    void BeginFrame(uint32_t frameIndex, CommandList* uploadCmdList);

    // Check if there are pending uploads that need processing
    bool HasPendingUploads() const { return !m_uploadQueue.empty(); }

    // Bind vertex/index buffers for rendering
    void BindVertexIndexBuffers(CommandList* cmdList);

    // =============================================================================
    // Statistics and Debug
    // =============================================================================

    struct Statistics {
        uint32_t totalMeshes = 0;
        uint32_t readyMeshes = 0;
        uint32_t pendingUploads = 0;
        uint32_t pendingDeletions = 0;
        size_t vertexBufferUsage = 0;
        size_t indexBufferUsage = 0;
        size_t uploadHeapUsage = 0;
    };

    Statistics GetStatistics() const;
    void PrintDebugInfo() const;

    // =============================================================================
    // Configuration
    // =============================================================================

    struct Config {
        size_t vertexBufferSize = 256 * 1024 * 1024; // 256MB
        size_t indexBufferSize = 64 * 1024 * 1024;   // 64MB
        size_t uploadHeapSize = 16 * 1024 * 1024;    // 16MB
        size_t maxUploadsPerFrame = 16;
        uint32_t maintenanceFrameInterval = 60;      // Frames between maintenance
    };

    void SetConfig(const Config& config);
    const Config& GetConfig() const { return m_config; }

private:
    // =============================================================================
    // Internal Types
    // =============================================================================

    struct MeshEntry {
        MeshHandle handle = INVALID_MESH_HANDLE;
        std::string name;
        MeshState state = MeshState::PendingUpload;
        uint32_t uploadFrameIndex = 0;

        // GPU buffer positions
        uint32_t vertexOffset = 0;
        uint32_t vertexCount = 0;
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;

        // Temporary data (cleared after upload)
        std::vector<uint8_t> vertexData;
        std::vector<uint8_t> indexData;

        // Render data (populated after upload)
        MeshRenderData renderData;
    };

    // =============================================================================
    // Internal Methods
    // =============================================================================

    bool Initialize();
    void ProcessUploadQueue();
    bool ProcessSingleUpload(MeshHandle handle);
    bool UploadData(const void* data, size_t dataSize, size_t destOffset,
                   size_t uploadOffset, bool isVertexData);
    void PerformMaintenance();
    void FlushUploads();

    // =============================================================================
    // Member Variables
    // =============================================================================

    // Configuration
    Config m_config;

    // D3D12 resources
    D3D12MA::Allocator* m_allocator = nullptr;
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_indexBuffer;
    std::unique_ptr<Buffer> m_uploadHeap;

    // Memory allocators
    std::unique_ptr<LinearAllocator> m_vertexAllocator;
    std::unique_ptr<LinearAllocator> m_indexAllocator;
    std::unique_ptr<LinearAllocator> m_uploadAllocator;

    // Mesh management
    std::unordered_map<MeshHandle, MeshEntry> m_meshRegistry;
    std::vector<MeshHandle> m_uploadQueue;
    MeshHandle m_nextMeshId = 1;

    // Frame management
    uint32_t m_frameIndex = 0;
    CommandList* m_currentUploadCmdList = nullptr;

    // State tracking
    bool m_isInitialized = false;
};
