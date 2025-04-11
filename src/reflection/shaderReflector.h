#pragma once

#include "api/vulkan/shader.h"

namespace aph
{
/**
 * Represents the layout of shader resources within a single descriptor set
 */
struct ShaderLayout
{
    // Resource mask bits indicating which bindings are used for each resource type
    std::bitset<VULKAN_NUM_BINDINGS> sampledImageMask       = {};
    std::bitset<VULKAN_NUM_BINDINGS> storageImageMask       = {};
    std::bitset<VULKAN_NUM_BINDINGS> uniformBufferMask      = {};
    std::bitset<VULKAN_NUM_BINDINGS> storageBufferMask      = {};
    std::bitset<VULKAN_NUM_BINDINGS> sampledTexelBufferMask = {};
    std::bitset<VULKAN_NUM_BINDINGS> storageTexelBufferMask = {};
    std::bitset<VULKAN_NUM_BINDINGS> inputAttachmentMask    = {};
    std::bitset<VULKAN_NUM_BINDINGS> samplerMask            = {};
    std::bitset<VULKAN_NUM_BINDINGS> separateImageMask      = {};
    std::bitset<VULKAN_NUM_BINDINGS> fpMask                 = {};

    // Array size for each binding
    uint8_t arraySize[VULKAN_NUM_BINDINGS] = {};

    // Special constant indicating an unsized array (bindless)
    static constexpr uint32_t UNSIZED_ARRAY = 0xff;
};

/**
 * Represents the state of a vertex attribute
 */
struct VertexAttribState
{
    uint32_t binding;
    Format format;
    uint32_t offset;
    uint32_t size;
};

/**
 * Contains the complete resource layout for a single shader stage
 */
struct ResourceLayout
{
    ShaderLayout layouts[VULKAN_NUM_DESCRIPTOR_SETS]              = {};
    VertexAttribState vertexAttributes[VULKAN_NUM_VERTEX_ATTRIBS] = {};

    std::bitset<VULKAN_NUM_VERTEX_ATTRIBS> inputMask              = {}; // Mask of active input attributes
    std::bitset<VULKAN_NUM_RENDER_TARGETS> outputMask             = {}; // Mask of active output attributes
    std::bitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS> specConstantMask = {}; // Mask of active specialization constants
    std::bitset<VULKAN_NUM_DESCRIPTOR_SETS> bindlessSetMask = {}; // Mask of descriptor sets using bindless resources
    uint32_t pushConstantSize                               = 0; // Size of push constant block in bytes
};

/**
 * Represents the combined resource layout across all shader stages in a pipeline
 */
struct CombinedResourceLayout
{
    /**
     * Information about a specific descriptor set across all stages
     */
    struct SetInfo
    {
        ShaderLayout shaderLayout                               = {};
        ShaderStageFlags stagesForBindings[VULKAN_NUM_BINDINGS] = {}; // Which shader stages use each binding
        ShaderStageFlags stagesForSets                          = {}; // Which shader stages use this set
    };

    std::array<SetInfo, VULKAN_NUM_DESCRIPTOR_SETS> setInfos            = {};
    std::array<VertexAttribState, VULKAN_NUM_VERTEX_ATTRIBS> vertexAttr = {};

    PushConstantRange pushConstantRange = {};

    std::bitset<VULKAN_NUM_VERTEX_ATTRIBS> attributeMask      = {}; // Mask of active vertex attributes
    std::bitset<VULKAN_NUM_RENDER_TARGETS> renderTargetMask   = {}; // Mask of active render targets
    std::bitset<VULKAN_NUM_DESCRIPTOR_SETS> descriptorSetMask = {}; // Mask of active descriptor sets
    std::bitset<VULKAN_NUM_DESCRIPTOR_SETS> bindlessDescriptorSetMask =
        {}; // Mask of descriptor sets using bindless resources
    std::bitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS> combinedSpecConstantMask =
        {}; // Combined mask of all specialization constants

    HashMap<ShaderStage, std::bitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS>> specConstantMask =
        {}; // Per-stage specialization constants
};

/**
 * Options for the reflection process
 */
struct ReflectionOptions
{
    bool extractInputAttributes  = true; // Extract vertex shader input attributes
    bool extractOutputAttributes = true; // Extract fragment shader output attributes
    bool extractPushConstants    = true; // Extract push constant blocks
    bool extractSpecConstants    = true; // Extract specialization constants
    bool validateBindings        = true; // Validate bindings for conflicts

    // Shader reflection caching
    bool enableCaching    = false; // Whether to use shader reflection caching
    std::string cachePath = ""; // Path to cache file (empty to disable)
};

/**
 * Request parameters for shader reflection
 */
struct ReflectRequest
{
    SmallVector<vk::Shader*> shaders;
    ReflectionOptions options                   = {};
};

/**
 * Container for descriptor resources needed by a pipeline
 */
struct DescriptorResourceInfo
{
    SmallVector<::vk::DescriptorSetLayoutBinding> bindings;
    SmallVector<::vk::DescriptorPoolSize> poolSizes;
};

/**
 * Results of the shader reflection process
 */
struct ReflectionResult
{
    VertexInput vertexInput;
    CombinedResourceLayout resourceLayout;
    std::array<DescriptorResourceInfo, VULKAN_NUM_DESCRIPTOR_SETS> descriptorResources;
    PushConstantRange pushConstantRange;
};

/**
 * Reflects shader information to extract resource layouts and requirements
 */
class ShaderReflector
{
public:
    /**
     * Constructs a shader reflector
     */
    ShaderReflector();

    /**
     * Destructor
     */
    ~ShaderReflector();

    /**
     * Performs reflection on the provided shaders
     * 
     * @param request The reflection request parameters
     * @return Results of the reflection process
     */
    ReflectionResult reflect(const ReflectRequest& request);

    /**
     * Utility method to get descriptor set layout bindings for a specific set
     * 
     * @param result The reflection result
     * @param set The descriptor set index
     * @return The layout bindings for the set
     */
    static SmallVector<::vk::DescriptorSetLayoutBinding> getLayoutBindings(const ReflectionResult& result,
                                                                           uint32_t set);

    /**
     * Utility method to get descriptor pool sizes for a specific set
     * 
     * @param result The reflection result
     * @param set The descriptor set index
     * @return The pool sizes for the set
     */
    static SmallVector<::vk::DescriptorPoolSize> getPoolSizes(const ReflectionResult& result, uint32_t set);

    /**
     * Check if a descriptor set uses bindless resources
     * 
     * @param result The reflection result
     * @param set The descriptor set index
     * @return True if the set uses bindless resources
     */
    static bool isBindlessSet(const ReflectionResult& result, uint32_t set);

    /**
     * Get all active descriptor sets
     * 
     * @param result The reflection result
     * @return A vector of active descriptor set indices
     */
    static SmallVector<uint32_t> getActiveDescriptorSets(const ReflectionResult& result);

private:
    // Private implementation to hide SPIRV-Cross details
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace aph
