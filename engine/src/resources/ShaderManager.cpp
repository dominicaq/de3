#include "ShaderManager.h"
#include "../io/FileReader.h"
#include <iostream>
#include <fstream>

ShaderHandle ShaderManager::CreateShaderFromFile(const std::string& filePath,
                                                 const std::string& entryPoint,
                                                 const std::string& target,
                                                 const std::string& debugName) {
    // Check if we already have this shader
    std::string key = filePath + entryPoint + target;
    auto it = m_pathToHandle.find(key);
    if (it != m_pathToHandle.end()) {
        printf("ShaderManager: Reusing existing shader %s\n", debugName.c_str());
        return it->second;
    }

    // Setup file paths
    std::string fullSourcePath = m_sourceDir + filePath;
    std::string cacheFileName = GenerateCacheFileName(filePath, entryPoint, target);
    std::string fullCachePath = m_cacheDir + cacheFileName;

    printf("ShaderManager: File exists? %s\n", std::filesystem::exists(fullSourcePath) ? "YES" : "NO");

    // Create cache info
    ShaderCacheInfo cacheInfo;
    cacheInfo.sourceFile = fullSourcePath;
    cacheInfo.cacheFile = fullCachePath;
    cacheInfo.sourceModTime = GetFileModTime(fullSourcePath);
    cacheInfo.cacheModTime = GetFileModTime(fullCachePath);

    // Create new shader
    auto shader = std::make_unique<Shader>();

    // Try to load from cache first
    if (IsCacheValid(cacheInfo) && LoadFromCache(fullCachePath, shader.get(), debugName)) {
        printf("ShaderManager: Loaded shader %s from cache\n", debugName.c_str());
    }
    else {
        // Compile from source
        if (!shader->InitializeFromFile(fullSourcePath, entryPoint, target, debugName)) {
            printf("ShaderManager: Failed to create shader %s from %s\n",
                   debugName.c_str(), fullSourcePath.c_str());
            return INVALID_SHADER_HANDLE;
        }

        // Save to cache
        if (SaveToCache(fullCachePath, shader.get())) {
            printf("ShaderManager: Cached shader %s to %s\n", debugName.c_str(), fullCachePath.c_str());
            cacheInfo.cacheModTime = GetFileModTime(fullCachePath);
        }
    }

    ShaderHandle handle = GenerateHandle();

    ShaderEntry entry;
    entry.shader = std::move(shader);
    entry.desc = {filePath, entryPoint, target, debugName};
    entry.cacheInfo = cacheInfo;

    m_shaders[handle] = std::move(entry);
    m_pathToHandle[key] = handle;

    printf("ShaderManager: Created shader %s (handle: %u)\n", debugName.c_str(), handle);
    return handle;
}

ShaderHandle ShaderManager::CreateShaderFromSource(const std::string& source,
                                                   const std::string& entryPoint,
                                                   const std::string& target,
                                                   const std::string& debugName) {
    auto shader = std::make_unique<Shader>();
    if (!shader->Initialize(source, entryPoint, target, debugName)) {
        printf("ShaderManager: Failed to create shader %s from source\n", debugName.c_str());
        return INVALID_SHADER_HANDLE;
    }

    ShaderHandle handle = GenerateHandle();

    ShaderEntry entry;
    entry.shader = std::move(shader);
    entry.desc = {"", entryPoint, target, debugName}; // No file path for source shaders
    entry.cacheInfo = {}; // Empty cache info for source shaders

    m_shaders[handle] = std::move(entry);

    printf("ShaderManager: Created shader %s from source (handle: %u)\n", debugName.c_str(), handle);
    return handle;
}

const Shader* ShaderManager::GetShader(ShaderHandle handle) const {
    auto it = m_shaders.find(handle);
    if (it != m_shaders.end()) {
        return it->second.shader.get();
    }
    return nullptr;
}

ShaderHandle ShaderManager::GetOrCreateShader(const ShaderDesc& desc) {
    return CreateShaderFromFile(desc.filePath, desc.entryPoint, desc.target, desc.debugName);
}

void ShaderManager::DestroyShader(ShaderHandle handle) {
    auto it = m_shaders.find(handle);
    if (it != m_shaders.end()) {
        // Remove from path mapping if it exists
        if (!it->second.desc.filePath.empty()) {
            std::string key = it->second.desc.filePath +
                             it->second.desc.entryPoint +
                             it->second.desc.target;
            m_pathToHandle.erase(key);
        }

        printf("ShaderManager: Destroyed shader %s (handle: %u)\n",
               it->second.desc.debugName.c_str(), handle);
        m_shaders.erase(it);
    }
}

