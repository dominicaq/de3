#include "GeometryManager.h"
#include <algorithm>
#include <cstdio>

GeometryManager::GeometryManager(D3D12MA::Allocator* allocator)
    : m_allocator(allocator)
    , m_frameIndex(0)
    , m_nextMeshId(1)
{
    if (!Initialize()) {
        throw std::runtime_error("Failed to initialize GeometryManager");
    }
}

GeometryManager::~GeometryManager() {
    // Wait for any pending uploads to complete
    FlushUploads();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_uploadHeap.reset();
}

bool GeometryManager::Initialize() {
    if (!m_allocator) {
        printf("GeometryManager: Invalid allocator\n");
        return false;
    }

    // Create large static vertex buffer
    m_vertexBuffer = std::make_unique<Buffer>();
    if (!m_vertexBuffer->Initialize(m_allocator, m_config.vertexBufferSize, sizeof(VertexAttributes), false)) {
        printf("GeometryManager: Failed to create vertex buffer\n");
        return false;
    }

    // Create large static index buffer
    m_indexBuffer = std::make_unique<Buffer>();
    if (!m_indexBuffer->Initialize(m_allocator, m_config.indexBufferSize, sizeof(uint32_t), false)) {
        printf("GeometryManager: Failed to create index buffer\n");
        return false;
    }

    // Create upload heap for staging data
    m_uploadHeap = std::make_unique<Buffer>();
    if (!m_uploadHeap->Initialize(m_allocator, m_config.uploadHeapSize, 1, true)) {
        printf("GeometryManager: Failed to create upload heap\n");
        return false;
    }

    // Initialize allocators
    m_vertexAllocator = std::make_unique<LinearAllocator>(m_config.vertexBufferSize);
    m_indexAllocator = std::make_unique<LinearAllocator>(m_config.indexBufferSize);
    m_uploadAllocator = std::make_unique<LinearAllocator>(m_config.uploadHeapSize);

    // Reserve space for mesh registry
    m_meshRegistry.reserve(1024);

    m_isInitialized = true;
    printf("GeometryManager: Initialized successfully\n");
    return true;
}

void GeometryManager::SetConfig(const Config& config) {
    // Only allow config changes before initialization or when empty
    if (m_isInitialized && !m_meshRegistry.empty()) {
        printf("GeometryManager: Cannot change config while meshes are loaded\n");
        return;
    }

    m_config = config;

    if (m_isInitialized) {
        // Reinitialize with new config
        m_meshRegistry.clear();
        m_uploadQueue.clear();
        m_nextMeshId = 1;
        m_isInitialized = false;

        if (!Initialize()) {
            throw std::runtime_error("Failed to reinitialize with new config");
        }
    }
}

// =============================================================================
// Public Interface - Microsoft Engine Sample Style
// =============================================================================

MeshHandle GeometryManager::CreateMesh(const MeshDescription& desc) {
    if (!m_isInitialized) {
        printf("GeometryManager: Not initialized\n");
        return INVALID_MESH_HANDLE;
    }

    // Validate input
    if (!desc.vertices || !desc.indices || desc.vertexCount == 0 || desc.indexCount == 0) {
        printf("GeometryManager: Invalid mesh description\n");
        return INVALID_MESH_HANDLE;
    }

    // Calculate memory requirements
    size_t vertexDataSize = desc.vertexCount * sizeof(VertexAttributes);
    size_t indexDataSize = desc.indexCount * sizeof(uint32_t);
    size_t totalSize = vertexDataSize + indexDataSize;

    // Check if we have enough space
    if (totalSize > m_config.uploadHeapSize) {
        printf("GeometryManager: Mesh too large for upload heap (%zu bytes)\n", totalSize);
        return INVALID_MESH_HANDLE;
    }

    // Allocate space in GPU buffers
    uint32_t vertexOffset = m_vertexAllocator->Allocate(vertexDataSize) / sizeof(VertexAttributes);
    uint32_t indexOffset = m_indexAllocator->Allocate(indexDataSize) / sizeof(uint32_t);

    if (vertexOffset == UINT32_MAX || indexOffset == UINT32_MAX) {
        printf("GeometryManager: Failed to allocate space for mesh '%s'\n",
               desc.name ? desc.name : "unnamed");
        return INVALID_MESH_HANDLE;
    }

    // Create mesh handle
    MeshHandle handle = m_nextMeshId++;

    // Create mesh entry
    MeshEntry entry;
    entry.handle = handle;
    entry.name = desc.name ? std::string(desc.name) : "unnamed";
    entry.vertexOffset = vertexOffset;
    entry.vertexCount = desc.vertexCount;
    entry.indexOffset = indexOffset;
    entry.indexCount = desc.indexCount;
    entry.state = MeshState::PendingUpload;
    entry.uploadFrameIndex = m_frameIndex;

    // Copy vertex data
    entry.vertexData.resize(vertexDataSize);
    memcpy(entry.vertexData.data(), desc.vertices, vertexDataSize);

    // Copy index data
    entry.indexData.resize(indexDataSize);
    memcpy(entry.indexData.data(), desc.indices, indexDataSize);

    // Add to registry
    m_meshRegistry[handle] = std::move(entry);

    // Queue for upload
    m_uploadQueue.push_back(handle);

    printf("GeometryManager: Created mesh '%s' (Handle: %u, Vertices: %u, Indices: %u)\n",
           entry.name.c_str(), handle, desc.vertexCount, desc.indexCount);

    return handle;
}

