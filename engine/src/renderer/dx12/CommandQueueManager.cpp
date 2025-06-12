#include "CommandQueueManager.h"

bool CommandQueueManager::Initialize(ID3D12Device* device) {
    if (!device) {
        return false;
    }

    m_device = device;

    // Create graphics queue (can do graphics, compute, and copy)
    m_graphicsQueue = std::make_unique<CommandQueue>();
    if (!m_graphicsQueue->Initialize(device, D3D12_COMMAND_LIST_TYPE_DIRECT)) {
        printf("CommandQueueManager failed to create graphics queue\n");
        return false;
    }

    m_computeQueue = std::make_unique<CommandQueue>();
    if (!m_computeQueue->Initialize(device, D3D12_COMMAND_LIST_TYPE_COMPUTE)) {
        printf("CommandQueueManager failed to create compute queue\n");
        return false;
    }

    m_copyQueue = std::make_unique<CommandQueue>();
    if (!m_copyQueue->Initialize(device, D3D12_COMMAND_LIST_TYPE_COPY)) {
        printf("CommandQueueManager failed to create copy queue\n");
        return false;
    }

    return true;
}

void CommandQueueManager::Shutdown() {
    // Flush all queues before shutdown
    if (IsInitialized()) {
        FlushAll();
    }

    // Reset all queues (destructors handle cleanup)
    m_copyQueue.reset();
    m_computeQueue.reset();
    m_graphicsQueue.reset();

    m_device = nullptr;
}

void CommandQueueManager::FlushAll() {
    if (m_graphicsQueue) {
        m_graphicsQueue->Flush();
    }

    if (m_computeQueue) {
        m_computeQueue->Flush();
    }

    if (m_copyQueue) {
        m_copyQueue->Flush();
    }
}

bool CommandQueueManager::IsInitialized() const {
    return m_device != nullptr &&
           m_graphicsQueue != nullptr &&
           m_computeQueue != nullptr &&
           m_copyQueue != nullptr;
}

CommandQueue* CommandQueueManager::GetQueue(D3D12_COMMAND_LIST_TYPE type) const {
    switch (type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return m_graphicsQueue.get();

    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return m_computeQueue.get();

    case D3D12_COMMAND_LIST_TYPE_COPY:
        return m_copyQueue.get();

    default:
        return nullptr;
    }
}
