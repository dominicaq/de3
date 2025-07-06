#pragma once
#include "../core/DX12Common.h"
#include <string>
#include <memory>

using Microsoft::WRL::ComPtr;

class Shader {
public:
    Shader() = default;
    ~Shader() = default;

    bool Initialize(const std::string& source, const std::string& entryPoint,
                   const std::string& target, const std::string& debugName);

    const std::string& GetName() const { return m_name; }
    ID3DBlob* GetShaderBlob() const { return m_shaderBlob.Get(); }

private:
    std::string m_name;
    ComPtr<ID3DBlob> m_shaderBlob;
};
