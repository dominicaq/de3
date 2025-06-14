#pragma once

#include "dx12/core/CommandAllocator.h"

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

    // Movable
    FrameResources(FrameResources&& other) noexcept
        : commandAllocator(std::move(other.commandAllocator))
        , frameFence(std::move(other.frameFence))
        , fenceValue(other.fenceValue)
        , fenceEvent(other.fenceEvent) {
        other.fenceEvent = nullptr;
        other.fenceValue = 0;
    }

    FrameResources& operator=(FrameResources&& other) noexcept {
        if (this != &other) {
            if (fenceEvent) {
                CloseHandle(fenceEvent);
            }

            commandAllocator = std::move(other.commandAllocator);
            frameFence = std::move(other.frameFence);
            fenceValue = other.fenceValue;
            fenceEvent = other.fenceEvent;

            other.fenceEvent = nullptr;
            other.fenceValue = 0;
        }
        return *this;
    }
};
