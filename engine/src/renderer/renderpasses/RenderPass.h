#include "dx12/core/DX12Common.h"
#include "dx12/core/CommandList.h"
#include "dx12/resources/Shader.h"
#include "../ResourceDescriptions.h"

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual bool Initialize(ID3D12Device* device) = 0;
    virtual void Execute(CommandList* cmdList, const RenderContext& ctx) = 0;

    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }
protected:
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;
};
