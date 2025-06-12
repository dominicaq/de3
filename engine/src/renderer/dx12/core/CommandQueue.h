#pragma once

#include "DX12Common.h"

class CommandQueue {
public:
    CommandQueue() = default;
    ~CommandQueue();

    // Non-copyable
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

    // Movable
    CommandQueue(CommandQueue&&) = default;
    CommandQueue& operator=(CommandQueue&&) = default;

    bool Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    void ExecuteCommandLists(uint32_t numCommandLists, ID3D12CommandList* const* commandLists);

    // Signal a fence value and return it
    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue) const;
    void WaitForFenceValue(uint64_t fenceValue);

    // Wait for all GPU work to complete
    void Flush();

    // Getters
    ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue.Get(); }
    uint64_t GetCurrentFenceValue() const { return m_fenceValue; }
    uint64_t GetCompletedFenceValue() const { return m_fence->GetCompletedValue(); }

private:
    // D3D12 objects
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12Fence> m_fence;

    // Synchronization
    uint64_t m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    // Queue type for debugging/validation
    D3D12_COMMAND_LIST_TYPE m_queueType = D3D12_COMMAND_LIST_TYPE_DIRECT;
};
