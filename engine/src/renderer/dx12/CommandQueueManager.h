#pragma once

#include "core/CommandQueue.h"
#include <memory>

class CommandQueueManager {
public:
    CommandQueueManager() = default;
    ~CommandQueueManager() = default;

    // Non-copyable
    CommandQueueManager(const CommandQueueManager&) = delete;
    CommandQueueManager& operator=(const CommandQueueManager&) = delete;

    // Movable
    CommandQueueManager(CommandQueueManager&&) = default;
    CommandQueueManager& operator=(CommandQueueManager&&) = default;

    // Initialize all command queues
    bool Initialize(ID3D12Device* device);
    void Shutdown();

    // Get specific queue types
    CommandQueue* GetGraphicsQueue() const { return m_graphicsQueue.get(); }
    CommandQueue* GetComputeQueue() const { return m_computeQueue.get(); }
    CommandQueue* GetCopyQueue() const { return m_copyQueue.get(); }
    // Get queue by type
    CommandQueue* GetQueue(D3D12_COMMAND_LIST_TYPE type) const;

    // Convenience methods
    void FlushAll();
    bool IsInitialized() const;

private:
    std::unique_ptr<CommandQueue> m_graphicsQueue;
    std::unique_ptr<CommandQueue> m_computeQueue;
    std::unique_ptr<CommandQueue> m_copyQueue;

    ID3D12Device* m_device = nullptr;
};