void GeometryManager::DestroyMesh(MeshHandle handle) {
    auto it = m_meshRegistry.find(handle);
    if (it != m_meshRegistry.end()) {
        MeshEntry& entry = it->second;

        // Mark for deletion (actual cleanup happens during maintenance)
        entry.state = MeshState::PendingDeletion;

        printf("GeometryManager: Marked mesh '%s' for deletion\n", entry.name.c_str());
    }
}

bool GeometryManager::IsMeshReady(MeshHandle handle) const {
    auto it = m_meshRegistry.find(handle);
    return it != m_meshRegistry.end() && it->second.state == MeshState::Ready;
}

const MeshRenderData* GeometryManager::GetMeshRenderData(MeshHandle handle) const {
    auto it = m_meshRegistry.find(handle);
    if (it != m_meshRegistry.end() && it->second.state == MeshState::Ready) {
        return &it->second.renderData;
    }
    return nullptr;
}

void GeometryManager::BeginFrame(uint32_t frameIndex, CommandList* uploadCmdList) {
    m_frameIndex = frameIndex;
    m_currentUploadCmdList = uploadCmdList;

    // Process upload queue automatically
    ProcessUploadQueue();

    // Perform maintenance periodically
    if (frameIndex % m_config.maintenanceFrameInterval == 0) {
        PerformMaintenance();
    }
}

void GeometryManager::BindVertexIndexBuffers(CommandList* cmdList) {
    if (!cmdList || !m_isInitialized) {
        return;
    }

    ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();

    // Bind vertex buffer
    D3D12_VERTEX_BUFFER_VIEW vertexView = m_vertexBuffer->GetVertexView();
    d3dCmdList->IASetVertexBuffers(0, 1, &vertexView);

    // Bind index buffer
    D3D12_INDEX_BUFFER_VIEW indexView = m_indexBuffer->GetIndexView(true);
    d3dCmdList->IASetIndexBuffer(&indexView);
}

// =============================================================================
// Internal Implementation
// =============================================================================

void GeometryManager::ProcessUploadQueue() {
    if (m_uploadQueue.empty() || !m_currentUploadCmdList) {
        return;
    }

    // Process uploads in batches to avoid overwhelming the upload heap
    size_t processed = 0;

    auto it = m_uploadQueue.begin();
    while (it != m_uploadQueue.end() && processed < m_config.maxUploadsPerFrame) {
        MeshHandle handle = *it;

        if (ProcessSingleUpload(handle)) {
            it = m_uploadQueue.erase(it);
            processed++;
        } else {
            ++it;
        }
    }

    if (processed > 0) {
        printf("GeometryManager: Processed %zu mesh uploads this frame\n", processed);
    }
}

bool GeometryManager::ProcessSingleUpload(MeshHandle handle) {
    auto it = m_meshRegistry.find(handle);
    if (it == m_meshRegistry.end()) {
        return true; // Remove from queue if mesh no longer exists
    }

    MeshEntry& entry = it->second;
    if (entry.state != MeshState::PendingUpload) {
        return true; // Already processed or in wrong state
    }

    // Calculate data sizes
    size_t vertexDataSize = entry.vertexData.size();
    size_t indexDataSize = entry.indexData.size();
    size_t totalSize = vertexDataSize + indexDataSize;

    // Check if we have enough space in upload heap
    if (!m_uploadAllocator->CanAllocate(totalSize)) {
        // Not enough space right now, try next frame
        return false;
    }

    // Allocate space in upload heap
    uint32_t uploadOffset = m_uploadAllocator->Allocate(totalSize);
    if (uploadOffset == UINT32_MAX) {
        return false;
    }

    // Upload vertex data
    if (!UploadData(entry.vertexData.data(), vertexDataSize,
                   entry.vertexOffset * sizeof(VertexAttributes),
                   uploadOffset, true)) {
        printf("GeometryManager: Failed to upload vertex data for mesh '%s'\n", entry.name.c_str());
        return false;
    }

    // Upload index data
    if (!UploadData(entry.indexData.data(), indexDataSize,
                   entry.indexOffset * sizeof(uint32_t),
                   uploadOffset + vertexDataSize, false)) {
        printf("GeometryManager: Failed to upload index data for mesh '%s'\n", entry.name.c_str());
        return false;
    }

    // Update mesh state
    entry.state = MeshState::Ready;

    // Setup render data
    entry.renderData.vertexOffset = entry.vertexOffset;
    entry.renderData.vertexCount = entry.vertexCount;
    entry.renderData.indexOffset = entry.indexOffset;
    entry.renderData.indexCount = entry.indexCount;

    // Clear temporary data to save memory
    entry.vertexData.clear();
    entry.vertexData.shrink_to_fit();
    entry.indexData.clear();
    entry.indexData.shrink_to_fit();

    printf("GeometryManager: Uploaded mesh '%s' to GPU\n", entry.name.c_str());
    return true;
}

