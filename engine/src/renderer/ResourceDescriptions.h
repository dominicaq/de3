#pragma once

#include "dx12/core/DX12Common.h"

struct ShaderDescription {
    std::string name;
    std::string vertexShaderSource;
    std::string pixelShaderSource;
    std::string vertexEntryPoint = "VSMain";
    std::string pixelEntryPoint = "PSMain";
    std::string vertexTarget = "vs_5_0";
    std::string pixelTarget = "ps_5_0";
    DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    bool enableDepth = false;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};

struct TextureDescription {

};

struct MaterialDescription {

};
