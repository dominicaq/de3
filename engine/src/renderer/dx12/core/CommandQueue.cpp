#include "CommandQueue.h"
#include <cassert>

CommandQueue::~CommandQueue() {
    // Wait for all GPU work to complete before destroying
    if (m_fence && m_commandQueue) {
        Flush();
    }

    // Close the fence event handle
    if (m_fenceEvent) {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }
}

bool CommandQueue::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
    if (!device) {
        return false;
    }

    m_queueType = type;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
    if (FAILED(hr)) {
        printf("CreateCommandQueue failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr)) {
        printf("CreateFence failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (!m_fenceEvent) {
        printf("CreateEventEx failed\n");
        return false;
    }

    return true;
}

void CommandQueue::ExecuteCommandLists(uint32_t numCommandLists, ID3D12CommandList* const* commandLists) {
    assert(m_commandQueue);
    m_commandQueue->ExecuteCommandLists(numCommandLists, commandLists);
}

uint64_t CommandQueue::Signal() {
    assert(m_commandQueue && m_fence);

    // Increment fence value and signal it
    ++m_fenceValue;
    HRESULT hr = m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    assert(SUCCEEDED(hr));

    return m_fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue) const {
    assert(m_fence);
    return m_fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue) {
    assert(m_fence && m_fenceEvent);

    if (IsFenceComplete(fenceValue)) {
        return; // Already complete
    }

    // Set the fence to signal the event when it reaches the target value
    HRESULT hr = m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
    assert(SUCCEEDED(hr));

    // Wait for the event to be signaled
    DWORD result = WaitForSingleObject(m_fenceEvent, INFINITE);
    assert(result == WAIT_OBJECT_0);
}

void CommandQueue::Flush() {
    // Signal and wait for the latest fence value
    uint64_t latestFenceValue = Signal();
    WaitForFenceValue(latestFenceValue);
}
