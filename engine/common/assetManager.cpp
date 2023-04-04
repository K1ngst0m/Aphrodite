#include "assetManager.h"

namespace aph
{

static const std::filesystem::path assetDir = "assets";
static const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
static const std::filesystem::path textureDir = assetDir / "textures";
static const std::filesystem::path modelDir = assetDir / "models";
static const std::filesystem::path fontDir = assetDir / "fonts";

std::filesystem::path AssetManager::GetAssertDir()
{
    return assetDir;
}
std::filesystem::path AssetManager::GetShaderDir(ShaderAssetType type)
{
    switch(type)
    {
    case ShaderAssetType::GLSL:
        return glslShaderDir;
    case ShaderAssetType::HLSL:
        assert("shader not implemented.");
        return glslShaderDir;
    }
    return glslShaderDir;
}
std::filesystem::path AssetManager::GetTextureDir()
{
    return textureDir;
}
std::filesystem::path AssetManager::GetModelDir()
{
    return modelDir;
}
std::filesystem::path AssetManager::GetFontDir()
{
    return fontDir;
}
}  // namespace aph
