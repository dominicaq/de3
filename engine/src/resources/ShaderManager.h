#pragma once
#include "../renderer/dx12/core/DX12Common.h"
#include "../renderer/dx12/resources/Shader.h"
#include "RenderTypes.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>

using Microsoft::WRL::ComPtr;

struct ShaderDesc {
    std::string filePath;
    std::string entryPoint;
    std::string target;
    std::string debugName;
};

struct ShaderCacheInfo {
    std::string sourceFile;
    std::string cacheFile;
    std::filesystem::file_time_type sourceModTime;
    std::filesystem::file_time_type cacheModTime;
};

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    // Non-copyable
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    // Set shader directories
    void SetShaderDirectories(const std::string& sourceDir, const std::string& cacheDir) {
        m_sourceDir = sourceDir;
        m_cacheDir = cacheDir;

        // Create cache directory if it doesn't exist
        std::filesystem::create_directories(m_cacheDir);
    }

    // Create shader from file
    ShaderHandle CreateShaderFromFile(const std::string& filePath,
                                     const std::string& entryPoint,
                                     const std::string& target,
                                     const std::string& debugName);

    // Create shader from source string (for backwards compatibility)
    ShaderHandle CreateShaderFromSource(const std::string& source,
                                       const std::string& entryPoint,
                                       const std::string& target,
                                       const std::string& debugName);

    // Get shader by handle
    const Shader* GetShader(ShaderHandle handle) const;

    // Get or create shader (useful for caching)
    ShaderHandle GetOrCreateShader(const ShaderDesc& desc);

    // Remove shader by handle
    void DestroyShader(ShaderHandle handle);

    // Hot-reload support
    void CheckForModifiedShaders();
    bool ReloadShader(ShaderHandle handle);

    // Clear all shaders
    void Clear();

    // Check if handle is valid
    bool IsValidHandle(ShaderHandle handle) const;

    // Debug info
    size_t GetShaderCount() const { return m_shaders.size(); }
    void PrintShaderInfo() const;

private:
    struct ShaderEntry {
        std::unique_ptr<Shader> shader;
        ShaderDesc desc;
        ShaderCacheInfo cacheInfo;
    };

    ShaderHandle GenerateHandle();
    std::filesystem::file_time_type GetFileModTime(const std::string& filePath);
    std::string GenerateCacheFileName(const std::string& filePath,
                                     const std::string& entryPoint,
                                     const std::string& target);
    bool LoadFromCache(const std::string& cacheFile, Shader* shader, const std::string& debugName);
    bool SaveToCache(const std::string& cacheFile, const Shader* shader);
    bool IsCacheValid(const ShaderCacheInfo& cacheInfo);

    std::unordered_map<ShaderHandle, ShaderEntry> m_shaders;
    std::unordered_map<std::string, ShaderHandle> m_pathToHandle; // For deduplication
    ShaderHandle m_nextHandle = 1;

    std::string m_sourceDir = "shaders/hlsl/";
    std::string m_cacheDir = "shaders/compiled/";
};
