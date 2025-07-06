#include "RenderPass.h"

class TriangleClass : public RenderPass {
public:
    virtual ~TriangleClass() = default;

    virtual bool Initialize(ID3D12Device* device) override {
        CompileShaders();
        CreateRootSignature(device);
        CreatePipelineState(device);
        return true;
    }

    virtual void Execute(CommandList* cmdList, const RenderContext& ctx) override {
        // Empty implementation
    }

private:
    Shader m_vertexShader;
    Shader m_pixelShader;

    void CompileShaders() {
        std::string vertexShaderSource = R"(
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

        std::string pixelShaderSource = R"(
            struct VSOutput {
                float4 position : SV_POSITION;
                float3 color : COLOR;
            };
            float4 PSMain(VSOutput input) : SV_TARGET {
                return float4(input.color, 1.0);
            }
        )";

        bool vsSuccess = m_vertexShader.Initialize(vertexShaderSource, "VSMain", "vs_5_1", "TriangleVS");
        printf("Vertex shader compile result: %s\n", vsSuccess ? "SUCCESS" : "FAILED");
        if (!vsSuccess) {
            throw std::runtime_error("Failed to compile vertex shader");
        }

        bool psSuccess = m_pixelShader.Initialize(pixelShaderSource, "PSMain", "ps_5_1", "TrianglePS");
        printf("Pixel shader compile result: %s\n", psSuccess ? "SUCCESS" : "FAILED");
        if (!psSuccess) {
            throw std::runtime_error("Failed to compile pixel shader");
        }
    }

    void CreateRootSignature(ID3D12Device* device) {
        // Create properly configured static samplers
        D3D12_STATIC_SAMPLER_DESC staticSamplers[4] = {};
        // Point sampler
        staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[0].MipLODBias = 0.0f;
        staticSamplers[0].MaxAnisotropy = 1;
        staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplers[0].MinLOD = 0.0f;
        staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[0].ShaderRegister = 0;
        staticSamplers[0].RegisterSpace = 0;
        staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Linear sampler
        staticSamplers[1] = staticSamplers[0];
        staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[1].ShaderRegister = 1;

        // Anisotropic sampler
        staticSamplers[2] = staticSamplers[0];
        staticSamplers[2].Filter = D3D12_FILTER_ANISOTROPIC;
        staticSamplers[2].MaxAnisotropy = 16;
        staticSamplers[2].ShaderRegister = 2;

        // Comparison sampler
        staticSamplers[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
        staticSamplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[3].MipLODBias = 0.0f;
        staticSamplers[3].MaxAnisotropy = 1;
        staticSamplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        staticSamplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        staticSamplers[3].MinLOD = 0.0f;
        staticSamplers[3].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[3].ShaderRegister = 3;
        staticSamplers[3].RegisterSpace = 0;
        staticSamplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Create root signature
        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 0;
        rootSigDesc.pParameters = nullptr;
        rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
        rootSigDesc.pStaticSamplers = staticSamplers;
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

        // Rasterizer state
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // Blend state
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

        // Depth stencil state
        psoDesc.DepthStencilState.DepthEnable = FALSE; // Triangle doesn't need depth
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

        // Input layout for triangle (position + color)
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
        psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

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
