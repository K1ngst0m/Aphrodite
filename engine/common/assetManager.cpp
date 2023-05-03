#include "assetManager.h"

namespace aph::asset
{

static const std::filesystem::path assetDir      = "assets";
static const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
static const std::filesystem::path textureDir    = assetDir / "textures";
static const std::filesystem::path modelDir      = assetDir / "models";
static const std::filesystem::path fontDir       = assetDir / "fonts";

std::filesystem::path GetAssertDir()
{
    return assetDir;
}
std::filesystem::path GetShaderDir(ShaderType type)
{
    switch(type)
    {
    case ShaderType::GLSL:
        return glslShaderDir;
    case ShaderType::HLSL:
        assert("shader not implemented.");
        return glslShaderDir;
    }
    return glslShaderDir;
}
std::filesystem::path GetTextureDir()
{
    return textureDir;
}
std::filesystem::path GetModelDir()
{
    return modelDir;
}
std::filesystem::path GetFontDir()
{
    return fontDir;
}

bool saveBinaryfile(const char* path, const AssetFile& file)
{
    std::ofstream outfile;
    outfile.open(path, std::ios::binary | std::ios::out);

    outfile.write(file.type, 4);
    uint32_t version = file.version;
    // version
    outfile.write((const char*)&version, sizeof(uint32_t));

    // json length
    uint32_t length = file.json.size();
    outfile.write((const char*)&length, sizeof(uint32_t));

    // blob length
    uint32_t bloblength = file.binaryBlob.size();
    outfile.write((const char*)&bloblength, sizeof(uint32_t));

    // json stream
    outfile.write(file.json.data(), length);
    // blob data
    outfile.write(file.binaryBlob.data(), file.binaryBlob.size());

    outfile.close();

    return true;
}

bool loadBinaryfile(const char* path, AssetFile& outputFile)
{
    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if(!infile.is_open())
        return false;

    // move file cursor to beginning
    infile.seekg(0);

    infile.read(outputFile.type, 4);
    infile.read((char*)&outputFile.version, sizeof(uint32_t));

    uint32_t jsonlen = 0;
    infile.read((char*)&jsonlen, sizeof(uint32_t));

    uint32_t bloblen = 0;
    infile.read((char*)&bloblen, sizeof(uint32_t));

    outputFile.json.resize(jsonlen);
    infile.read(outputFile.json.data(), jsonlen);

    outputFile.binaryBlob.resize(bloblen);
    infile.read(outputFile.binaryBlob.data(), bloblen);

    return true;
}
}  // namespace aph::asset
