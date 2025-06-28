#include "GeometryManager.h"

GeometryManager::GeometryManager(D3D12MA::Allocator* allocator) : m_allocator(allocator) {
    // Create vertex buffer (static)
    m_vertexBuffer = std::make_unique<Buffer>();
    if (!m_vertexBuffer->Initialize(m_allocator, m_VERTEX_BUFFER_SIZE, sizeof(VertexAttributes), true)) {
        throw std::runtime_error("Failed to create triangle vertex buffer");
    }

    // Create index buffer (static)
    m_indexBuffer = std::make_unique<Buffer>();
    if (!m_indexBuffer->Initialize(m_allocator, m_INDEX_BUFFER_SIZE, sizeof(uint32_t), true)) {
        throw std::runtime_error("Failed to create triangle index buffer");
    }

    // Create upload buffer (dynamic)
    m_uploadBuffer = std::make_unique<Buffer>();
    if (!m_uploadBuffer->Initialize(m_allocator, m_UPLOAD_BUFFER_SIZE, 1, true)) {
        throw std::runtime_error("Failed to create upload buffer");
    }
}

GeometryHandle GeometryManager::GenerateHandle() {
    static GeometryHandle nextHandle = 1;
    return ++nextHandle;
}

void GeometryManager::BindBuffers(CommandList* cmdList) {
    ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();

    // Bind vertex buffer
    D3D12_VERTEX_BUFFER_VIEW vertexView = m_vertexBuffer->GetVertexView();
    d3dCmdList->IASetVertexBuffers(0, 1, &vertexView);

    // Bind index buffer
    D3D12_INDEX_BUFFER_VIEW indexView = m_indexBuffer->GetIndexView(true); // 32-bit indices
    d3dCmdList->IASetIndexBuffer(&indexView);
}

GeometryHandle GeometryManager::RegisterGeometry(const GeometryData& data) {
    // Calculate required space
    size_t vertexDataSize = data.vertexCount * data.vertexStride;
    size_t indexDataSize = data.indexCount * sizeof(uint32_t);

    // Check if data fits in upload buffer
    size_t totalUploadSize = vertexDataSize + indexDataSize;
    if (totalUploadSize > m_UPLOAD_BUFFER_SIZE) {
        printf("GeometryManager: Geometry too large for upload buffer (%zu bytes)\n", totalUploadSize);
        return INVALID_GEOMETRY_HANDLE;
    }

    // Find space in vertex buffer
    uint32_t vertexOffset = FindVertexSpace(data.vertexCount, data.vertexStride);
    if (vertexOffset == UINT32_MAX) {
        printf("GeometryManager: No space in vertex buffer for %u vertices\n", data.vertexCount);
        return INVALID_GEOMETRY_HANDLE;
    }

    // Find space in index buffer
    uint32_t indexOffset = FindIndexSpace(data.indexCount);
    if (indexOffset == UINT32_MAX) {
        printf("GeometryManager: No space in index buffer for %u indices\n", data.indexCount);
        return INVALID_GEOMETRY_HANDLE;
    }

    // TODO: Reuse discarded handles
    // Generate new handle
    GeometryHandle handle = GenerateHandle();

    // Upload vertex data
    if (!UploadVertexData(data.vertexData, vertexDataSize, vertexOffset * data.vertexStride)) {
        printf("GeometryManager: Failed to upload vertex data\n");
        return INVALID_GEOMETRY_HANDLE;
    }

    // Upload index data
    if (!UploadIndexData(data.indexData, indexDataSize, indexOffset * sizeof(uint32_t))) {
        printf("GeometryManager: Failed to upload index data\n");
        return INVALID_GEOMETRY_HANDLE;
    }

    // Create geometry description
    GeometryDescription desc;
    desc.handle = handle;
    desc.vertexOffset = vertexOffset;
    desc.vertexCount = data.vertexCount;
    desc.indexOffset = indexOffset;
    desc.indexCount = data.indexCount;
    desc.vertexStride = data.vertexStride;
    desc.isValid = true;
    desc.name = data.name;

    // Store description
    m_geometryDescriptions.push_back(desc);

    printf("GeometryManager: Registered geometry '%s' - Handle: %u, Vertices: %u, Indices: %u\n",
           data.name ? data.name : "unnamed", handle, data.vertexCount, data.indexCount);

    return handle;
}

