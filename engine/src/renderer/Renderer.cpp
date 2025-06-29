#include "Renderer.h"

void Renderer::TEMP_FUNC() {
   // TODO: TEMP
   float triangleVertices[] = {
       // Position (3 floats)      Color (3 floats)
        0.0f,  0.5f, 0.0f,        1.0f, 0.0f, 0.0f,  // Top vertex - Red
        0.5f, -0.5f, 0.0f,        0.0f, 1.0f, 0.0f,  // Bottom right - Green
       -0.5f, -0.5f, 0.0f,        0.0f, 0.0f, 1.0f   // Bottom left - Blue
   };

   uint32_t triangleIndices[] = { 0, 1, 2 };

    // Register geometry with GeometryManager
    GeometryData geoData;
    geoData.vertexData = triangleVertices;
    geoData.vertexCount = 3;
    geoData.vertexStride = 6 * sizeof(float);
    geoData.indexData = triangleIndices;
    geoData.indexCount = 3;
    geoData.name = "Triangle";

    m_triangleGeometry = m_geometryManager->RegisterGeometry(geoData);
    if (m_triangleGeometry == INVALID_GEOMETRY_HANDLE) {
        throw std::runtime_error("Failed to register triangle geometry");
    }

   // Create test shader
   ShaderDescription triangleShaderDesc;
   triangleShaderDesc.name = "BasicTriangle";
   triangleShaderDesc.renderTargetFormat = m_swapChain->GetFormat();
   triangleShaderDesc.vertexShaderSource = R"(
   struct VSInput {
       float3 position : POSITION;
       float3 color : COLOR;
   };

   struct VSOutput {
       float4 position : SV_POSITION;
       float3 color : COLOR;
   };

   VSOutput VSMain(VSInput input) {
       VSOutput output;
       output.position = float4(input.position, 1.0);
       output.color = input.color;
       return output;
   }
   )";

   triangleShaderDesc.pixelShaderSource = R"(
   struct VSOutput {
       float4 position : SV_POSITION;
       float3 color : COLOR;
   };

   float4 PSMain(VSOutput input) : SV_TARGET {
       return float4(input.color, 1.0);
   }
   )";

   m_testShader = std::make_unique<Shader>();
   bool success = m_testShader->Initialize(m_device->GetDevice(), triangleShaderDesc);
   printf("Shader init result: %s\n", success ? "SUCCESS" : "FAILED");
   if (!success) {
       throw std::runtime_error("Failed to initialize test shader");
   }
}

Renderer::Renderer(HWND hwnd, const EngineConfig& config) {
    m_device = std::make_unique<DX12Device>();
    if (!m_device->Initialize(config.enableDebugLayer)) {
        throw std::runtime_error("Failed to initialize device");
    }

    // Create command queue manager
    m_commandManager = std::make_unique<CommandQueueManager>();
    if (!m_commandManager->Initialize(m_device->GetDevice())) {
        throw std::runtime_error("Failed to create command queues");
    }

    // Create SwapChain - use graphics queue
    m_swapChain = std::make_unique<SwapChain>(
        m_device.get(),
        m_commandManager->GetGraphicsQueue()->GetCommandQueue(),
        hwnd,
        config.windowWidth,
        config.windowHeight,
        config.backBufferCount,
        config.backBufferFormat
    );
    if (!m_swapChain->IsInitialized()) {
        throw std::runtime_error("Failed to create swap chain");
    }

    // Create RTV descriptor heap and views for back buffers
    if (!CreateBackBufferRTVs()) {
        throw std::runtime_error("Failed to create back buffer RTVs");
    }

    // Initialize per-frame resources
    if (!InitializeFrameResources()) {
        throw std::runtime_error("Failed to initialize frame resources");
    }

    // Create shared command list
    m_commandList = std::make_unique<CommandList>();
    if (!m_commandList->Initialize(m_device->GetDevice(),
                                   m_frameResources[0].commandAllocator.get(),
                                   D3D12_COMMAND_LIST_TYPE_DIRECT)) {
        throw std::runtime_error("Failed to create command list");
    }

    // TODO: remove
    m_geometryManager = std::make_unique<GeometryManager>(m_device->GetAllocator());
    TEMP_FUNC();
}

Renderer::~Renderer() {
    if (m_device && m_commandManager) {
        // Only wait for current frame - not all frames
        WaitForFrame(m_currentFrameIndex);
    }
    ReleaseBackBufferRTVs();
    m_frameResources.clear();
}

bool Renderer::CreateBackBufferRTVs() {
    // Release existing heap first
    ReleaseBackBufferRTVs();

    UINT bufferCount = m_swapChain->GetBufferCount();

    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = bufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    HRESULT hr = m_device->GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap));
    if (FAILED(hr)) {
        printf("Failed to create RTV descriptor heap: 0x%08X\n", hr);
        return false;
    }

