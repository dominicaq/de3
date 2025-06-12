#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include "CommandAllocator.h"

using Microsoft::WRL::ComPtr;

class CommandList {
public:
    CommandList() = default;
    ~CommandList() = default;

    // Non-copyable
    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    // Movable
    CommandList(CommandList&&) = default;
    CommandList& operator=(CommandList&&) = default;

    // Initialize the command list with an allocator
    bool Initialize(ID3D12Device* device, CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Reset the command list with an allocator (starts recording)
    bool Reset(CommandAllocator* allocator, ID3D12PipelineState* initialState = nullptr);

    // Close the command list (stops recording)
    bool Close();

    // Clear a render target
    void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float clearColor[4]);

    // Resource transitions
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    // Check if command list is ready
    bool IsReady() const { return m_commandList != nullptr; }

    // Check if command list is recording
    bool IsRecording() const { return m_isRecording; }

    // Get the underlying D3D12 command list
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    // Get command list type
    D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

private:
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    bool m_isRecording = false;
};
