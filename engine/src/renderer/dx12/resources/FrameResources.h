#pragma once

#include "../core/CommandAllocator.h"

struct FrameResources {
    std::unique_ptr<CommandAllocator> commandAllocator;
    ComPtr<ID3D12Fence> frameFence;
    UINT64 fenceValue = 0;
    HANDLE fenceEvent = nullptr;

    FrameResources() = default;
    ~FrameResources() {
        if (fenceEvent) {
            CloseHandle(fenceEvent);
        }
    }

    // Non-copyable
    FrameResources(const FrameResources&) = delete;
    FrameResources& operator=(const FrameResources&) = delete;

    // Move semantics
    FrameResources(FrameResources&& other) noexcept
        : commandAllocator(std::move(other.commandAllocator))
        , frameFence(std::move(other.frameFence))
        , fenceValue(other.fenceValue)
        , fenceEvent(other.fenceEvent)
    {
        other.fenceEvent = nullptr;
        other.fenceValue = 0;
    }

    FrameResources& operator=(FrameResources&& other) noexcept {
        if (this != &other) {
            // Clean up current resources
            if (fenceEvent) {
                CloseHandle(fenceEvent);
            }

            // Move from other
            commandAllocator = std::move(other.commandAllocator);
            frameFence = std::move(other.frameFence);
            fenceValue = other.fenceValue;
            fenceEvent = other.fenceEvent;

            // Clear other
            other.fenceEvent = nullptr;
            other.fenceValue = 0;
        }
        return *this;
    }
};