#ifdef _DEBUG
    m_rtvDescriptorHeap->SetName(L"Back Buffer RTV Descriptor Heap");
#endif

    // Get descriptor increment size
    m_rtvDescriptorSize = m_device->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create RTVs for each back buffer
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < bufferCount; i++) {
        ID3D12Resource* backBuffer = m_swapChain->GetBackBuffer(i);
        if (!backBuffer) {
            printf("Failed to get back buffer %u\n", i);
            return false;
        }

        m_device->GetDevice()->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    return true;
}

void Renderer::ReleaseBackBufferRTVs() {
    m_rtvDescriptorHeap.Reset();
    m_rtvDescriptorSize = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentBackBufferRTV() const {
    return GetBackBufferRTV(m_swapChain->GetCurrentBackBufferIndex());
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetBackBufferRTV(UINT index) const {
    if (!m_rtvDescriptorHeap) {
        return {};
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * m_rtvDescriptorSize;
    return handle;
}

bool Renderer::InitializeFrameResources() {
    UINT bufferCount = m_swapChain->GetBufferCount();
    m_frameResources.resize(bufferCount);

    for (UINT i = 0; i < bufferCount; i++) {
        FrameResources& frame = m_frameResources[i];

        // Create command allocator for this frame
        frame.commandAllocator = std::make_unique<CommandAllocator>();
        if (!frame.commandAllocator->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT)) {
            printf("Failed to create command allocator for frame %u\n", i);
            return false;
        }

        // Create fence for this frame
        HRESULT hr = m_device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                                        IID_PPV_ARGS(&frame.frameFence));
        if (FAILED(hr)) {
            printf("Failed to create fence for frame %u: 0x%08X\n", i, hr);
            return false;
        }

        // Create fence event
        frame.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!frame.fenceEvent) {
            printf("Failed to create fence event for frame %u\n", i);
            return false;
        }

#ifdef _DEBUG
        std::wstring fenceName = L"Frame " + std::to_wstring(i) + L" Fence";
        frame.frameFence->SetName(fenceName.c_str());
#endif
    }

    return true;
}

