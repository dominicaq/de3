#pragma once

// Core DX12 headers
#include <d3d12.h>          // Main DX12 interfaces (ID3D12Device, etc.)
#include <dxgi1_6.h>        // DXGI for swap chain, factory, adapters
#include <d3dcompiler.h>    // HLSL shader compilation

// ComPtr smart pointer
#include <wrl/client.h>     // For Microsoft::WRL::ComPtr
using Microsoft::WRL::ComPtr;

// Standard library
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <bitset>

#include <sstream>
#include <stdexcept>

// DX12 Feature flags
struct DX12Features {
    enum FLAG : std::size_t {
        RAY_TRACING,
        MESH_SHADERS,
        VARIABLE_RATE_SHADING,
        SAMPLER_FEEDBACK,
        // Add more features ...
        FLAG_COUNT
    };
    std::bitset<FLAG_COUNT> features;
};

// Helper function declarations
namespace DX12Util {
    void ThrowIfFailed(HRESULT hr);
    std::string HrToString(HRESULT hr);
}
