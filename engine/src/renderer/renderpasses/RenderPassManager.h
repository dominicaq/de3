#pragma once
#include "../dx12/core/DX12Common.h"
#include "../dx12/core/CommandList.h"
#include "../dx12/resources/Shader.h"
#include "../ResourceDescriptions.h"
#include "RenderPass.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

class RenderPassManager {
public:
    RenderPassManager() = default;
    ~RenderPassManager() = default;

    // Pass management
    void AddPass(std::unique_ptr<RenderPass> pass) {
        m_passes.push_back(std::move(pass));
    }

    // Execution
    void ExecuteAllPasses(CommandList* cmdList, const RenderContext& ctx) {
        ID3D12GraphicsCommandList* d3dCmdList = cmdList->GetCommandList();
        if (!d3dCmdList) {
            printf("ExecuteAllPasses: Failed to get D3D12 command list\n");
            return;
        }

        for (auto& pass : m_passes) {
            d3dCmdList->SetGraphicsRootSignature(pass->GetRootSignature());
            d3dCmdList->SetPipelineState(pass->GetPipelineState());
            pass->Execute(cmdList, ctx);
        }
    }

    // Lifecycle
    bool InitializeAllPasses(ID3D12Device* device) {
        for (auto& pass : m_passes) {
            if (!pass->Initialize(device)) {
                return false;
            }
        }
        return true;
    }

    void OnResize(uint32_t width, uint32_t height) {
        for (auto& pass : m_passes) {
            pass->OnResize(width, height);
        }
    }

private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
};
