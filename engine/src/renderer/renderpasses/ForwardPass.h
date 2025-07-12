#pragma once

#include "RenderPass.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entt.hpp>

// Constant buffer structure for GPU
struct MVPConstants {
    glm::mat4 mvp;
    // Add padding if needed for 256-byte alignment
};

class ForwardPass : public RenderPass {
public:
    ~ForwardPass() = default;

    virtual bool Initialize(ID3D12Device* device) override {
        m_device = device;
        CompileShaders();
        CreateRootSignature(device);
        CreatePipelineState(device);
        CreateConstantBuffer(device);
        return true;
    }

    virtual void Execute(CommandList* cmdList, const RenderContext& ctx) override {
        ctx.renderer->SetupRenderTarget(cmdList);
        ctx.renderer->SetupViewportAndScissor(cmdList);

        // Standard clear color (can be made configurable)
        float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        ctx.renderer->ClearBackBuffer(cmdList, clearColor);

        ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();
        if (!d3dCmdList) {
            printf("ForwardPass: Failed to get D3D12 command list\n");
            return;
        }

        // Set pipeline state and root signature
        d3dCmdList->SetPipelineState(m_pipelineState.Get());
        d3dCmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        d3dCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Bind vertex and index buffers
        ctx.geometryManager->BindVertexIndexBuffers(cmdList);

        // Update and bind MVP matrix
        UpdateMVPMatrix(ctx.registry);
        d3dCmdList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

        // Draw mesh with index 1
        DrawMesh(cmdList, ctx.geometryManager, 1);
    }

    virtual char* GetName() const override { return "Forward Pass"; }

private:
    Shader m_vertexShader;
    Shader m_pixelShader;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_constantBuffer;
    void* m_constantBufferData = nullptr;

    void CompileShaders() {
        std::string vertexShaderSource = R"(
            cbuffer MVPBuffer : register(b0) {
                row_major float4x4 mvp;
            };

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
                output.position = mul(float4(input.position, 1.0), mvp);
                output.color = input.color;
                return output;
            }
        )";

        std::string pixelShaderSource = R"(
            struct VSOutput {
                float4 position : SV_POSITION;
                float3 color : COLOR;
            };

            float4 PSMain(VSOutput input) : SV_TARGET {
                return float4(input.color, 1.0);
            }
        )";

        bool vsSuccess = m_vertexShader.Initialize(vertexShaderSource, "VSMain", "vs_5_1", "ForwardVS");
        printf("Forward vertex shader compile result: %s\n", vsSuccess ? "SUCCESS" : "FAILED");
        if (!vsSuccess) {
            throw std::runtime_error("Failed to compile forward vertex shader");
        }

        bool psSuccess = m_pixelShader.Initialize(pixelShaderSource, "PSMain", "ps_5_1", "ForwardPS");
        printf("Forward pixel shader compile result: %s\n", psSuccess ? "SUCCESS" : "FAILED");
        if (!psSuccess) {
            throw std::runtime_error("Failed to compile forward pixel shader");
        }
    }

    void CreateRootSignature(ID3D12Device* device) {
        // Root parameter for MVP constant buffer
        D3D12_ROOT_PARAMETER rootParam = {};
        rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParam.Descriptor.ShaderRegister = 0;
        rootParam.Descriptor.RegisterSpace = 0;
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 1;
        rootSigDesc.pParameters = &rootParam;
        rootSigDesc.NumStaticSamplers = 0;
        rootSigDesc.pStaticSamplers = nullptr;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        ComPtr<ID3DBlob> signature, errorBlob;
        ThrowIfFailed(D3D12SerializeRootSignature(
            &rootSigDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signature,
            &errorBlob
        ));

        ThrowIfFailed(device->CreateRootSignature(
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature)
        ));
    }

    void CreateConstantBuffer(ID3D12Device* device) {
        // Create constant buffer (256-byte aligned)
        UINT bufferSize = (sizeof(MVPConstants) + 255) & ~255;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = bufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)
        ));

        // Map the constant buffer
        D3D12_RANGE readRange = { 0, 0 };
        ThrowIfFailed(m_constantBuffer->Map(0, &readRange, &m_constantBufferData));
    }

    void UpdateMVPMatrix(entt::registry& registry) {
        // Create view and projection matrices
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 1.0f, 2.0f),   // Camera position - diagonal view to see cube properly
            glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
        );

        // Use proper DX12 projection (0-1 depth range)
        glm::mat4 projection = glm::perspectiveFov(
            glm::radians(45.0f),    // 45 degree FOV
            1920.0f, 1080.0f,       // Screen dimensions
            0.1f,                   // Near plane
            100.0f                  // Far plane
        );

        // Find the entity with mesh index 1 (you might want to store this mapping)
        // For now, get the first entity with a ModelMatrix component
        glm::mat4 model = glm::mat4(1.0f);

        auto view_entities = registry.view<ModelMatrix>();
        if (!view_entities.empty()) {
            auto entity = view_entities.front();
            const auto& modelMatrix = registry.get<ModelMatrix>(entity);
            model = modelMatrix.matrix;
        }

        // Calculate MVP matrix
        glm::mat4 mvp = projection * view * model;

        // Update constant buffer
        MVPConstants constants;
        constants.mvp = mvp;

        memcpy(m_constantBufferData, &constants, sizeof(MVPConstants));
    }

    void CreatePipelineState(ID3D12Device* device) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // Shaders
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS.pShaderBytecode = m_vertexShader.GetShaderBlob()->GetBufferPointer();
        psoDesc.VS.BytecodeLength = m_vertexShader.GetShaderBlob()->GetBufferSize();
        psoDesc.PS.pShaderBytecode = m_pixelShader.GetShaderBlob()->GetBufferPointer();
        psoDesc.PS.BytecodeLength = m_pixelShader.GetShaderBlob()->GetBufferSize();

        // Rasterizer state
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Blend state (standard alpha blending)
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
            psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
        }

        // Depth stencil state (disabled for simplicity)
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
        };
        psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
        psoDesc.DepthStencilState.BackFace = defaultStencilOp;

        // Input layout for position + color rendering
        D3D12_INPUT_ELEMENT_DESC inputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        psoDesc.InputLayout.pInputElementDescs = inputElements;
        psoDesc.InputLayout.NumElements = _countof(inputElements);

        psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Render target formats
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; // No depth buffer for now

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.NodeMask = 0;
        psoDesc.CachedPSO.pCachedBlob = nullptr;
        psoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }
};
