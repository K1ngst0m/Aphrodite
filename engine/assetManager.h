#ifndef ASSETMANAGER_H_
#define ASSETMANAGER_H_

#include "common.h"

namespace vkl {

enum class ShaderAssetType {
    GLSL,
    HLSL,
};

class AssetManager {
public:
    static const std::filesystem::path &GetAssertDir();
    static const std::filesystem::path &GetShaderDir(ShaderAssetType type);
    static const std::filesystem::path &GetTextureDir();
    static const std::filesystem::path &GetModelDir();
};
} // namespace vkl

#endif // ASSETMANAGER_H_
