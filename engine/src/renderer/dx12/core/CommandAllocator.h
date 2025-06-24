#pragma once

#include "DX12Common.h"

class CommandAllocator {
public:
    CommandAllocator() = default;
    ~CommandAllocator() = default;

    // Non-copyable
    CommandAllocator(const CommandAllocator&) = delete;
    CommandAllocator& operator=(const CommandAllocator&) = delete;

    // Life Cycle Management
    bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    bool Reset();
    bool IsReady() const { return m_allocator != nullptr; }

    // Getters
    ID3D12CommandAllocator* GetAllocator() const { return m_allocator.Get(); }
    D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

private:
    ComPtr<ID3D12CommandAllocator> m_allocator;
    D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
};
