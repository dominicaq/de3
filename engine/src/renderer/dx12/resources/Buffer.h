#pragma once

#include "../core/DX12Common.h"
#include "../core/CommandList.h"
#include <D3D12MemAlloc.h>
#include <memory>

class Buffer {
public:
    Buffer() = default;
    ~Buffer();

    bool Initialize(D3D12MA::Allocator* allocator, UINT64 size, UINT stride, bool dynamic = false);

    // Core interface
    ID3D12Resource* GetResource() const { return m_buffer.Get(); }
    UINT64 GetSize() const { return m_size; }
    UINT GetStride() const { return m_stride; }
    UINT GetCount() const { return static_cast<UINT>(m_size / m_stride); }

    // Update for dynamic buffers
    bool Update(const void* data, UINT64 size, UINT64 offset = 0);

    // View creation
    D3D12_VERTEX_BUFFER_VIEW GetVertexView() const;
    D3D12_INDEX_BUFFER_VIEW GetIndexView(bool is32Bit = true) const;

private:
    ComPtr<ID3D12Resource> m_buffer;
    D3D12MA::Allocation* m_allocation = nullptr;

    UINT64 m_size = 0;
    UINT m_stride = 0;
    bool m_isDynamic = false;
    void* m_mappedData = nullptr;
};
