#pragma once

#include "dx12/core/DX12Common.h"
#include "renderer/Renderer.h"
#include "resources/GeometryManager.h"
#include <entt/entt.hpp>

struct FrameData {
    float deltaTime;
    // Matrix4 view;
    // Matrix4 projection;
    // Matrix4 viewProjection;
    // Other per-frame data
};

struct RenderContext {
    // const FrameData& frameData;
    float deltaTime;

    entt::registry& registry;
    GeometryManager* geometryManager;
    // TextureManager* textureManager;
    // MaterialManager* materialManager;
    // ShaderManager* shaderManager;

    Renderer* renderer;
};

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
