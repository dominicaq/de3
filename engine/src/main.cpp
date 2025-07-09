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

    PrintConfigStats(g_config);\

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

    // Create mesh using description
    MeshDescription triangleDesc = {};
    triangleDesc.name = "Triangle";
    triangleDesc.vertices = triangleVertices;
    triangleDesc.indices = triangleIndices;
    triangleDesc.vertexCount = 3;
    triangleDesc.indexCount = 3;
    triangleMesh = geometryManager->CreateMesh(triangleDesc);
    if (triangleMesh == INVALID_MESH_HANDLE) {
        throw std::runtime_error("Failed to create triangle mesh");
    }

    printf("Created triangle mesh with handle %u\n", triangleMesh);
    // END OF TEMP

    // Game loop
    FPSUtils::FPSUtils fpsUtils;
    int frameCount = 0;
    while (!window.ShouldClose()) {
        window.ProcessEvents();

        // Render loop
        CommandList* cmdList = renderer->BeginFrame();
        geometryManager->BeginFrame(frameCount++, cmdList);
        passManager.ExecuteAllPasses(cmdList, ctx);
        renderer->EndFrame(g_config);

        if (g_config.cappedFPS) {
            fpsUtils.LimitFrameRate(g_config.targetFPS);
        }
        frameCount++;

#ifdef _DEBUG
        if (frameCount % g_config.targetFPS == 0) {
            renderer->DebugPrintValidationMessages();
        }

        float fps;
        if (fpsUtils.UpdateFPSCounter(fps, 1000)) {
            std::cout << "FPS: " << fps << std::endl;
        }

        // static uint32_t frameCount = 0;
        // if (++frameCount % 180 * 9 == 0) {  // Every 9 seconds
        //     renderer->GetGeometryManager()->PrintDebugInfo();
        // }
#endif
    }

    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