bool GeometryManager::UploadData(const void* data, size_t dataSize, size_t destOffset,
                                size_t uploadOffset, bool isVertexData) {
    // Copy data to upload heap
    if (!m_uploadHeap->Update(data, static_cast<UINT64>(dataSize), static_cast<UINT64>(uploadOffset))) {
        printf("GeometryManager: Failed to update upload heap\n");
        return false;
    }

    ID3D12GraphicsCommandList* cmdList = m_currentUploadCmdList->GetCommandList();

    // Determine target buffer and state
    Buffer* targetBuffer = isVertexData ? m_vertexBuffer.get() : m_indexBuffer.get();
    D3D12_RESOURCE_STATES targetState = isVertexData ?
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER :
        D3D12_RESOURCE_STATE_INDEX_BUFFER;

    // Transition to copy destination
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = targetBuffer->GetResource();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    // Copy data
    cmdList->CopyBufferRegion(
        targetBuffer->GetResource(),
        destOffset,
        m_uploadHeap->GetResource(),
        uploadOffset,
        dataSize
    );

    // Transition to final state
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = targetState;
    cmdList->ResourceBarrier(1, &barrier);

    return true;
}

void GeometryManager::PerformMaintenance() {
    // Clean up deleted meshes
    auto it = m_meshRegistry.begin();
    while (it != m_meshRegistry.end()) {
        if (it->second.state == MeshState::PendingDeletion) {
            printf("GeometryManager: Cleaning up mesh '%s'\n", it->second.name.c_str());

            // TODO: In a real implementation, you'd want to:
            // 1. Add freed space back to allocators (requires more sophisticated allocator)
            // 2. Defragment buffers periodically
            // 3. Handle reference counting for in-flight renders

            it = m_meshRegistry.erase(it);
        } else {
            ++it;
        }
    }

    // Reset upload allocator periodically (simple strategy)
    m_uploadAllocator->Reset();

    // printf("GeometryManager: Performed maintenance at frame %u\n", m_frameIndex);
}

void GeometryManager::FlushUploads() {
    // Process all remaining uploads
    while (!m_uploadQueue.empty() && m_currentUploadCmdList) {
        ProcessUploadQueue();
    }
}

// =============================================================================
// Statistics and Debug
// =============================================================================

GeometryManager::Statistics GeometryManager::GetStatistics() const {
    Statistics stats = {};

    stats.totalMeshes = static_cast<uint32_t>(m_meshRegistry.size());
    stats.vertexBufferUsage = m_vertexAllocator ? m_vertexAllocator->GetUsedSpace() : 0;
    stats.indexBufferUsage = m_indexAllocator ? m_indexAllocator->GetUsedSpace() : 0;
    stats.uploadHeapUsage = m_uploadAllocator ? m_uploadAllocator->GetUsedSpace() : 0;
    stats.pendingUploads = static_cast<uint32_t>(m_uploadQueue.size());

    // Count by state
    for (const auto& pair : m_meshRegistry) {
        switch (pair.second.state) {
            case MeshState::PendingUpload: break; // Already counted in pendingUploads
            case MeshState::Ready: stats.readyMeshes++; break;
            case MeshState::PendingDeletion: stats.pendingDeletions++; break;
        }
    }

    return stats;
}

void GeometryManager::PrintDebugInfo() const {
    Statistics stats = GetStatistics();

    printf("=== GeometryManager Debug Info ===\n");
    printf("Total Meshes: %u\n", stats.totalMeshes);
    printf("Ready Meshes: %u\n", stats.readyMeshes);
    printf("Pending Uploads: %u\n", stats.pendingUploads);
    printf("Pending Deletions: %u\n", stats.pendingDeletions);
    printf("Vertex Buffer Usage: %.1f MB / %.1f MB\n",
           stats.vertexBufferUsage / (1024.0f * 1024.0f),
           m_config.vertexBufferSize / (1024.0f * 1024.0f));
    printf("Index Buffer Usage: %.1f MB / %.1f MB\n",
           stats.indexBufferUsage / (1024.0f * 1024.0f),
           m_config.indexBufferSize / (1024.0f * 1024.0f));
    printf("Upload Heap Usage: %.1f MB / %.1f MB\n",
           stats.uploadHeapUsage / (1024.0f * 1024.0f),
           m_config.uploadHeapSize / (1024.0f * 1024.0f));
    printf("===================================\n");
}
