#pragma once

#include "GeometryTypes.h"
#include "renderer/dx12/resources/Buffer.h"
#include <vector>
#include <algorithm>

class GeometryManager {
private:
    // Allocator
    uint32_t GeometryManager::FindVertexSpace(uint32_t vertexCount, uint32_t vertexStride);
    uint32_t GeometryManager::FindIndexSpace(uint32_t indexCount);

    bool UploadIndexData(const void* data, size_t dataSize, size_t destOffset);
    bool UploadVertexData(const void* data, size_t dataSize, size_t destOffset);

    const size_t m_VERTEX_BUFFER_SIZE = 64 * 1024 * 1024; // 64 MB
    const size_t m_INDEX_BUFFER_SIZE = 16 * 1024 * 1024;  // 16 MB
    const size_t m_UPLOAD_BUFFER_SIZE = 4 * 1024 * 1024;  // 4 MB
    D3D12MA::Allocator* m_allocator;

    // Buffers
    std::vector<GeometryDescription> m_geometryDescriptions;
    std::unique_ptr<Buffer> m_vertexBuffer;
    std::unique_ptr<Buffer> m_indexBuffer;
    std::unique_ptr<Buffer> m_uploadBuffer;

public:
    GeometryManager(D3D12MA::Allocator* allocator);

    void BindBuffers(CommandList* cmdList);
    GeometryHandle GenerateHandle();
    GeometryHandle RegisterGeometry(const GeometryData& data);
    const GeometryDescription& GetGeometryDesc(GeometryHandle handle) const;
};
