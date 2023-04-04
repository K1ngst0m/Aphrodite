#ifndef ASSETMANAGER_H_
#define ASSETMANAGER_H_

#include "common.h"

namespace aph {

enum class ShaderAssetType {
    GLSL,
    HLSL,
};

class AssetManager {
public:
    static std::filesystem::path GetAssertDir();
    static std::filesystem::path GetShaderDir(ShaderAssetType type);
    static std::filesystem::path GetTextureDir();
    static std::filesystem::path GetModelDir();
    static std::filesystem::path GetFontDir();
};
} // namespace aph

#endif // ASSETMANAGER_H_
