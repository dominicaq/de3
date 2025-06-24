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

// class Shader {
// public:
//     // Construction & Loading
//     bool LoadFromFile(ID3D12Device* device, const std::string& filePath,
//                      const std::string& vsEntry = "VSMain",
//                      const std::string& psEntry = "PSMain");

//     bool LoadFromSource(ID3D12Device* device, const std::string& source,
//                        const std::string& vsEntry = "VSMain",
//                        const std::string& psEntry = "PSMain");

//     // Variant support (for future ShaderManager)
//     bool LoadWithDefines(ID3D12Device* device, const std::string& filePath,
//                         const std::vector<std::string>& defines,
//                         const std::string& vsEntry = "VSMain",
//                         const std::string& psEntry = "PSMain");

//     // Pipeline state creation with configurable states
//     bool CreatePipelineState(ID3D12Device* device,
//                            const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

//     // Simplified pipeline state creation with defaults
//     bool CreateDefaultPipelineState(ID3D12Device* device, DXGI_FORMAT renderTargetFormat);

//     // Rendering
//     void SetPipelineState(ID3D12GraphicsCommandList* cmdList);

//     // Accessors (useful for ShaderManager)
//     ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
//     ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }
//     const std::string& GetFilePath() const { return m_filePath; }

//     // Hot reload support
//     bool NeedsReload() const;
//     bool Reload(ID3D12Device* device);

// private:
//     // Compilation
//     bool CompileShader(const std::string& source, const std::string& entryPoint,
//                       const std::string& target, const std::vector<std::string>& defines,
//                       ComPtr<ID3DBlob>& outBlob);

//     // Root signature creation
//     bool CreateRootSignature(ID3D12Device* device);

//     // Data members
//     ComPtr<ID3D12RootSignature> m_rootSignature;
//     ComPtr<ID3D12PipelineState> m_pipelineState;

//     // Hot reload support
//     std::string m_filePath;
//     FILETIME m_lastWriteTime;
//     std::vector<std::string> m_defines;
//     std::string m_vsEntry, m_psEntry;

//     // Shader bytecode (keep for hot reload)
//     ComPtr<ID3DBlob> m_vsBlob, m_psBlob;
// };