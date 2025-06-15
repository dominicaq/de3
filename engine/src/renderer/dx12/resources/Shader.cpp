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
    HRESULT hr;
    ComPtr<ID3DBlob> vs, ps, errorBlob;

    // Compile vertex shader
    hr = D3DCompile(
        g_shaderSource,
        strlen(g_shaderSource),
        nullptr,
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
        if (errorBlob) {
            printf("VS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // Compile pixel shader
    hr = D3DCompile(
        g_shaderSource,
        strlen(g_shaderSource),
        nullptr,
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
        if (errorBlob) {
            printf("PS Compile Error: %s\n", (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // Create empty root signature
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    hr = D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            printf("Root Signature Serialize Error: %s\n", (char*)errorBlob->GetBufferPointer());
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
        printf("CreateRootSignature failed: 0x%08X\n", hr);
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

    // Input layout
    psoDesc.InputLayout.pInputElementDescs = nullptr;
    psoDesc.InputLayout.NumElements = 0;

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

    hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        printf("CreateGraphicsPipelineState failed: 0x%08X\n", hr);
        return false;
    }

    return true;
}

void Shader::SetPipelineState(ID3D12GraphicsCommandList* cmdList) {
    cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
    cmdList->SetPipelineState(m_pipelineState.Get());
}
