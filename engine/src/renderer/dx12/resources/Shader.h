#pragma once

#include "../core/DX12Common.h"
#include <renderer/ResourceDescriptions.h>

#include <string>
#include <memory>

using Microsoft::WRL::ComPtr;

class Shader {
public:
    Shader() = default;
    ~Shader() = default;

    bool Initialize(ID3D12Device* device, const ShaderDescription& desc);
    void SetPipelineState(ID3D12GraphicsCommandList* cmdList);

    const std::string& GetName() const { return m_name; }
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }

private:
    bool CompileShader(const std::string& source, const std::string& entryPoint,
                      const std::string& target, const std::string& debugName,
                      ComPtr<ID3DBlob>& outBlob);
    bool CreateRootSignature(ID3D12Device* device);
    bool CreatePipelineState(ID3D12Device* device, const ShaderDescription& desc,
                           ID3DBlob* vsBlob, ID3DBlob* psBlob);

    std::string m_name;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
};