void ShaderManager::CheckForModifiedShaders() {
    for (auto& [handle, entry] : m_shaders) {
        if (entry.desc.filePath.empty()) continue; // Skip source-based shaders

        auto currentModTime = GetFileModTime(entry.cacheInfo.sourceFile);
        if (currentModTime > entry.cacheInfo.sourceModTime) {
            printf("ShaderManager: Detected modification in %s, reloading...\n",
                   entry.cacheInfo.sourceFile.c_str());
            ReloadShader(handle);
        }
    }
}

bool ShaderManager::ReloadShader(ShaderHandle handle) {
    auto it = m_shaders.find(handle);
    if (it == m_shaders.end() || it->second.desc.filePath.empty()) {
        return false;
    }

    auto& entry = it->second;
    auto newShader = std::make_unique<Shader>();

    // Update cache info
    entry.cacheInfo.sourceModTime = GetFileModTime(entry.cacheInfo.sourceFile);

    if (!newShader->InitializeFromFile(entry.cacheInfo.sourceFile,
                                      entry.desc.entryPoint,
                                      entry.desc.target,
                                      entry.desc.debugName)) {
        printf("ShaderManager: Failed to reload shader %s\n", entry.desc.debugName.c_str());
        return false;
    }

    // Update cache
    if (SaveToCache(entry.cacheInfo.cacheFile, newShader.get())) {
        entry.cacheInfo.cacheModTime = GetFileModTime(entry.cacheInfo.cacheFile);
    }

    entry.shader = std::move(newShader);

    printf("ShaderManager: Successfully reloaded shader %s\n", entry.desc.debugName.c_str());
    return true;
}

void ShaderManager::Clear() {
    printf("ShaderManager: Clearing %zu shaders\n", m_shaders.size());
    m_shaders.clear();
    m_pathToHandle.clear();
    m_nextHandle = 1;
}

bool ShaderManager::IsValidHandle(ShaderHandle handle) const {
    return handle != INVALID_SHADER_HANDLE && m_shaders.find(handle) != m_shaders.end();
}

void ShaderManager::PrintShaderInfo() const {
    printf("ShaderManager: %zu shaders loaded:\n", m_shaders.size());
    for (const auto& [handle, entry] : m_shaders) {
        printf("  Handle %u: %s (%s, %s)\n",
               handle,
               entry.desc.debugName.c_str(),
               entry.desc.entryPoint.c_str(),
               entry.desc.target.c_str());
    }
}

ShaderHandle ShaderManager::GenerateHandle() {
    return m_nextHandle++;
}

std::filesystem::file_time_type ShaderManager::GetFileModTime(const std::string& filePath) {
    try {
        return std::filesystem::last_write_time(filePath);
    }
    catch (const std::filesystem::filesystem_error&) {
        return {};
    }
}

std::string ShaderManager::GenerateCacheFileName(const std::string& filePath,
                                                const std::string& entryPoint,
                                                const std::string& target) {
    // Create a unique filename based on source file, entry point, and target
    // Example: "verts_VSMain_vs_5_1.bin"
    std::string baseName = std::filesystem::path(filePath).stem().string();
    return baseName + "_" + entryPoint + "_" + target + ".bin";
}

bool ShaderManager::LoadFromCache(const std::string& cacheFile, Shader* shader, const std::string& debugName) {
    auto data = FileReader::ReadFileBytes(cacheFile);
    if (data.empty()) {
        return false;
    }

    // Create a blob from the cached data
    ComPtr<ID3DBlob> blob;
    HRESULT hr = D3DCreateBlob(data.size(), &blob);
    if (FAILED(hr)) {
        printf("ShaderManager: Failed to create blob for cached shader %s\n", debugName.c_str());
        return false;
    }

    memcpy(blob->GetBufferPointer(), data.data(), data.size());

    // Create shader directly from blob (bypass compilation)
    return shader->InitializeFromBlob(blob.Get(), debugName);
}

bool ShaderManager::SaveToCache(const std::string& cacheFile, const Shader* shader) {
    ID3DBlob* blob = shader->GetShaderBlob();
    if (!blob) {
        return false;
    }

    std::ofstream file(cacheFile, std::ios::binary);
    if (!file.is_open()) {
        printf("ShaderManager: Failed to open cache file %s for writing\n", cacheFile.c_str());
        return false;
    }

    file.write(static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());
    return file.good();
}

bool ShaderManager::IsCacheValid(const ShaderCacheInfo& cacheInfo) {
    // Cache is valid if:
    // 1. Cache file exists
    // 2. Cache file is newer than source file
    return std::filesystem::exists(cacheInfo.cacheFile) &&
           cacheInfo.cacheModTime >= cacheInfo.sourceModTime;
}
