#pragma once

#include <string>

/* =============================================================================
   Definitions are used as IO request for their respective managers to load a
   resource. Descriptions are meant to be immutable once created.
   ============================================================================= */

struct MeshDescription {
    const std::string fpath;
};

struct ShaderDescription {
    const std::string name;
    const std::string shaderSrcPath;
    const std::string entryPoint = "VSMain";
    const std::string target = "vs_5_0";
    const bool enableDepth = false;
};

struct TextureDescription {
    const std::string fpath;
};

// PBR Material
// MRAO = Metalness-Roughness-Ambient Occlusion
struct MaterialDescription {
    const std::string albedoPath;
    const std::string normalPath;
    const std::string mraoPath;
    const std::string emissvePath;
    const std::string heightPath;
};