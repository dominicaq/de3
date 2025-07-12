#pragma once

#include "../resources/RenderTypes.h"

struct MeshRenderer {
    MeshHandle meshIndex = INVALID_MESH_HANDLE;
    MaterialHandle materialIndex = INVALID_MATERIAL_HANDLE;
};