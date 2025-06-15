#pragma once

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "../core/DX12Common.h"

class Shader {
public:
    bool Initialize(ID3D12Device* device);
    void SetPipelineState(ID3D12GraphicsCommandList* cmdList);

private:
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};
