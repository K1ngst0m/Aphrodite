#ifndef ASSETMANAGER_H_
#define ASSETMANAGER_H_

#include "common.h"

namespace aph::asset
{

enum class AssetType
{

};

enum class ShaderType
{
    GLSL,
    HLSL,
};

struct AssetFile
{
    char              type[4];
    int               version;
    std::string       json;
    std::vector<char> binaryBlob;
};

bool saveBinaryfile(const char* path, const AssetFile& file);
bool loadBinaryfile(const char* path, AssetFile& outputFile);

enum class TextureFormat : uint32_t
{
    Unknown = 0,
    RGBA8
};

// struct TextureInfo {
//     uint64_t textureSize;
//     TextureFormat textureFormat;
//     CompressionMode compressionMode;
//     uint32_t pixelsize[3];
//     std::string originalFile;
// };

// //parses the texture metadata from an asset file
// TextureInfo readTextureInfo(AssetFile* file);
// void unpackTexture(TextureInfo* info, const char* sourcebuffer, size_t sourceSize, char* destination);
// AssetFile packTexture(TextureInfo* info, void* pixelData);

std::filesystem::path GetAssertDir();
std::filesystem::path GetShaderDir(ShaderType type);
std::filesystem::path GetTextureDir();
std::filesystem::path GetModelDir();
std::filesystem::path GetFontDir();
}  // namespace aph::asset

#endif  // ASSETMANAGER_H_
