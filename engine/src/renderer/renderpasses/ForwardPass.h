#pragma once

#include "RenderPass.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entt.hpp>

class ForwardPass : public RenderPass {
public:
    ~ForwardPass() = default;

    virtual bool Initialize(ID3D12Device* device) override {
        CompileShaders();
        CreateRootSignature(device);
        CreatePipelineState(device);
        return true;
    }

    virtual void Execute(CommandList* cmdList, const RenderContext& ctx) override {
        ctx.renderer->SetupRenderTarget(cmdList);
        ctx.renderer->SetupViewportAndScissor(cmdList);

        if (!ctx.uniformManager) {
            printf("ForwardPass: UniformManager not available in context\n");
            return;
        }

        ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();
        ctx.uniformManager->BindDescriptorHeap(d3dCmdList);
        if (!d3dCmdList) {
            printf("ForwardPass: Failed to get D3D12 command list\n");
            return;
        }

        d3dCmdList->SetPipelineState(m_pipelineState.Get());
        d3dCmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        d3dCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ctx.geometryManager->BindVertexIndexBuffers(cmdList);

        float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        ctx.renderer->ClearBackBuffer(cmdList, clearColor);
        ctx.renderer->ClearDepthBuffer(cmdList, 1.0f, 0);

        // TODO: Instance rendering
        // Upload all model matrices to a buffer, then use instanced drawing

        glm::mat4 mvp;
        auto meshView = ctx.registry.view<MeshHandle, ModelMatrix>();
        for (auto entity : meshView) {
            const MeshHandle meshHandle = ctx.registry.get<MeshHandle>(entity);
            const auto& modelMatrix = ctx.registry.get<ModelMatrix>(entity);

            // Compute MVP for this specific entity
            mvp = ctx.targetCamera->getProjectionMatrix() * ctx.targetCamera->getViewMatrix() * modelMatrix.matrix;

            // Upload MVP for this draw call
            auto mvpUniform = ctx.uniformManager->UploadUniform(&mvp, sizeof(glm::mat4));
            if (!mvpUniform.IsValid()) {
                printf("ForwardPass: Failed to upload MVP constants for entity\n");
                continue;
            }

            // Bind MVP uniform for this entity
            ctx.uniformManager->SetGraphicsRootDescriptorTable(d3dCmdList, 0, mvpUniform);

            // Draw this entity's mesh
            DrawMesh(cmdList, ctx.geometryManager, meshHandle);
        }
    }
    virtual char* GetName() const override { return "Forward Pass"; }

private:
    Shader m_vertexShader;
    Shader m_pixelShader;

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
        // Single descriptor table for MVP constant buffer
        D3D12_ROOT_PARAMETER rootParameter = {};

        // MVP constants (b0)
        D3D12_DESCRIPTOR_RANGE mvpRange = {};
        mvpRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        mvpRange.NumDescriptors = 1;
        mvpRange.BaseShaderRegister = 0;
        mvpRange.RegisterSpace = 0;
        mvpRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameter.DescriptorTable.NumDescriptorRanges = 1;
        rootParameter.DescriptorTable.pDescriptorRanges = &mvpRange;

        // Create root signature
        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 1; // Single MVP constant buffer table
        rootSigDesc.pParameters = &rootParameter;
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

    void CreatePipelineState(ID3D12Device* device) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // Shaders
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS.pShaderBytecode = m_vertexShader.GetShaderBlob()->GetBufferPointer();
        psoDesc.VS.BytecodeLength = m_vertexShader.GetShaderBlob()->GetBufferSize();
        psoDesc.PS.pShaderBytecode = m_pixelShader.GetShaderBlob()->GetBufferPointer();
        psoDesc.PS.BytecodeLength = m_pixelShader.GetShaderBlob()->GetBufferSize();

        // Rasterizer state - enabled back-face culling for better depth testing
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;  // Enable back-face culling
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

        // **DEPTH STENCIL STATE - ENABLED FOR DEPTH TESTING**
        psoDesc.DepthStencilState.DepthEnable = TRUE;                           // Enable depth testing
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;  // Allow depth writes
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;       // Standard depth function (closer objects pass)
        psoDesc.DepthStencilState.StencilEnable = FALSE;                        // Disable stencil testing for now
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
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;  // **SPECIFY DEPTH BUFFER FORMAT**

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
