#include "CommandList.h"

bool CommandList::Initialize(ID3D12Device* device, CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type) {
    if (!device || !allocator || !allocator->IsReady()) {
        return false;
    }

    m_type = type;

    // Create command list with the provided allocator
    HRESULT hr = device->CreateCommandList(0, type, allocator->GetAllocator(), nullptr, IID_PPV_ARGS(&m_commandList));
    if (FAILED(hr)) {
        printf("CreateCommandList failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    // Close it immediately - command lists are created in recording state
    hr = m_commandList->Close();
    if (FAILED(hr)) {
        printf("CommandList Close failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_isRecording = false;
    return true;
}

bool CommandList::Reset(CommandAllocator* allocator, ID3D12PipelineState* initialState) {
    if (!m_commandList || !allocator || !allocator->IsReady()) {
        return false;
    }

    // Reset the command list to start recording
    HRESULT hr = m_commandList->Reset(allocator->GetAllocator(), initialState);
    if (FAILED(hr)) {
        printf("CommandList Reset failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_isRecording = true;
    return true;
}

bool CommandList::Close() {
    if (!m_commandList || !m_isRecording) {
        return false;
    }

    // Close the command list to stop recording
    HRESULT hr = m_commandList->Close();
    if (FAILED(hr)) {
        printf("CommandList Close failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_isRecording = false;
    return true;
}

void CommandList::ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float clearColor[4]) {
    if (!m_commandList || !m_isRecording) {
        return;
    }

    m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandList::TransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
    if (!m_commandList || !m_isRecording || !resource) {
        return;
    }

    // Skip if no transition needed
    if (stateBefore == stateAfter) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrier);
}
