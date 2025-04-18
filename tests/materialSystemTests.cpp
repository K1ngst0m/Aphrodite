#include "material/materialRegistry.h"
#include "material/parameterLayout.h"
#include "material/typeUtils.h"
#include "common/logger.h"
#include "global/globalManager.h"

#include <catch2/catch_all.hpp>

using namespace aph;

TEST_CASE("TypeUtils functionality", "[material][typeutils]")
{
    SECTION("Data type sizes")
    {
        REQUIRE(TypeUtils::getTypeSize(DataType::eFloat) == 4);
        REQUIRE(TypeUtils::getTypeSize(DataType::eVec2) == 8);
        REQUIRE(TypeUtils::getTypeSize(DataType::eVec3) == 12);
        REQUIRE(TypeUtils::getTypeSize(DataType::eVec4) == 16);
        REQUIRE(TypeUtils::getTypeSize(DataType::eMat4) == 64);
    }
    
    SECTION("Type classification")
    {
        REQUIRE(TypeUtils::isScalarType(DataType::eFloat) == true);
        REQUIRE(TypeUtils::isScalarType(DataType::eVec3) == false);
        
        REQUIRE(TypeUtils::isVectorType(DataType::eVec3) == true);
        REQUIRE(TypeUtils::isVectorType(DataType::eFloat) == false);
        
        REQUIRE(TypeUtils::isMatrixType(DataType::eMat3) == true);
        REQUIRE(TypeUtils::isMatrixType(DataType::eVec3) == false);
        
        REQUIRE(TypeUtils::isTextureType(DataType::eTexture2D) == true);
        REQUIRE(TypeUtils::isTextureType(DataType::eFloat) == false);
    }
    
    SECTION("Type alignment")
    {
        REQUIRE(TypeUtils::getTypeAlignment(DataType::eFloat) == 4);
        REQUIRE(TypeUtils::getTypeAlignment(DataType::eVec2) == 8);
        REQUIRE(TypeUtils::getTypeAlignment(DataType::eVec3) == 16);
        REQUIRE(TypeUtils::getTypeAlignment(DataType::eVec4) == 16);
    }
}

TEST_CASE("MaterialTemplate functionality", "[material][template]")
{
    SECTION("Create and configure template")
    {
        auto testTemplate = std::make_unique<MaterialTemplate>(
            "TestMaterial",
            MaterialDomain::eOpaque,
            MaterialFeatureBits::eStandard
        );
        
        REQUIRE(testTemplate->getName() == "TestMaterial");
        REQUIRE(testTemplate->getDomain() == MaterialDomain::eOpaque);
        REQUIRE(testTemplate->getFeatureFlags() == MaterialFeatureBits::eStandard);
        
        // Initially no parameters
        REQUIRE(testTemplate->getParameterLayout().empty());
        
        // Add parameters
        MaterialParameterDesc colorParam;
        colorParam.name = "color";
        colorParam.type = DataType::eVec4;
        colorParam.size = TypeUtils::getTypeSize(DataType::eVec4);
        colorParam.isTexture = false;
        testTemplate->addParameter(colorParam);
        
        REQUIRE(testTemplate->getParameterLayout().size() == 1);
        REQUIRE(testTemplate->getParameterLayout()[0].name == "color");
        
        // Duplicate parameter should be ignored
        testTemplate->addParameter(colorParam);
        REQUIRE(testTemplate->getParameterLayout().size() == 1);
        
        // Add different parameter
        MaterialParameterDesc roughnessParam;
        roughnessParam.name = "roughness";
        roughnessParam.type = DataType::eFloat;
        roughnessParam.size = TypeUtils::getTypeSize(DataType::eFloat);
        roughnessParam.isTexture = false;
        testTemplate->addParameter(roughnessParam);
        
        REQUIRE(testTemplate->getParameterLayout().size() == 2);
    }
}

