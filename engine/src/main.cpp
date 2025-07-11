#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"
// Engine includes
#include "renderer/Renderer.h"
#include "renderer/FPSUtils.h"

#include <entt/entt.hpp>

// Geometry System
#include "renderer/renderpasses/RenderPassManager.h"
#include "resources/GeometryManager.h"
// TEMP
#include "renderer/renderpasses/TrianglePass.h"

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
    passManager.AddPass(std::make_unique<TrianglePass>());
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
    auto entity = registry.create();
    MeshHandle triangleMesh = INVALID_MESH_HANDLE;
    VertexAttributes triangleVertices[] = {
        { {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },    // Top - Red
        { {1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },   // Bottom right - Green
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }   // Bottom left - Blue
    };
    uint32_t triangleIndices[] = { 0, 1, 2 };

    // Create a mesh on the CPU (this wont be done by hand in the future)
    CPUMesh triCPUdata;
    triCPUdata.vertices = triangleVertices;
    triCPUdata.indices = triangleIndices;
    triCPUdata.vertexCount = 3;
    triCPUdata.indexCount = 3;
    triangleMesh = geometryManager->CreateMesh(triCPUdata);
    if (triangleMesh == INVALID_MESH_HANDLE) {
        throw std::runtime_error("Failed to create triangle mesh");
    }

    printf("Created triangle mesh with handle %u\n", triangleMesh);
    // END OF TEMP

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
