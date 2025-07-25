#include "Shader.h"
#include "../../../io/FileReader.h"
#include <iostream>

bool Shader::Initialize(const std::string& source, const std::string& entryPoint,
                       const std::string& target, const std::string& debugName) {
    m_name = debugName;
    ComPtr<ID3DBlob> errorBlob;

    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = D3DCompile(
        source.c_str(),
        source.length(),
        debugName.c_str(),
        nullptr,
        nullptr,
        entryPoint.c_str(),
        target.c_str(),
        compileFlags,
        0,
        &m_shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        printf("%s Compile FAILED: 0x%08X\n", debugName.c_str(), hr);
        if (errorBlob) {
            printf("%s Error: %s\n", debugName.c_str(), (char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    printf("%s compiled successfully\n", debugName.c_str());
    return true;
}

bool Shader::InitializeFromFile(const std::string& filePath,
                               const std::string& entryPoint,
                               const std::string& target,
                               const std::string& debugName) {
    std::string source = FileReader::ReadFile(filePath);
    if (source.empty()) {
        printf("Failed to load shader file '%s'\n", filePath.c_str());
        return false;
    }

    return Initialize(source, entryPoint, target, debugName);
}

bool Shader::InitializeFromBlob(ID3DBlob* blob, const std::string& debugName) {
    if (!blob) {
        printf("Shader::InitializeFromBlob: Invalid blob for %s\n", debugName.c_str());
        return false;
    }

    m_name = debugName;
    m_shaderBlob = blob;

    printf("%s loaded from cache successfully\n", debugName.c_str());
    return true;
}
