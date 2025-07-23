#pragma once

#include "dx12/core/DX12Common.h"
#include "renderer/Renderer.h"
#include "resources/GeometryManager.h"
#include "resources/UniformManager.h"

#include <components/Camera.h>
#include <entt/entt.hpp>

struct RenderContext {
    float deltaTime = 0.0f;
    Camera* targetCamera = nullptr;

    entt::registry& registry;
    GeometryManager* geometryManager;
    UniformManager* uniformManager = nullptr;
    // TextureManager* textureManager;
    // MaterialManager* materialManager;
    // ShaderManager* shaderManager;

    Renderer* renderer;
    explicit RenderContext(entt::registry& reg)
    : registry(reg) {}
};
