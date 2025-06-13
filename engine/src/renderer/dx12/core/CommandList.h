#pragma once

#include "DX12Common.h"
#include "CommandAllocator.h"

class CommandList {
public:
    CommandList() = default;
    ~CommandList() = default;

    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    CommandList(CommandList&&) = default;
    CommandList& operator=(CommandList&&) = default;

    // Initialization and lifecycle
    bool Initialize(ID3D12Device* device, CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
    bool Reset(CommandAllocator* allocator, ID3D12PipelineState* initialState = nullptr);
    bool Close();

    // Rendering operations
    void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float clearColor[4]);
    void TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

    // State queries
    bool IsReady() const { return m_commandList != nullptr; }
    bool IsRecording() const { return m_isRecording; }

    // Accessors
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
    D3D12_COMMAND_LIST_TYPE GetType() const { return m_type; }

private:
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    bool m_isRecording = false;
};
