#pragma once

#include <d3d12.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>
#include <memory>
#include <array>
#include <vector>
#include <mutex>

using Microsoft::WRL::ComPtr;

class UniformManager {
public:
    struct UniformHandle {
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
        uint32_t descriptorIndex = UINT32_MAX;

        bool IsValid() const { return gpuAddress != 0; }
    };

    struct Config {
        size_t frameBufferSize = 2 * 1024 * 1024;  // 2MB per frame buffer
        uint32_t maxDescriptors = 2000;            // Max CBVs per frame
        uint32_t frameCount = 3;                   // Triple buffering
    };

    UniformManager(D3D12MA::Allocator* allocator, ID3D12Device* device);
    ~UniformManager();

    bool Initialize(const Config& config = {});
    void Shutdown();

    // Frame management
    void BeginFrame(uint32_t frameIndex);
    void EndFrame();

    // Uniform allocation and upload
    UniformHandle UploadUniform(const void* data, size_t size);

    // Descriptor heap management
    void BindDescriptorHeap(ID3D12GraphicsCommandList* cmdList);
    void SetGraphicsRootDescriptorTable(ID3D12GraphicsCommandList* cmdList,
                                       uint32_t rootParameterIndex,
                                       UniformHandle handle);

    // Debug/stats
    void PrintStats() const;
    size_t GetCurrentFrameUsage() const;

private:
    struct FrameData {
        D3D12MA::Allocation* allocation = nullptr;
        ID3D12Resource* resource = nullptr;
        uint8_t* mappedData = nullptr;
        size_t currentOffset = 0;
        size_t size = 0;
    };

    D3D12MA::Allocator* m_allocator = nullptr;
    ID3D12Device* m_device = nullptr;
    Config m_config;

    // Frame-based uniform buffers (ring buffer approach)
    std::array<FrameData, 3> m_frameData;
    uint32_t m_currentFrame = 0;
    uint32_t m_frameCount = 3;

    // Descriptor heap for CBVs
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    uint32_t m_descriptorSize = 0;
    uint32_t m_currentDescriptorOffset = 0;

    // Thread safety for multi-threaded upload
    std::mutex m_allocationMutex;

    bool m_isInitialized = false;

    bool CreateFrameBuffers();
    bool CreateDescriptorHeap();
    uint32_t AllocateDescriptor(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, size_t bufferSize);
    void ResetFrameData(FrameData& frameData);
};
