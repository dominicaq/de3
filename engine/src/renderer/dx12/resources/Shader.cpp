#include "Shader.h"

const char* g_shaderSource = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;

    // Counter-clockwise triangle (proper winding for back-face culling)
    if (vertexID == 0) {
        output.position = float4(0.0, 0.5, 0.0, 1.0);   // Top vertex
        output.color = float3(1.0, 0.0, 0.0);           // Red
    }
    else if (vertexID == 1) {
        output.position = float4(0.5, -0.5, 0.0, 1.0);  // Bottom right
        output.color = float3(0.0, 1.0, 0.0);           // Green
    }
    else {
        output.position = float4(-0.5, -0.5, 0.0, 1.0); // Bottom left
        output.color = float3(0.0, 0.0, 1.0);           // Blue
    }

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
)";

bool Shader::Initialize(ID3D12Device* device) {
    if (!device) {
        printf("Shader::Initialize - NULL device!\n");
        return false;
    }

    HRESULT hr;
    ComPtr<ID3DBlob> vs, ps, errorBlob;

    printf("Compiling vertex shader...\n");
    // Compile vertex shader
    hr = D3DCompile(
        g_shaderSource,
        strlen(g_shaderSource),
        "TestShader.hlsl",
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &vs,
        &errorBlob
    );

    if (FAILED(hr)) {
        printf("VS Compile FAILED: 0x%08X\n", hr);
        if (errorBlob) {
            printf("VS Error: %s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    printf("Compiling pixel shader...\n");
    // Compile pixel shader
    errorBlob.Reset();
    hr = D3DCompile(
        g_shaderSource,
        strlen(g_shaderSource),
        "TestShader.hlsl",
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &ps,
        &errorBlob
    );

    if (FAILED(hr)) {
        printf("PS Compile FAILED: 0x%08X\n", hr);
        if (errorBlob) {
            printf("PS Error: %s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // Create properly configured static samplers to prevent warnings
    D3D12_STATIC_SAMPLER_DESC staticSamplers[4] = {};

    // Point sampler - properly configured
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].MipLODBias = 0.0f;
    staticSamplers[0].MaxAnisotropy = 1;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // Proper for non-comparison
    staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplers[0].MinLOD = 0.0f;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].RegisterSpace = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Linear sampler - properly configured
    staticSamplers[1] = staticSamplers[0]; // Copy base settings
    staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[1].ShaderRegister = 1;

    // Anisotropic sampler - properly configured
    staticSamplers[2] = staticSamplers[0]; // Copy base settings
    staticSamplers[2].Filter = D3D12_FILTER_ANISOTROPIC;
    staticSamplers[2].MaxAnisotropy = 16;
    staticSamplers[2].ShaderRegister = 2;

    // Comparison sampler - properly configured for depth/shadow sampling
    staticSamplers[3].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT; // Proper comparison filter
    staticSamplers[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[3].MipLODBias = 0.0f;
    staticSamplers[3].MaxAnisotropy = 1;
    staticSamplers[3].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // Now this makes sense
    staticSamplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    staticSamplers[3].MinLOD = 0.0f;
    staticSamplers[3].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[3].ShaderRegister = 3;
    staticSamplers[3].RegisterSpace = 0;
    staticSamplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // Create root signature with static samplers
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = _countof(staticSamplers);
    rootSigDesc.pStaticSamplers = staticSamplers;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> signature;
    errorBlob.Reset();
    hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &errorBlob
    );

    if (FAILED(hr)) {
        printf("Root Signature Serialize FAILED: 0x%08X\n", hr);
        if (errorBlob) {
            printf("Root Sig Error: %s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    hr = device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    if (FAILED(hr)) {
        printf("CreateRootSignature FAILED: 0x%08X\n", hr);
        return false;
    }

    // Create pipeline state
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    // Shaders
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS.pShaderBytecode = vs->GetBufferPointer();
    psoDesc.VS.BytecodeLength = vs->GetBufferSize();
    psoDesc.PS.pShaderBytecode = ps->GetBufferPointer();
    psoDesc.PS.BytecodeLength = ps->GetBufferSize();

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

    // Input layout (none for our vertex shader)
    psoDesc.InputLayout.pInputElementDescs = nullptr;
    psoDesc.InputLayout.NumElements = 0;

    psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Render target formats
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Must match swap chain format
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.NodeMask = 0;
    psoDesc.CachedPSO.pCachedBlob = nullptr;
    psoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        printf("CreateGraphicsPipelineState FAILED: 0x%08X\n", hr);
        return false;
    }

    return true;
}

void Shader::SetPipelineState(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineState.Get());
}