uint32_t GeometryManager::FindVertexSpace(uint32_t vertexCount, uint32_t vertexStride) {
    // Simple linear allocation for now
    // TODO: Implement proper free space tracking

    uint32_t totalVertexBytes = 0;
    for (const auto& desc : m_geometryDescriptions) {
        if (desc.isValid) {
            uint32_t endOffset = (desc.vertexOffset + desc.vertexCount) * desc.vertexStride;
            totalVertexBytes = std::max(totalVertexBytes, endOffset);
        }
    }

    uint32_t requiredBytes = vertexCount * vertexStride;
    if (totalVertexBytes + requiredBytes > m_VERTEX_BUFFER_SIZE) {
        return UINT32_MAX; // No space
    }

    // Return offset in vertices (not bytes)
    return totalVertexBytes / vertexStride;
}

uint32_t GeometryManager::FindIndexSpace(uint32_t indexCount) {
    // Simple linear allocation for now
    // TODO: Implement proper free space tracking

    uint32_t totalIndices = 0;
    for (const auto& desc : m_geometryDescriptions) {
        if (desc.isValid) {
            totalIndices = std::max(totalIndices, desc.indexOffset + desc.indexCount);
        }
    }

    if (totalIndices + indexCount > m_INDEX_BUFFER_SIZE / sizeof(uint32_t)) {
        return UINT32_MAX; // No space
    }

    return totalIndices;
}

bool GeometryManager::UploadVertexData(const void* data, size_t dataSize, size_t destOffset) {
    // Copy to upload buffer
    if (!m_vertexBuffer->Update(data, dataSize, 0)) {
        return false;
    }

    // TODO: Create command list and copy from upload buffer to vertex buffer
    // This will need the same pattern as your UploadStaticBuffer function
    // but copying to m_vertexBuffer at destOffset

    return true; // Placeholder
}

bool GeometryManager::UploadIndexData(const void* data, size_t dataSize, size_t destOffset) {
    // Copy to upload buffer
    if (!m_indexBuffer->Update(data, dataSize, 0)) {
        return false;
    }

    // TODO: Create command list and copy from upload buffer to index buffer
    // This will need the same pattern as your UploadStaticBuffer function
    // but copying to m_indexBuffer at destOffset

    return true; // Placeholder
}

const GeometryDescription& GeometryManager::GetGeometryDesc(GeometryHandle handle) const {
    for (const auto& desc : m_geometryDescriptions) {
        if (desc.handle == handle && desc.isValid) {
            return desc;
        }
    }

    // Handle not found - return invalid description
    static GeometryDescription invalidDesc;
    invalidDesc.isValid = false;
    invalidDesc.handle = INVALID_GEOMETRY_HANDLE;
    return invalidDesc;
}

// bool Renderer::UploadStaticBuffer(Buffer* buffer, const void* data, size_t dataSize) {
//     if (dataSize > m_uploadBuffer->GetSize()) {
//         printf("UploadStaticBuffer: Data too large for upload buffer\n");
//         return false;
//     }

//     // Put data into the upload buffer
//     if (!m_uploadBuffer->Update(data, dataSize, 0)) {
//         return false;
//     }

//     ID3D12GraphicsCommandList* cmdList = m_commandList->GetCommandList();

//     // Transition static buffer to copy destination
//     D3D12_RESOURCE_BARRIER barrier = {};
//     barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//     barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//     barrier.Transition.pResource = buffer->GetResource();
//     barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
//     barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
//     barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

//     cmdList->ResourceBarrier(1, &barrier);

//     // Copy from upload buffer to static buffer
//     cmdList->CopyBufferRegion(
//         buffer->GetResource(),           // Destination
//         0,                              // Dest offset
//         m_uploadBuffer->GetResource(),   // Source
//         0,                              // Source offset
//         dataSize                        // Size
//     );

//     // Transition to vertex buffer state
//     barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
//     barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
//     cmdList->ResourceBarrier(1, &barrier);

//     // Close the command list
//     if (!m_commandList->Close()) {
//         printf("Failed to close command list for upload\n");
//         return false;
//     }

//     // Execute immediately
//     ID3D12CommandList* commandLists[] = { cmdList };
//     m_commandManager->GetGraphicsQueue()->ExecuteCommandLists(1, commandLists);

//     // Wait for completion using existing fence
//     UINT64 uploadFenceValue = 999999;
//     HRESULT hr = m_commandManager->GetGraphicsQueue()->GetCommandQueue()->Signal(
//         m_frameResources[0].frameFence.Get(), uploadFenceValue);

//     if (FAILED(hr)) {
//         printf("Failed to signal fence for upload: 0x%08X\n", hr);
//         return false;
//     }

//     // Wait for upload to complete
//     while (m_frameResources[0].frameFence->GetCompletedValue() < uploadFenceValue) {
//         Sleep(1);
//     }

//     return true;
// }