CommandList* Renderer::BeginFrame() {
    // Get current frame index from swap chain
    m_currentFrameIndex = m_swapChain->GetCurrentBackBufferIndex();
    FrameResources& currentFrame = m_frameResources[m_currentFrameIndex];

    // Wait for this frame to be available (if we're cycling through faster than GPU can keep up)
    WaitForFrame(m_currentFrameIndex);

    // Reset command allocator for current frame
    if (!currentFrame.commandAllocator->Reset()) {
        printf("Failed to reset command allocator for frame %u\n", m_currentFrameIndex);
        return nullptr;
    }

    // Reset command list with current frame's allocator
    if (!m_commandList->Reset(currentFrame.commandAllocator.get())) {
        printf("Failed to reset command list for frame %u\n", m_currentFrameIndex);
        return nullptr;
    }

    // Transition back buffer to render target
    ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
    m_commandList->TransitionBarrier(
        backBuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    return m_commandList.get();
}

void Renderer::EndFrame(const EngineConfig& config) {
    FrameResources& currentFrame = m_frameResources[m_currentFrameIndex];

    // Transition back buffer to present
    ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
    m_commandList->TransitionBarrier(
        backBuffer,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    if (!m_commandList->Close()) {
        printf("Failed to close command list for frame %u\n", m_currentFrameIndex);
        return;
    }

    // Execute commands
    ID3D12CommandList* commandLists[] = { m_commandList->GetCommandList() };
    m_commandManager->GetGraphicsQueue()->ExecuteCommandLists(1, commandLists);

    // Signal fence for current frame with unique value
    const UINT64 currentFenceValue = m_nextFenceValue++;
    currentFrame.fenceValue = currentFenceValue;

    HRESULT hr = m_commandManager->GetGraphicsQueue()->GetCommandQueue()->Signal(
        currentFrame.frameFence.Get(), currentFenceValue);
    if (FAILED(hr)) {
        printf("Failed to signal fence for frame %u: 0x%08X\n", m_currentFrameIndex, hr);
    }

    m_swapChain->Present(config.vsync);
}

void Renderer::WaitForFrame(UINT frameIndex) {
    if (frameIndex >= m_frameResources.size()) {
        return;
    }

    FrameResources& frame = m_frameResources[frameIndex];

    // If no fence value set, frame hasn't been used yet
    if (frame.fenceValue == 0) {
        return;
    }

    // Check if frame is already complete
    if (frame.frameFence->GetCompletedValue() >= frame.fenceValue) {
        return;
    }

    // Wait for frame completion
    HRESULT hr = frame.frameFence->SetEventOnCompletion(frame.fenceValue, frame.fenceEvent);
    if (FAILED(hr)) {
        printf("SetEventOnCompletion failed for frame %u: 0x%08X\n", frameIndex, hr);
        return;
    }

    WaitForSingleObject(frame.fenceEvent, INFINITE);
}

void Renderer::WaitForAllFrames() {
    for (UINT i = 0; i < m_frameResources.size(); i++) {
        WaitForFrame(i);
    }
}

bool Renderer::IsFrameComplete(UINT frameIndex) const {
    if (frameIndex >= m_frameResources.size()) {
        return true;
    }

    const FrameResources& frame = m_frameResources[frameIndex];

    // If no fence value set, frame hasn't been used yet
    if (frame.fenceValue == 0) {
        return true;
    }

    return frame.frameFence->GetCompletedValue() >= frame.fenceValue;
}

void Renderer::SetupRenderTarget(CommandList* cmdList) {
    if (!cmdList) {
        printf("SetupRenderTarget: Invalid command list\n");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetCurrentBackBufferRTV();
    cmdList->GetCommandList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void Renderer::SetupViewportAndScissor(CommandList* cmdList) {
    if (!cmdList) {
        printf("SetupViewportAndScissor: Invalid command list\n");
        return;
    }

    ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();

    // Set viewport
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(m_swapChain->GetWidth());
    viewport.Height = static_cast<FLOAT>(m_swapChain->GetHeight());
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d3dCmdList->RSSetViewports(1, &viewport);

    // Set scissor rect
    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(m_swapChain->GetWidth());
    scissorRect.bottom = static_cast<LONG>(m_swapChain->GetHeight());
    d3dCmdList->RSSetScissorRects(1, &scissorRect);
}

void Renderer::ClearBackBuffer(CommandList* cmdList, const float clearColor[4]) {
    if (!cmdList || !clearColor) {
        printf("ClearBackBuffer: Invalid command list or clear color\n");
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = GetCurrentBackBufferRTV();
    cmdList->GetCommandList()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

UINT Renderer::GetBackBufferWidth() const {
    return m_swapChain ? m_swapChain->GetWidth() : 0;
}

UINT Renderer::GetBackBufferHeight() const {
    return m_swapChain ? m_swapChain->GetHeight() : 0;
}

DXGI_FORMAT Renderer::GetBackBufferFormat() const {
    return m_swapChain ? m_swapChain->GetFormat() : DXGI_FORMAT_UNKNOWN;
}

void Renderer::OnReconfigure(UINT width, UINT height, UINT bufferCount) {
    if (!m_swapChain) {
        return;
    }

    // Wait for all frames before reconfiguring
    WaitForAllFrames();

    // If buffer count is changing, we need to recreate frame resources
    UINT oldBufferCount = m_swapChain->GetBufferCount();
    UINT newBufferCount = (bufferCount == 0) ? oldBufferCount : bufferCount;

    // Release RTVs before swap chain reconfigure
    ReleaseBackBufferRTVs();

    if (!m_swapChain->Reconfigure(width, height, bufferCount)) {
        printf("Failed to reconfigure swap chain\n");
        return;
    }

    // Recreate RTVs for new back buffers
    if (!CreateBackBufferRTVs()) {
        printf("Failed to recreate back buffer RTVs after reconfigure\n");
        return;
    }

    // Recreate frame resources if buffer count changed
    if (newBufferCount != oldBufferCount) {
        m_frameResources.clear();
        if (!InitializeFrameResources()) {
            printf("Failed to reinitialize frame resources after reconfigure\n");
        }
    }
}

void Renderer::TestShaderDraw(CommandList* cmdList) {
    if (!m_testShader || !cmdList || m_triangleGeometry == INVALID_GEOMETRY_HANDLE) {
        printf("TestShaderDraw: Invalid resources\n");
        return;
    }

    ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();
    if (!d3dCmdList) {
        printf("TestShaderDraw: Failed to get D3D12 command list\n");
        return;
    }

    // Set pipeline state and root signature
    m_testShader->SetPipelineState(d3dCmdList);

    // Get geometry description
    const GeometryDescription& geoDesc = m_geometryManager->GetGeometryDesc(m_triangleGeometry);

    // Bind geometry manager's buffers
    m_geometryManager->BindBuffers(cmdList);

    // Set primitive topology
    d3dCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw using offsets into shared buffers
    d3dCmdList->DrawIndexedInstanced(
        geoDesc.indexCount,     // 3 indices
        1,                      // 1 instance
        geoDesc.indexOffset,    // Start index
        geoDesc.vertexOffset,   // Base vertex
        0                       // Start instance
    );
}
