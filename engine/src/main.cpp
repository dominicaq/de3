#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"

// Engine includes
#include "renderer/Renderer.h"
#include "renderer/FPSUtils.h"
#include <entt/entt.hpp>

// ECS systems
#include "components/systems/TransformSystem.h"
#include "components/systems/GameObjectSystem.h"
#include "sceneutils/SceneUtils.h"

// Geometry System
#include "renderer/renderpasses/RenderPassManager.h"
#include "resources/GeometryManager.h"

// TEMP
#include "renderer/renderpasses/ForwardPass.h"
#include "renderer/renderpasses/TrianglePass.h"
#include "RotationScript.h"

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static EngineConfig g_config;

int main() {
    Window window;
    if (!window.Create(g_config)) {
        std::cerr << "Failed to create window!" << std::endl;
        return -1;
    }

    std::unique_ptr<Renderer> renderer;
    try {
        // Initialize DirectX 12
        renderer = std::make_unique<Renderer>(window.GetHandle(), g_config);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Graphics initialization failed: " << e.what() << std::endl;
        MessageBoxA(window.GetHandle(), e.what(), "DirectX 12 Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // Setup resize callback - SwapChain will handle GPU synchronization
    window.SetResizeCallback([&renderer](UINT width, UINT height) {
        if (renderer) {
            renderer->OnReconfigure(width, height);
        }
    });

    PrintConfigStats(g_config);

    // TEMP CODE
    entt::registry registry;

    // Geometry System
    DX12Device* device = renderer->GetDevice();
    RenderPassManager passManager;
    passManager.AddPass(std::make_unique<ForwardPass>());
    // passManager.AddPass(std::make_unique<TrianglePass>());
    if (!passManager.InitializeAllPasses(device->GetD3D12Device())) {
        throw std::runtime_error("Failed to init RenderPasses");
    }

    // Configure
    std::unique_ptr<GeometryManager> geometryManager = std::make_unique<GeometryManager>(device->GetAllocator());
    GeometryManager::Config geoConfig;
    geoConfig.vertexBufferSize = 128 * 1024 * 1024;  // 128MB - adjust as needed
    geoConfig.indexBufferSize = 32 * 1024 * 1024;    // 32MB
    geoConfig.uploadHeapSize = 8 * 1024 * 1024;      // 8MB
    geoConfig.maxUploadsPerFrame = 8;
    geometryManager->SetConfig(geoConfig);

    // Now create context with geometry manager
    RenderContext ctx {0.0f, registry};
    ctx.geometryManager = geometryManager.get();
    ctx.renderer = renderer.get();

    // Hello Triangle
    MeshHandle triangleMesh = INVALID_MESH_HANDLE;
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
        // Front face (Red)
        0, 1, 2,   2, 3, 0,
        // Back face (Green)
        4, 5, 6,   6, 7, 4,
        // Left face (Blue)
        8, 9, 10,   10, 11, 8,
        // Right face (Yellow)
        12, 13, 14,   14, 15, 12,
        // Top face (Magenta)
        16, 17, 18,   18, 19, 16,
        // Bottom face (Cyan)
        20, 21, 22,   22, 23, 20
    };

    // Create a mesh on the CPU (this wont be done by hand in the future)
    CPUMesh triCPUdata;
    triCPUdata.vertices = cubeVertices;
    triCPUdata.indices = cubeIndices;
    triCPUdata.vertexCount = 24;
    triCPUdata.indexCount = 36;
    triangleMesh = geometryManager->CreateMesh(triCPUdata);
    if (triangleMesh == INVALID_MESH_HANDLE) {
        throw std::runtime_error("Failed to create triangle mesh");
    }

    printf("Created triangle mesh with handle %u\n", triangleMesh);
    // --------------------- Temp GameObject ---------------------
    entt::entity temp_entity = registry.create();
    SceneData temp_saveData;
    temp_saveData.name = "Ground Plane";
    temp_saveData.position = glm::vec3(0.0f, 0.0f, -2.0f);
    temp_saveData.eulerAngles = glm::vec3(0.0f, 45.0f, 0.0f);
    temp_saveData.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    GameObject* tempObject = SceneUtils::addGameObjectComponent(registry, temp_entity, temp_saveData);
    tempObject->addScript<RotationScript>();
    // TODO: assign meshIndex to the entity
    // END OF TEMP

    GameObjectSystem gameObjectSystem(ctx.registry);
    TransformSystem transformSystem(ctx.registry);

    // Game loop
    TimePoint lastTime = Clock::now();
    FPSUtils::FPSUtils fpsUtils;
    int frameCount = 0;
    float debugPrintTimer = 0;
    while (!window.ShouldClose()) {
        TimePoint frameStart = Clock::now();
        window.ProcessEvents();

        // Render loop
        CommandList* cmdList = renderer->BeginFrame();
        geometryManager->BeginFrame(frameCount, cmdList);
        passManager.ExecuteAllPasses(cmdList, ctx);
        renderer->EndFrame(g_config);

        if (g_config.cappedFPS) {
            fpsUtils.LimitFrameRate(g_config.targetFPS);
        }

        TimePoint frameEnd = Clock::now();
        std::chrono::duration<float> delta = frameEnd - frameStart;
        ctx.deltaTime = delta.count();
        frameCount++;

        // ECS updates
        gameObjectSystem.updateAll(0, ctx.deltaTime);
        transformSystem.updateTransformComponents();

#ifdef _DEBUG
        if (frameCount % g_config.targetFPS == 0) {
            renderer->DebugPrintValidationMessages();
        }

        float fps;
        if (fpsUtils.UpdateFPSCounter(fps, 1000)) {
            std::cout << "FPS: " << fps << std::endl;
        }

        debugPrintTimer += ctx.deltaTime;
        if (debugPrintTimer >= 9.0f) {
            geometryManager->PrintDebugInfo();
            debugPrintTimer = 0.0f;
        }
#endif
    }

    renderer->FlushGPU();
    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