TEST_CASE("MaterialRegistry functionality", "[material][registry]")
{
    // Initialize GlobalManager for testing
    GlobalManager::instance().initialize();
    
    // Create a MaterialRegistry for testing using the static Create method
    auto registryResult = MaterialRegistry::Create();
    REQUIRE(registryResult.success());
    auto* registry = registryResult.value();
    
    SECTION("Register and find templates")
    {
        // Should already have built-in templates from Create()
        REQUIRE_FALSE(registry->getTemplates().empty());
        
        // Built-in StandardPBR template should exist
        auto standardPbrResult = registry->findTemplate("StandardPBR");
        REQUIRE(standardPbrResult.success());
        REQUIRE(standardPbrResult.value() != nullptr);
        
        auto notFoundResult = registry->findTemplate("NonExistentTemplate");
        REQUIRE_FALSE(notFoundResult.success());
        
        // Register template
        auto testTemplate = std::make_unique<MaterialTemplate>(
            "TestMaterial",
            MaterialDomain::eOpaque,
            MaterialFeatureBits::eStandard
        );
        
        auto registerResult = registry->registerTemplate(std::move(testTemplate));
        REQUIRE(registerResult.success());
        
        auto rawPtr = registerResult.value();
        REQUIRE(rawPtr != nullptr);
        
        auto findResult = registry->findTemplate("TestMaterial");
        REQUIRE(findResult.success());
        REQUIRE(findResult.value() == rawPtr);
        
        // Register null template should fail
        auto nullResult = registry->registerTemplate(nullptr);
        REQUIRE_FALSE(nullResult.success());
    }
    
    // Clean up registry
    MaterialRegistry::Destroy(registry);
    
    // Clean up
    GlobalManager::instance().shutdown();
}

TEST_CASE("ParameterLayout functionality", "[material][parameters]")
{
    SECTION("Parameter alignment calculation")
    {
        REQUIRE(ParameterLayout::calculateAlignedOffset(0, DataType::eFloat) == 0);
        REQUIRE(ParameterLayout::calculateAlignedOffset(2, DataType::eFloat) == 4);
        REQUIRE(ParameterLayout::calculateAlignedOffset(4, DataType::eVec2) == 8);
        REQUIRE(ParameterLayout::calculateAlignedOffset(8, DataType::eVec3) == 16);
        REQUIRE(ParameterLayout::calculateAlignedOffset(20, DataType::eVec4) == 32);
    }
    
    SECTION("Parameter separation")
    {
        SmallVector<MaterialParameterDesc> params;
        
        MaterialParameterDesc floatParam;
        floatParam.name = "scalar";
        floatParam.type = DataType::eFloat;
        floatParam.isTexture = false;
        params.push_back(floatParam);
        
        MaterialParameterDesc texParam;
        texParam.name = "texture";
        texParam.type = DataType::eTexture2D;
        texParam.isTexture = true;
        params.push_back(texParam);
        
        SmallVector<MaterialParameterDesc> uniformParams;
        SmallVector<MaterialParameterDesc> textureParams;
        
        ParameterLayout::separateParameters(params, uniformParams, textureParams);
        
        REQUIRE(uniformParams.size() == 1);
        REQUIRE(textureParams.size() == 1);
        REQUIRE(uniformParams[0].name == "scalar");
        REQUIRE(textureParams[0].name == "texture");
    }
    
    SECTION("Layout generation")
    {
        auto testTemplate = std::make_unique<MaterialTemplate>(
            "TestMaterial",
            MaterialDomain::eOpaque,
            MaterialFeatureBits::eStandard
        );
        
        // Add parameters in arbitrary order
        MaterialParameterDesc vec4Param;
        vec4Param.name = "color";
        vec4Param.type = DataType::eVec4;
        vec4Param.size = TypeUtils::getTypeSize(DataType::eVec4);
        vec4Param.isTexture = false;
        testTemplate->addParameter(vec4Param);
        
        MaterialParameterDesc floatParam;
        floatParam.name = "roughness";
        floatParam.type = DataType::eFloat;
        floatParam.size = TypeUtils::getTypeSize(DataType::eFloat);
        floatParam.isTexture = false;
        testTemplate->addParameter(floatParam);
        
        MaterialParameterDesc texParam;
        texParam.name = "albedoMap";
        texParam.type = DataType::eTexture2D;
        texParam.size = TypeUtils::getTypeSize(DataType::eTexture2D);
        texParam.isTexture = true;
        testTemplate->addParameter(texParam);
        
        auto layout = ParameterLayout::generateLayout(testTemplate.get());
        
        // Layout should have aligned offsets
        REQUIRE(layout.size() == 3);
        
        // Find each parameter by name
        auto colorIt = std::find_if(layout.begin(), layout.end(), 
                                   [](const MaterialParameterDesc& p) { return p.name == "color"; });
        auto roughnessIt = std::find_if(layout.begin(), layout.end(), 
                                       [](const MaterialParameterDesc& p) { return p.name == "roughness"; });
        auto albedoMapIt = std::find_if(layout.begin(), layout.end(), 
                                       [](const MaterialParameterDesc& p) { return p.name == "albedoMap"; });
        
        REQUIRE(colorIt != layout.end());
        REQUIRE(roughnessIt != layout.end());
        REQUIRE(albedoMapIt != layout.end());
        
        // Vec4 should be aligned to 16
        REQUIRE(colorIt->offset % 16 == 0);
        
        // Calculate total size
        uint32_t totalSize = ParameterLayout::calculateTotalSize(layout);
        REQUIRE(totalSize % 16 == 0); // UBO requires 16-byte alignment
    }
}

