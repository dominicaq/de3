#pragma once

#include "../dx12/core/DX12Common.h"
#include "../dx12/core/CommandList.h"
#include "../dx12/resources/Shader.h"
#include "../RenderData.h"

class RenderPass {
public:
    virtual ~RenderPass() = default;

    // Life cycle - Updated to include ShaderManager
    virtual bool Initialize(ID3D12Device* device, ShaderManager* shaderManager = nullptr) = 0;
    virtual void Execute(CommandList* cmdList, const RenderContext& ctx) = 0;

    // Interface
    virtual void OnResize(uint32_t width, uint32_t height) {}
    virtual char* GetName() const { return "N/A"; }

    // Getters
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }

protected:
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;

    // Protected draw call methods for derived classes
    virtual void DrawMesh(CommandList* cmdList, GeometryManager* geometryManager, MeshHandle meshHandle) {
        if (!cmdList || !geometryManager || meshHandle == INVALID_MESH_HANDLE) {
            return;
        }
        const MeshView* renderData = geometryManager->GetMeshRenderData(meshHandle);
        if (renderData) {
            cmdList->GetCommandList()->DrawIndexedInstanced(
                renderData->indexCount,     // IndexCountPerInstance
                1,                          // InstanceCount
                renderData->indexOffset,    // StartIndexLocation
                renderData->vertexOffset,   // BaseVertexLocation
                0                           // StartInstanceLocation
            );
        }
    }

    virtual void DrawMeshInstanced(CommandList* cmdList, GeometryManager* geometryManager,
                                  MeshHandle meshHandle, uint32_t instanceCount,
                                  uint32_t startInstance = 0) {
        if (!cmdList || !geometryManager || meshHandle == INVALID_MESH_HANDLE) {
            return;
        }
        const MeshView* renderData = geometryManager->GetMeshRenderData(meshHandle);
        if (renderData) {
            cmdList->GetCommandList()->DrawIndexedInstanced(
                renderData->indexCount,
                instanceCount,
                renderData->indexOffset,
                renderData->vertexOffset,
                startInstance
            );
        }
    }
};
