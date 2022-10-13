#include "assetManager.h"

namespace vkl {

static const std::filesystem::path assetDir      = "assets";
static const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
static const std::filesystem::path textureDir    = assetDir / "textures";
static const std::filesystem::path modelDir      = assetDir / "models";
static const std::filesystem::path fontDir       = assetDir / "fonts";

const std::filesystem::path &AssetManager::GetAssertDir() {
    return assetDir;
}
const std::filesystem::path &AssetManager::GetShaderDir(ShaderAssetType type) {
    switch (type) {
    case ShaderAssetType::GLSL:
        return glslShaderDir;
    case ShaderAssetType::HLSL:
        assert("shader not implemented.");
        return glslShaderDir;
    }
    return glslShaderDir;
}
const std::filesystem::path &AssetManager::GetTextureDir() {
    return textureDir;
}
const std::filesystem::path &AssetManager::GetModelDir() {
    return modelDir;
}
const std::filesystem::path &AssetManager::GetFontDir() {
    return fontDir;
}
} // namespace vkl
