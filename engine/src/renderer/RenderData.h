#pragma once

#include "dx12/core/DX12Common.h"
#include "renderer/Renderer.h"
#include "resources/GeometryManager.h"
#include "resources/UniformManager.h"
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
    UniformManager* uniformManager = nullptr;
    // TextureManager* textureManager;
    // MaterialManager* materialManager;
    // ShaderManager* shaderManager;

    Renderer* renderer;
};
