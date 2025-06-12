#include "CommandAllocator.h"

bool CommandAllocator::Initialize(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type) {
    if (!device) {
        return false;
    }

    m_type = type;

    HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator));
    if (FAILED(hr)) {
        printf("CreateCommandAllocator failed with HRESULT: 0x%08X\n", hr);
        return false;
    }

    return true;
}

bool CommandAllocator::Reset() {
    if (!m_allocator) {
        return false;
    }

    // Reset the allocator to reuse memory
    HRESULT hr = m_allocator->Reset();
    if (FAILED(hr)) {
        return false;
    }
    return true;
}