TEST_CASE("Material functionality", "[material][instance]")
{
    // Initialize GlobalManager for testing
    GlobalManager::instance().initialize();
    
    // Create a MaterialRegistry for testing
    auto registryResult = MaterialRegistry::Create();
    REQUIRE(registryResult.success());
    auto* registry = registryResult.value();
    
    SECTION("Material instance creation and parameter setting")
    {
        // Get the built-in StandardPBR template
        auto templateResult = registry->findTemplate("StandardPBR");
        REQUIRE(templateResult.success());
        
        // Create a material from the template
        auto materialResult = registry->createMaterial(templateResult.value());
        REQUIRE(materialResult.success());
        auto* material = materialResult.value();
        
        // Set some parameters
        float roughness = 0.75f;
        REQUIRE(material->setFloat("roughness", roughness).success());
        
        float metallic = 0.5f;
        REQUIRE(material->setFloat("metallic", metallic).success());
        
        float baseColor[4] = {0.8f, 0.4f, 0.2f, 1.0f};
        REQUIRE(material->setVec4("baseColor", baseColor).success());
        
        // Set a texture
        REQUIRE(material->setTexture("albedoMap", "textures/test_albedo.tex").success());
        
        // Verify parameter storage is working
        REQUIRE(material->getParameterData() != nullptr);
        REQUIRE(material->getParameterDataSize() > 0);
        
        // Verify dirty flag behavior
        REQUIRE(material->isDirty());
        material->updateGPUResources();
        REQUIRE_FALSE(material->isDirty());
        
        // Verify texture bindings
        auto& textures = material->getTextureBindings();
        REQUIRE(textures.size() > 0);
        auto it = textures.find("albedoMap");
        REQUIRE(it != textures.end());
        REQUIRE(it->second == "textures/test_albedo.tex");
        
        // Fail cases
        REQUIRE_FALSE(material->setFloat("nonexistent", 1.0f).success());
        REQUIRE_FALSE(material->setVec4("roughness", baseColor).success()); // Type mismatch
        
        // Explicitly free the material
        registry->freeMaterial(material);
    }
    
    SECTION("MaterialAsset serialization")
    {
        // Get the built-in StandardPBR template
        auto templateResult = registry->findTemplate("StandardPBR");
        REQUIRE(templateResult.success());
        
        // Create a material from the template
        auto materialResult = registry->createMaterial(templateResult.value());
        REQUIRE(materialResult.success());
        auto* material = materialResult.value();
        
        // Set some parameters
        REQUIRE(material->setFloat("roughness", 0.75f).success());
        REQUIRE(material->setFloat("metallic", 0.5f).success());
        
        float baseColor[4] = {0.8f, 0.4f, 0.2f, 1.0f};
        REQUIRE(material->setVec4("baseColor", baseColor).success());
        
        // Set a texture
        REQUIRE(material->setTexture("albedoMap", "textures/test_albedo.tex").success());
        
        {
            // Create a material asset with registry reference for proper cleanup
            MaterialAsset asset(material, registry);
            
            // Verify material access
            REQUIRE(asset.isLoaded());
            REQUIRE(asset.getMaterial() == material);
            
            // For non-file-system dependent test, we can check the TOML serialization
            std::string tomlData = asset.serializeToTOML();
            REQUIRE_FALSE(tomlData.empty());
            
            // Check for expected TOML contents
            REQUIRE(tomlData.find("template = \"StandardPBR\"") != std::string::npos);
            REQUIRE(tomlData.find("baseColor") != std::string::npos);
            REQUIRE(tomlData.find("roughness") != std::string::npos);
            REQUIRE(tomlData.find("metallic") != std::string::npos);
            REQUIRE(tomlData.find("albedoMap") != std::string::npos);
        }
        
        // No need to manually free material as asset should handle it
    }
    
    // Clean up
    MaterialRegistry::Destroy(registry);
    GlobalManager::instance().shutdown();
} 