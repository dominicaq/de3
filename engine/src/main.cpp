#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"

// Engine includes
#include "renderer/Renderer.h"
#include "renderer/FPSUtils.h"
#include "io/InputManager.h"
#include <entt/entt.hpp>

// ECS systems
#include "components/systems/TransformSystem.h"
#include "components/systems/GameObjectSystem.h"
#include "sceneutils/SceneUtils.h"

// Geometry System
#include "renderer/renderpasses/RenderPassManager.h"
#include "resources/GeometryManager.h"
#include "resources/UniformManager.h"
#include "resources/ShaderManager.h"

// TEMP
#include "renderer/renderpasses/ForwardPass.h"
#include "RotationScript.h"
#include "FreeCamera.h"

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static EngineConfig g_config;

int main() {
    Window window;
    if (!window.Create(g_config)) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }
    InputManager::getInstance().init(window.GetHandle());

    std::unique_ptr<Renderer> renderer;
    try {
        renderer = std::make_unique<Renderer>(window.GetHandle(), g_config);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Graphics initialization failed: " << e.what() << std::endl;
        MessageBoxA(window.GetHandle(), e.what(), "DirectX 12 Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Setup resize callback: SwapChain will handle GPU synchronization
    window.SetResizeCallback([&renderer](UINT width, UINT height) {
        if (renderer) {
            renderer->OnReconfigure(width, height);
        }
    });

    PrintConfigStats(g_config);

    DX12Device* device = renderer->GetDevice();

    // ================================
    std::unique_ptr<ShaderManager> shaderManager = std::make_unique<ShaderManager>();
    shaderManager->SetShaderDirectories("../../shaders/hlsl/", "../../shaders/compiled/");

    // ================================
    std::unique_ptr<GeometryManager> geometryManager = std::make_unique<GeometryManager>(device->GetAllocator());
    GeometryManager::Config geoConfig;
    geoConfig.vertexBufferSize = 128 * 1024 * 1024;  // 128MB
    geoConfig.indexBufferSize = 32 * 1024 * 1024;    // 32MB
    geoConfig.uploadHeapSize = 8 * 1024 * 1024;      // 8MB
    geoConfig.maxUploadsPerFrame = 8;
    geometryManager->SetConfig(geoConfig);

    // ================================
    std::unique_ptr<UniformManager> uniformManager = std::make_unique<UniformManager>(
        device->GetAllocator(),
        device->GetD3D12Device()
    );

    // Per frame config
    UniformManager::Config uniformConfig;
    uniformConfig.frameBufferSize = 4 * 1024 * 1024;
    uniformConfig.maxDescriptors = 2000;
    uniformConfig.frameCount = 3;

    if (!uniformManager->Initialize(uniformConfig)) {
        std::cerr << "Failed to initialize UniformManager!" << std::endl;
        return -1;
    }

    // ================================
    // Render Pass System
    RenderPassManager passManager;
    passManager.AddPass(std::make_unique<ForwardPass>());

    // Initialize passes with both device and shader manager
    if (!passManager.InitializeAllPasses(device->GetD3D12Device(), shaderManager.get())) {
        throw std::runtime_error("Failed to init RenderPasses");
    }

    // Create render context with both managers
    // TEMP CODE
    entt::registry registry;
    RenderContext renderCtx {registry};
    renderCtx.geometryManager = geometryManager.get();
    renderCtx.uniformManager = uniformManager.get();
    renderCtx.renderer = renderer.get();

    VertexAttributes cubeVertices[] = {
        // Front face - Red (Z = +0.5)
        { {-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f} },  // 0
        { { 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f} },  // 1
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f} },  // 2
        { {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f} },  // 3

        // Back face - Green (Z = -0.5)
        { { 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 4
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 5
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 6
        { { 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f} },  // 7

        // Left face - Blue (X = -0.5)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f} },  // 8
        { {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f} },  // 9
        { {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f} },  // 10
        { {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f} },  // 11

        // Right face - Yellow (X = +0.5)
        { { 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f} },  // 12
        { { 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f} },  // 13
        { { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f} },  // 14
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f} },  // 15

        // Top face - Magenta (Y = +0.5)
        { {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f} },  // 16
        { { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f} },  // 17
        { { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f} },  // 18
        { {-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f} },  // 19

        // Bottom face - Cyan (Y = -0.5)
        { {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f} },  // 20
        { { 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f} },  // 21
        { { 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f} },  // 22
        { {-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f} }   // 23
    };

    uint32_t cubeIndices[] = {
        0, 2, 1,   0, 3, 2,
        4, 6, 5,   4, 7, 6,
        8, 10, 9,   8, 11, 10,
        12, 14, 13,   12, 15, 14,
        16, 18, 17,   16, 19, 18,
        20, 22, 21,   20, 23, 22
    };

    // Create a mesh on the CPU (this wont be done by hand in the future)
    CPUMesh triCPUdata;
    triCPUdata.vertices = cubeVertices;
    triCPUdata.indices = cubeIndices;
    triCPUdata.vertexCount = 24;
    triCPUdata.indexCount = 36;
    MeshHandle cubeMesh = INVALID_MESH_HANDLE;
    cubeMesh = geometryManager->CreateMesh(triCPUdata);
    if (cubeMesh == INVALID_MESH_HANDLE) {
        throw std::runtime_error("Failed to create triangle mesh");
    }

    printf("Created triangle mesh with handle %u\n", cubeMesh);

    // =========================================================================
    entt::entity temp_entity = registry.create();
    SceneData temp_saveData;
    temp_saveData.name = "Ground Plane";
    temp_saveData.position = glm::vec3(0.0f, 0.0f, -2.0f);
    temp_saveData.eulerAngles = glm::vec3(0.0f, 45.0f, 0.0f);
    temp_saveData.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    GameObject* tempObject = SceneUtils::addGameObjectComponent(registry, temp_entity, temp_saveData);
    tempObject->addScript<RotationScript>();
    registry.emplace<MeshHandle>(temp_entity, cubeMesh);

    // =========================================================================
    entt::entity cameraEntity = registry.create();
    SceneData cameraEntityData;
    cameraEntityData.name = "Primary Camera";
    cameraEntityData.position = glm::vec3(0.0f, 0.0f, 2.0f);
    cameraEntityData.eulerAngles = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraEntityData.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    GameObject* cameraObject = SceneUtils::addGameObjectComponent(registry, cameraEntity, cameraEntityData);
    cameraObject->addScript<FreeCamera>();
    cameraObject->addComponent<Camera>(cameraEntity, registry);

    renderCtx.targetCamera = &cameraObject->getComponent<Camera>();

    // END OF TEMP

    GameObjectSystem gameObjectSystem(renderCtx.registry);
    TransformSystem transformSystem(renderCtx.registry);

    // Game loop
    TimePoint lastTime = Clock::now();
    FPSUtils::FPSUtils fpsUtils;
    int frameCount = 0;
    float debugPrintTimer = 0;

    while (!window.ShouldClose()) {
        TimePoint frameStart = Clock::now();
        window.ProcessEvents();
        Input.update();

        CommandList* cmdList = renderer->BeginFrame();

        geometryManager->BeginFrame(frameCount, cmdList);
        uniformManager->BeginFrame(frameCount);

        renderCtx.targetCamera->setAspectRatio(
            (float)renderCtx.renderer->GetBackBufferWidth(),
            (float)renderCtx.renderer->GetBackBufferHeight()
        );
        passManager.ExecuteAllPasses(cmdList, renderCtx);

        uniformManager->EndFrame();
        renderer->EndFrame(g_config);

        if (g_config.cappedFPS) {
            fpsUtils.LimitFrameRate(g_config.targetFPS);
        }

        TimePoint frameEnd = Clock::now();
        std::chrono::duration<float> delta = frameEnd - frameStart;
        renderCtx.deltaTime = delta.count();
        frameCount++;

        // ECS updates
        gameObjectSystem.updateAll(0, renderCtx.deltaTime);
        transformSystem.updateTransformComponents();

#ifdef _DEBUG
        // Hot-reload shaders (check for file changes)
        passManager.CheckForShaderChanges(shaderManager.get());
        // Manual reload on F5 key
        if (Input.isKeyPressed(VK_F5)) {
            printf("Manual shader reload triggered\n");
            shaderManager->CheckForModifiedShaders();
        }

        if (frameCount % g_config.targetFPS == 0) {
            renderer->DebugPrintValidationMessages();
        }

        float fps;
        if (fpsUtils.UpdateFPSCounter(fps, 1000)) {
            std::cout << "FPS: " << fps << std::endl;
        }

        debugPrintTimer += renderCtx.deltaTime;
        if (debugPrintTimer >= 9.0f) {
            geometryManager->PrintDebugInfo();
            uniformManager->PrintStats();
            debugPrintTimer = 0.0f;
        }
#endif
    }

    // Cleanup
    renderer->FlushGPU();

    // Shutdown managers before renderer
    shaderManager.reset();
    uniformManager.reset();
    geometryManager.reset();

    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
