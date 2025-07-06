#include <windows.h>
#include <iostream>
#include "Window.h"
#include "Config.h"
// Engine includes
#include "renderer/Renderer.h"
#include "renderer/FPSUtils.h"

#include <entt/entt.hpp>

// Geometry System
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

    PrintConfigStats(g_config);

    // TEMP CODE
    entt::registry registry;
    RenderContext ctx {0.0f, registry};

    // Triangle
    auto entity = registry.create();

    // Geometry System
    DX12Device* device = renderer->GetDevice();
    std::unique_ptr<GeometryManager> geometryManager = std::make_unique<GeometryManager>(device->GetAllocator());

    // Optional: Configure geometry manager
    GeometryManager::Config geoConfig;
    geoConfig.vertexBufferSize = 128 * 1024 * 1024;  // 128MB - adjust as needed
    geoConfig.indexBufferSize = 32 * 1024 * 1024;    // 32MB
    geoConfig.uploadHeapSize = 8 * 1024 * 1024;      // 8MB
    geoConfig.maxUploadsPerFrame = 8;
    geometryManager->SetConfig(geoConfig);

    std::unique_ptr<TriangleClass> trianglepass = std::make_unique<TriangleClass>();
    if (!trianglepass->Initialize(device->GetDevice())) {
        throw std::runtime_error("Failed to initialize triangle render pass");
    }

    MeshHandle triangleMesh = INVALID_MESH_HANDLE;
    // Create triangle vertices
    VertexAttributes triangleVertices[] = {
        { {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f} },    // Top - Red
        { {1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },   // Bottom right - Green
        { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }   // Bottom left - Blue
    };
    uint32_t triangleIndices[] = { 0, 1, 2 };

    // Create mesh using simple interface
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
    int frameCount = 0;
    while (!window.ShouldClose()) {
        window.ProcessEvents();
        // Begin frame and get command list
        CommandList* cmdList = renderer->BeginFrame();
        // Handle geometry uploads automatically
        static uint32_t frameCounter = 0;
        geometryManager->BeginFrame(frameCounter++, cmdList);

        renderer->SetupRenderTarget(cmdList);
        renderer->SetupViewportAndScissor(cmdList);

        // Rainbow color that cycles over time
        static float time = 0.0f;
        time += 0.016f; // ~60fps increment

        float r = (sin(time * 2.0f) + 1.0f) * 0.5f;
        float g = (sin(time * 2.0f + 2.094f) + 1.0f) * 0.5f; // 2π/3 offset
        float b = (sin(time * 2.0f + 4.188f) + 1.0f) * 0.5f; // 4π/3 offset
        float clearColor[4] = { r, g, b, 1.0f };

        // Clear back buffer with animated rainbow color
        renderer->ClearBackBuffer(cmdList, clearColor);

        // Draw test triangle
        ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();
        if (!d3dCmdList) {
            printf("TestMeshDraw: Failed to get D3D12 command list\n");
            return -1;
        }

        // Set pipeline state and root signature from triangle pass
        d3dCmdList->SetGraphicsRootSignature(trianglepass->GetRootSignature());
        d3dCmdList->SetPipelineState(trianglepass->GetPipelineState());
        d3dCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // renderer->TestMeshDraw(cmdList);
        // Bind geometry buffers once
        geometryManager->BindVertexIndexBuffers(cmdList);
        const MeshRenderData* renderData = geometryManager->GetMeshRenderData(triangleMesh);
        if (renderData) {
            d3dCmdList->DrawIndexedInstanced(
                renderData->indexCount,     // IndexCountPerInstance
                1,                          // InstanceCount
                renderData->indexOffset,    // StartIndexLocation
                renderData->vertexOffset,   // BaseVertexLocation
                0                           // StartInstanceLocation
            );
        }
        // static uint32_t frameCount = 0;
        // if (++frameCount % 180 * 9 == 0) {  // Every 9 seconds
        //     renderer->GetGeometryManager()->PrintDebugInfo();
        // }

        // Finish frame and present
        renderer->EndFrame(g_config);

#ifdef _DEBUG
        if (frameCount % g_config.targetFPS == 0) {
            renderer->DebugPrintValidationMessages();
        }
#endif

        if (g_config.cappedFPS && !g_config.vsync) {
            FPSUtils::LimitFrameRate(g_config.targetFPS);
        }

        float fps;
        if (FPSUtils::UpdateFPSCounter(fps, 1000)) {
            std::cout << "FPS: " << fps << std::endl;
        }
    }

    std::cout << "Shutting down engine..." << std::endl;
    window.Destroy();
    std::cout << "Engine shutdown complete." << std::endl;
    return 0;
}
