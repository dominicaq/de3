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
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <exception>

// Microsoft's ThrowIfFailed implementation from DirectX-Graphics-Samples
class HrException : public std::runtime_error
{
    inline std::string HrToString(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

// Debug object naming (from Microsoft samples)
#if defined(_DEBUG) || defined(DBG)
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
    {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
        {
            pObject->SetName(fullName);
        }
    }
    #define NAME_D3D12_OBJECT(obj) SetName((obj).Get(), L#obj)
    #define NAME_D3D12_OBJECT_INDEXED(obj, n) SetNameIndexed((obj).Get(), L#obj, n)
#else
    #define NAME_D3D12_OBJECT(obj)
    #define NAME_D3D12_OBJECT_INDEXED(obj, n)
#endif

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
