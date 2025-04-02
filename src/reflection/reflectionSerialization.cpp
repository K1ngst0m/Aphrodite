#include "reflectionSerialization.h"
#include "common/profiler.h"

namespace aph::reflection
{
// Helper for serializing shader stages
toml::array serializeShaderStages(ShaderStageFlags stages)
{
    toml::array result;

    if (stages & ShaderStage::VS)
        result.push_back("vs");
    if (stages & ShaderStage::TCS)
        result.push_back("tcs");
    if (stages & ShaderStage::TES)
        result.push_back("tes");
    if (stages & ShaderStage::GS)
        result.push_back("gs");
    if (stages & ShaderStage::FS)
        result.push_back("fs");
    if (stages & ShaderStage::CS)
        result.push_back("cs");
    if (stages & ShaderStage::TS)
        result.push_back("ts");
    if (stages & ShaderStage::MS)
        result.push_back("ms");

    return result;
}

// Helper for deserializing shader stages
ShaderStageFlags deserializeShaderStages(const toml::array* arr)
{
    ShaderStageFlags result;
    if (!arr)
        return result;

    for (const auto& val : *arr)
    {
        if (val.is_string())
        {
            std::string stage = val.as_string()->get();
            if (stage == "vs")
                result |= ShaderStage::VS;
            else if (stage == "tcs")
                result |= ShaderStage::TCS;
            else if (stage == "tes")
                result |= ShaderStage::TES;
            else if (stage == "gs")
                result |= ShaderStage::GS;
            else if (stage == "fs")
                result |= ShaderStage::FS;
            else if (stage == "cs")
                result |= ShaderStage::CS;
            else if (stage == "ts")
                result |= ShaderStage::TS;
            else if (stage == "ms")
                result |= ShaderStage::MS;
        }
    }

    return result;
}

// Serializes a vertex attribute state
toml::table serializeVertexAttribState(const VertexAttribState& attr)
{
    toml::table result;
    result.insert("binding", static_cast<int64_t>(attr.binding));
    result.insert("format", static_cast<int64_t>(attr.format));
    result.insert("offset", static_cast<int64_t>(attr.offset));
    result.insert("size", static_cast<int64_t>(attr.size));
    return result;
}

// Deserializes a vertex attribute state
VertexAttribState deserializeVertexAttribState(const toml::table* table)
{
    VertexAttribState result{};
    if (!table)
        return result;

    if (auto binding = table->get("binding"))
        result.binding = static_cast<uint32_t>(binding->as_integer()->get());

    if (auto format = table->get("format"))
        result.format = static_cast<Format>(format->as_integer()->get());

    if (auto offset = table->get("offset"))
        result.offset = static_cast<uint32_t>(offset->as_integer()->get());

    if (auto size = table->get("size"))
        result.size = static_cast<uint32_t>(size->as_integer()->get());

    return result;
}

// Serializes a push constant range
toml::table serializePushConstantRange(const PushConstantRange& range)
{
    toml::table result;
    result.insert("stages", serializeShaderStages(range.stageFlags));
    result.insert("offset", static_cast<int64_t>(range.offset));
    result.insert("size", static_cast<int64_t>(range.size));
    return result;
}

// Deserializes a push constant range
PushConstantRange deserializePushConstantRange(const toml::table* table)
{
    PushConstantRange result{};
    if (!table)
        return result;

    if (auto stages = table->get_as<toml::array>("stages"))
        result.stageFlags = deserializeShaderStages(&*stages);

    if (auto offset = table->get("offset"))
        result.offset = static_cast<uint32_t>(offset->as_integer()->get());

    if (auto size = table->get("size"))
        result.size = static_cast<uint32_t>(size->as_integer()->get());

    return result;
}

toml::table serializeShaderLayout(const ShaderLayout& layout)
{
    toml::table result;

    // Serialize masks
    result.insert("sampledImageMask", serializeBitset(layout.sampledImageMask));
    result.insert("storageImageMask", serializeBitset(layout.storageImageMask));
    result.insert("uniformBufferMask", serializeBitset(layout.uniformBufferMask));
    result.insert("storageBufferMask", serializeBitset(layout.storageBufferMask));
    result.insert("sampledTexelBufferMask", serializeBitset(layout.sampledTexelBufferMask));
    result.insert("storageTexelBufferMask", serializeBitset(layout.storageTexelBufferMask));
    result.insert("inputAttachmentMask", serializeBitset(layout.inputAttachmentMask));
    result.insert("samplerMask", serializeBitset(layout.samplerMask));
    result.insert("separateImageMask", serializeBitset(layout.separateImageMask));
    result.insert("fpMask", serializeBitset(layout.fpMask));
    result.insert("immutableSamplerMask", serializeBitset(layout.immutableSamplerMask));

    // Serialize array sizes
    toml::array arraySizes;
    for (uint32_t i = 0; i < VULKAN_NUM_BINDINGS; i++)
    {
        if (layout.arraySize[i] > 0)
        {
            toml::table entry;
            entry.insert("binding", static_cast<int64_t>(i));
            entry.insert("size", static_cast<int64_t>(layout.arraySize[i]));
            arraySizes.push_back(std::move(entry));
        }
    }
    result.insert("arraySizes", std::move(arraySizes));

    return result;
}

ShaderLayout deserializeShaderLayout(const toml::table* table)
{
    ShaderLayout result{};
    if (!table)
        return result;

    // Deserialize masks
    if (auto mask = table->get_as<toml::array>("sampledImageMask"))
        result.sampledImageMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("storageImageMask"))
        result.storageImageMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("uniformBufferMask"))
        result.uniformBufferMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("storageBufferMask"))
        result.storageBufferMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("sampledTexelBufferMask"))
        result.sampledTexelBufferMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("storageTexelBufferMask"))
        result.storageTexelBufferMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("inputAttachmentMask"))
        result.inputAttachmentMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("samplerMask"))
        result.samplerMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("separateImageMask"))
        result.separateImageMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("fpMask"))
        result.fpMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    if (auto mask = table->get_as<toml::array>("immutableSamplerMask"))
        result.immutableSamplerMask = deserializeBitset<VULKAN_NUM_BINDINGS>(&*mask);

    // Deserialize array sizes
    if (auto arraySizes = table->get_as<toml::array>("arraySizes"))
    {
        for (const auto& entry : *arraySizes)
        {
            if (auto entryTable = entry.as_table())
            {
                if (auto binding = entryTable->get("binding"))
                {
                    auto bindingIndex = binding->as_integer()->get();

                    if (auto size = entryTable->get("size"))
                    {
                        if (bindingIndex >= 0 && bindingIndex < VULKAN_NUM_BINDINGS)
                        {
                            result.arraySize[bindingIndex] = static_cast<uint8_t>(size->as_integer()->get());
                        }
                    }
                }
            }
        }
    }

    return result;
}

toml::table serializeResourceLayout(const ResourceLayout& layout)
{
    toml::table result;

    // Serialize descriptor set layouts
    toml::array descriptorSetLayouts;
    for (uint32_t i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        toml::table setLayout;
        setLayout.insert("index", static_cast<int64_t>(i));
        setLayout.insert("layout", serializeShaderLayout(layout.layouts[i]));
        descriptorSetLayouts.push_back(std::move(setLayout));
    }
    result.insert("descriptorSetLayouts", std::move(descriptorSetLayouts));

    // Serialize vertex attributes
    toml::array vertexAttributes;
    for (uint32_t i = 0; i < VULKAN_NUM_VERTEX_ATTRIBS; i++)
    {
        if (layout.inputMask.test(i))
        {
            toml::table attr;
            attr.insert("index", static_cast<int64_t>(i));
            attr.insert("attribute", serializeVertexAttribState(layout.vertexAttributes[i]));
            vertexAttributes.push_back(std::move(attr));
        }
    }
    result.insert("vertexAttributes", std::move(vertexAttributes));

    // Serialize masks and sizes
    result.insert("inputMask", serializeBitset(layout.inputMask));
    result.insert("outputMask", serializeBitset(layout.outputMask));
    result.insert("specConstantMask", serializeBitset(layout.specConstantMask));
    result.insert("bindlessSetMask", serializeBitset(layout.bindlessSetMask));
    result.insert("pushConstantSize", static_cast<int64_t>(layout.pushConstantSize));

    return result;
}

ResourceLayout deserializeResourceLayout(const toml::table* table)
{
    ResourceLayout result{};
    if (!table)
        return result;

    // Deserialize descriptor set layouts
    if (auto descriptorSetLayouts = table->get_as<toml::array>("descriptorSetLayouts"))
    {
        for (const auto& entry : *descriptorSetLayouts)
        {
            if (auto entryTable = entry.as_table())
            {
                if (auto index = entryTable->get("index"))
                {
                    auto setIndex = index->as_integer()->get();

                    if (auto layout = entryTable->get_as<toml::table>("layout"))
                    {
                        if (setIndex >= 0 && setIndex < VULKAN_NUM_DESCRIPTOR_SETS)
                        {
                            result.layouts[setIndex] = deserializeShaderLayout(&*layout);
                        }
                    }
                }
            }
        }
    }

    // Deserialize vertex attributes
    if (auto vertexAttributes = table->get_as<toml::array>("vertexAttributes"))
    {
        for (const auto& entry : *vertexAttributes)
        {
            if (auto entryTable = entry.as_table())
            {
                if (auto index = entryTable->get("index"))
                {
                    auto attrIndex = index->as_integer()->get();

                    if (auto attribute = entryTable->get_as<toml::table>("attribute"))
                    {
                        if (attrIndex >= 0 && attrIndex < VULKAN_NUM_VERTEX_ATTRIBS)
                        {
                            result.vertexAttributes[attrIndex] = deserializeVertexAttribState(&*attribute);
                        }
                    }
                }
            }
        }
    }

    // Deserialize masks and sizes
    if (auto mask = table->get_as<toml::array>("inputMask"))
        result.inputMask = deserializeBitset<VULKAN_NUM_VERTEX_ATTRIBS>(&*mask);

    if (auto mask = table->get_as<toml::array>("outputMask"))
        result.outputMask = deserializeBitset<VULKAN_NUM_RENDER_TARGETS>(&*mask);

    if (auto mask = table->get_as<toml::array>("specConstantMask"))
        result.specConstantMask = deserializeBitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS>(&*mask);

    if (auto mask = table->get_as<toml::array>("bindlessSetMask"))
        result.bindlessSetMask = deserializeBitset<VULKAN_NUM_DESCRIPTOR_SETS>(&*mask);

    if (auto size = table->get("pushConstantSize"))
        result.pushConstantSize = static_cast<uint32_t>(size->as_integer()->get());

    return result;
}

toml::table serializeCombinedResourceLayout(const CombinedResourceLayout& layout)
{
    toml::table result;

    // Serialize descriptor set infos
    toml::array setInfos;
    for (uint32_t i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        if (layout.descriptorSetMask.test(i))
        {
            toml::table setInfo;
            setInfo.insert("index", static_cast<int64_t>(i));

            // Serialize shader layout
            setInfo.insert("shaderLayout", serializeShaderLayout(layout.setInfos[i].shaderLayout));

            // Serialize stages for bindings
            toml::array bindingStages;
            for (uint32_t binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                if (layout.setInfos[i].stagesForBindings[binding])
                {
                    toml::table entry;
                    entry.insert("binding", static_cast<int64_t>(binding));
                    entry.insert("stages", serializeShaderStages(layout.setInfos[i].stagesForBindings[binding]));
                    bindingStages.push_back(std::move(entry));
                }
            }
            setInfo.insert("bindingStages", std::move(bindingStages));

            // Serialize stages for set
            setInfo.insert("setStages", serializeShaderStages(layout.setInfos[i].stagesForSets));

            setInfos.push_back(std::move(setInfo));
        }
    }
    result.insert("setInfos", std::move(setInfos));

    // Serialize vertex attributes
    toml::array vertexAttributes;
    for (uint32_t i = 0; i < VULKAN_NUM_VERTEX_ATTRIBS; i++)
    {
        if (layout.attributeMask.test(i))
        {
            toml::table attr;
            attr.insert("index", static_cast<int64_t>(i));
            attr.insert("attribute", serializeVertexAttribState(layout.vertexAttr[i]));
            vertexAttributes.push_back(std::move(attr));
        }
    }
    result.insert("vertexAttributes", std::move(vertexAttributes));

    // Serialize push constant range
    result.insert("pushConstantRange", serializePushConstantRange(layout.pushConstantRange));

    // Serialize masks
    result.insert("attributeMask", serializeBitset(layout.attributeMask));
    result.insert("renderTargetMask", serializeBitset(layout.renderTargetMask));
    result.insert("descriptorSetMask", serializeBitset(layout.descriptorSetMask));
    result.insert("bindlessDescriptorSetMask", serializeBitset(layout.bindlessDescriptorSetMask));
    result.insert("combinedSpecConstantMask", serializeBitset(layout.combinedSpecConstantMask));

    // Serialize per-stage spec constants
    toml::table specConstantMasks;
    for (const auto& [stage, mask] : layout.specConstantMask)
    {
        std::string stageName = vk::utils::toString(stage);
        specConstantMasks.insert(stageName, serializeBitset(mask));
    }
    result.insert("specConstantMasks", std::move(specConstantMasks));

    return result;
}

CombinedResourceLayout deserializeCombinedResourceLayout(const toml::table* table)
{
    CombinedResourceLayout result{};
    if (!table)
        return result;

    // Deserialize descriptor set infos
    if (auto setInfos = table->get_as<toml::array>("setInfos"))
    {
        for (const auto& entry : *setInfos)
        {
            if (auto entryTable = entry.as_table())
            {
                if (auto index = entryTable->get("index"))
                {
                    auto setIndex = index->as_integer()->get();

                    if (setIndex >= 0 && setIndex < VULKAN_NUM_DESCRIPTOR_SETS)
                    {
                        // Deserialize shader layout
                        if (auto shaderLayout = entryTable->get_as<toml::table>("shaderLayout"))
                        {
                            result.setInfos[setIndex].shaderLayout = deserializeShaderLayout(&*shaderLayout);
                        }

                        // Deserialize stages for bindings
                        if (auto bindingStages = entryTable->get_as<toml::array>("bindingStages"))
                        {
                            for (const auto& bindingEntry : *bindingStages)
                            {
                                if (auto bindingTable = bindingEntry.as_table())
                                {
                                    if (auto binding = bindingTable->get("binding"))
                                    {
                                        auto bindingIndex = binding->as_integer()->get();

                                        if (bindingIndex >= 0 && bindingIndex < VULKAN_NUM_BINDINGS)
                                        {
                                            if (auto stages = bindingTable->get_as<toml::array>("stages"))
                                            {
                                                result.setInfos[setIndex].stagesForBindings[bindingIndex] =
                                                    deserializeShaderStages(&*stages);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Deserialize stages for set
                        if (auto setStages = entryTable->get_as<toml::array>("setStages"))
                        {
                            result.setInfos[setIndex].stagesForSets = deserializeShaderStages(&*setStages);
                        }

                        // Mark set as used in the descriptor set mask
                        result.descriptorSetMask.set(setIndex);
                    }
                }
            }
        }
    }

    // Deserialize vertex attributes
    if (auto vertexAttributes = table->get_as<toml::array>("vertexAttributes"))
    {
        for (const auto& entry : *vertexAttributes)
        {
            if (auto entryTable = entry.as_table())
            {
                if (auto index = entryTable->get("index"))
                {
                    auto attrIndex = index->as_integer()->get();

                    if (attrIndex >= 0 && attrIndex < VULKAN_NUM_VERTEX_ATTRIBS)
                    {
                        if (auto attribute = entryTable->get_as<toml::table>("attribute"))
                        {
                            result.vertexAttr[attrIndex] = deserializeVertexAttribState(&*attribute);
                            result.attributeMask.set(attrIndex);
                        }
                    }
                }
            }
        }
    }

    // Deserialize push constant range
    if (auto pushConstantRange = table->get_as<toml::table>("pushConstantRange"))
    {
        result.pushConstantRange = deserializePushConstantRange(&*pushConstantRange);
    }

    // Deserialize masks
    if (auto mask = table->get_as<toml::array>("attributeMask"))
        result.attributeMask = deserializeBitset<VULKAN_NUM_VERTEX_ATTRIBS>(&*mask);

    if (auto mask = table->get_as<toml::array>("renderTargetMask"))
        result.renderTargetMask = deserializeBitset<VULKAN_NUM_RENDER_TARGETS>(&*mask);

    if (auto mask = table->get_as<toml::array>("descriptorSetMask"))
        result.descriptorSetMask = deserializeBitset<VULKAN_NUM_DESCRIPTOR_SETS>(&*mask);

    if (auto mask = table->get_as<toml::array>("bindlessDescriptorSetMask"))
        result.bindlessDescriptorSetMask = deserializeBitset<VULKAN_NUM_DESCRIPTOR_SETS>(&*mask);

    if (auto mask = table->get_as<toml::array>("combinedSpecConstantMask"))
        result.combinedSpecConstantMask = deserializeBitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS>(&*mask);

    // Deserialize per-stage spec constants
    if (auto specConstantMasks = table->get_as<toml::table>("specConstantMasks"))
    {
        for (const auto& [stageName, mask] : *specConstantMasks)
        {
            ShaderStage stage = ShaderStage::NA;

            if (stageName == "VS")
                stage = ShaderStage::VS;
            else if (stageName == "TCS")
                stage = ShaderStage::TCS;
            else if (stageName == "TES")
                stage = ShaderStage::TES;
            else if (stageName == "GS")
                stage = ShaderStage::GS;
            else if (stageName == "FS")
                stage = ShaderStage::FS;
            else if (stageName == "CS")
                stage = ShaderStage::CS;
            else if (stageName == "TS")
                stage = ShaderStage::TS;
            else if (stageName == "MS")
                stage = ShaderStage::MS;

            if (stage != ShaderStage::NA && mask.is_array())
            {
                result.specConstantMask[stage] = deserializeBitset<VULKAN_NUM_TOTAL_SPEC_CONSTANTS>(mask.as_array());
            }
        }
    }

    return result;
}

toml::array serializeVertexInput(const VertexInput& input)
{
    toml::array result;

    // Serialize attributes
    for (const auto& attr : input.attributes)
    {
        toml::table attrTable;
        attrTable.insert("location", static_cast<int64_t>(attr.location));
        attrTable.insert("binding", static_cast<int64_t>(attr.binding));
        attrTable.insert("format", static_cast<int64_t>(attr.format));
        attrTable.insert("offset", static_cast<int64_t>(attr.offset));
        result.push_back(std::move(attrTable));
    }

    return result;
}

VertexInput deserializeVertexInput(const toml::array* array)
{
    VertexInput result;
    if (!array)
        return result;

    // Deserialize attributes
    for (const auto& entry : *array)
    {
        if (auto attrTable = entry.as_table())
        {
            VertexInput::VertexAttribute attr;

            if (auto location = attrTable->get("location"))
                attr.location = static_cast<uint32_t>(location->as_integer()->get());

            if (auto binding = attrTable->get("binding"))
                attr.binding = static_cast<uint32_t>(binding->as_integer()->get());

            if (auto format = attrTable->get("format"))
                attr.format = static_cast<Format>(format->as_integer()->get());

            if (auto offset = attrTable->get("offset"))
                attr.offset = static_cast<uint32_t>(offset->as_integer()->get());

            result.attributes.push_back(attr);
        }
    }

    // Reconstruct bindings based on attributes
    if (!result.attributes.empty())
    {
        uint32_t maxBinding = 0;
        for (const auto& attr : result.attributes)
        {
            maxBinding = std::max(maxBinding, attr.binding);
        }

        result.bindings.resize(maxBinding + 1);

        // Calculate stride for each binding
        HashMap<uint32_t, uint32_t> bindingStrides;
        for (const auto& attr : result.attributes)
        {
            // Simple stride calculation - assume attributes are tightly packed
            uint32_t attrSize = 0;
            switch (attr.format)
            {
            case Format::R32_FLOAT:
                attrSize = 4;
                break;
            case Format::RG32_FLOAT:
                attrSize = 8;
                break;
            case Format::RGB32_FLOAT:
                attrSize = 12;
                break;
            case Format::RGBA32_FLOAT:
                attrSize = 16;
                break;
            // Add more format cases as needed
            default:
                attrSize = 4;
                break;
            }

            bindingStrides[attr.binding] += attrSize;
        }

        // Set stride for each binding
        for (uint32_t i = 0; i <= maxBinding; i++)
        {
            if (bindingStrides.contains(i))
            {
                result.bindings[i].stride = bindingStrides[i];
            }
        }
    }

    return result;
}

toml::table serializeDescriptorResourceInfo(const DescriptorResourceInfo& info)
{
    toml::table result;

    // Serialize bindings
    toml::array bindings;
    for (const auto& binding : info.bindings)
    {
        toml::table bindingTable;
        bindingTable.insert("binding", static_cast<int64_t>(binding.binding));
        bindingTable.insert("descriptorType", static_cast<int64_t>(static_cast<int>(binding.descriptorType)));
        bindingTable.insert("descriptorCount", static_cast<int64_t>(binding.descriptorCount));
        bindingTable.insert("stageFlags", static_cast<int64_t>(static_cast<uint32_t>(binding.stageFlags)));
        bindings.push_back(std::move(bindingTable));
    }
    result.insert("bindings", std::move(bindings));

    // Serialize pool sizes
    toml::array poolSizes;
    for (const auto& poolSize : info.poolSizes)
    {
        toml::table poolSizeTable;
        poolSizeTable.insert("type", static_cast<int64_t>(static_cast<int>(poolSize.type)));
        poolSizeTable.insert("descriptorCount", static_cast<int64_t>(poolSize.descriptorCount));
        poolSizes.push_back(std::move(poolSizeTable));
    }
    result.insert("poolSizes", std::move(poolSizes));

    return result;
}

DescriptorResourceInfo deserializeDescriptorResourceInfo(const toml::table* table)
{
    DescriptorResourceInfo result;
    if (!table)
        return result;

    // Deserialize bindings
    if (auto bindings = table->get_as<toml::array>("bindings"))
    {
        for (const auto& entry : *bindings)
        {
            if (auto bindingTable = entry.as_table())
            {
                ::vk::DescriptorSetLayoutBinding binding;

                if (auto idx = bindingTable->get("binding"))
                    binding.binding = static_cast<uint32_t>(idx->as_integer()->get());

                if (auto type = bindingTable->get("descriptorType"))
                    binding.descriptorType = static_cast<::vk::DescriptorType>(type->as_integer()->get());

                if (auto count = bindingTable->get("descriptorCount"))
                    binding.descriptorCount = static_cast<uint32_t>(count->as_integer()->get());

                if (auto flags = bindingTable->get("stageFlags"))
                    binding.stageFlags = static_cast<::vk::ShaderStageFlags>(flags->as_integer()->get());

                result.bindings.push_back(binding);
            }
        }
    }

    // Deserialize pool sizes
    if (auto poolSizes = table->get_as<toml::array>("poolSizes"))
    {
        for (const auto& entry : *poolSizes)
        {
            if (auto poolSizeTable = entry.as_table())
            {
                ::vk::DescriptorPoolSize poolSize;

                if (auto type = poolSizeTable->get("type"))
                    poolSize.type = static_cast<::vk::DescriptorType>(type->as_integer()->get());

                if (auto count = poolSizeTable->get("descriptorCount"))
                    poolSize.descriptorCount = static_cast<uint32_t>(count->as_integer()->get());

                result.poolSizes.push_back(poolSize);
            }
        }
    }

    return result;
}

toml::table serializeReflectionResult(const ReflectionResult& result)
{
    toml::table root;

    // Add metadata
    root.insert("format_version", 1);
    root.insert("timestamp", std::time(nullptr));

    // Serialize main components
    root.insert("vertexInput", serializeVertexInput(result.vertexInput));
    root.insert("resourceLayout", serializeCombinedResourceLayout(result.resourceLayout));
    root.insert("pushConstantRange", serializePushConstantRange(result.pushConstantRange));

    // Serialize descriptor resources
    toml::array descriptorResources;
    for (uint32_t i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        if (result.resourceLayout.descriptorSetMask.test(i))
        {
            toml::table setTable;
            setTable.insert("set", static_cast<int64_t>(i));
            setTable.insert("resources", serializeDescriptorResourceInfo(result.descriptorResources[i]));
            descriptorResources.push_back(std::move(setTable));
        }
    }
    root.insert("descriptorResources", std::move(descriptorResources));

    return root;
}

ReflectionResult deserializeReflectionResult(const toml::table* table)
{
    ReflectionResult result{};
    if (!table)
        return result;

    // Deserialize format version for future compatibility
    int formatVersion = 1;
    if (auto version = table->get("format_version"))
    {
        formatVersion = static_cast<int>(version->as_integer()->get());
    }
    CM_LOG_DEBUG("format version: %d", formatVersion);

    // Deserialize main components
    if (auto vertexInput = table->get_as<toml::array>("vertexInput"))
        result.vertexInput = deserializeVertexInput(&*vertexInput);

    if (auto resourceLayout = table->get_as<toml::table>("resourceLayout"))
        result.resourceLayout = deserializeCombinedResourceLayout(&*resourceLayout);

    if (auto pushConstantRange = table->get_as<toml::table>("pushConstantRange"))
        result.pushConstantRange = deserializePushConstantRange(&*pushConstantRange);

    // Deserialize descriptor resources
    if (auto descriptorResources = table->get_as<toml::array>("descriptorResources"))
    {
        for (const auto& entry : *descriptorResources)
        {
            if (auto setTable = entry.as_table())
            {
                if (auto set = setTable->get("set"))
                {
                    auto setIndex = set->as_integer()->get();

                    if (setIndex >= 0 && setIndex < VULKAN_NUM_DESCRIPTOR_SETS)
                    {
                        if (auto resources = setTable->get_as<toml::table>("resources"))
                        {
                            result.descriptorResources[setIndex] = deserializeDescriptorResourceInfo(&*resources);
                        }
                    }
                }
            }
        }
    }

    return result;
}

bool saveReflectionToFile(const ReflectionResult& result, const std::filesystem::path& path)
{
    APH_PROFILER_SCOPE();

    try
    {
        // Create a TOML document
        toml::table document = serializeReflectionResult(result);

        // Create the directory if it doesn't exist
        if (!std::filesystem::exists(path.parent_path()))
        {
            std::filesystem::create_directories(path.parent_path());
        }

        // Open the file for writing
        std::ofstream out(path);
        if (!out.is_open())
        {
            VK_LOG_ERR("Failed to open file for writing: %s", path.string().c_str());
            return false;
        }

        // Write the TOML document to the file
        out << document;

        return true;
    }
    catch (const std::exception& e)
    {
        VK_LOG_ERR("Error saving reflection data: %s", e.what());
        return false;
    }
}

Result loadReflectionFromFile(const std::filesystem::path& path, ReflectionResult& result)
{
    APH_PROFILER_SCOPE();

    try
    {
        // Check if the file exists
        if (!std::filesystem::exists(path))
        {
            return { Result::RuntimeError, "Reflection file not found: " + path.string() };
        }

        // Parse the TOML file
        toml::table document;
        try
        {
            document = toml::parse_file(path.string());
        }
        catch (const toml::parse_error& e)
        {
            return { Result::RuntimeError, "Failed to parse TOML: " + std::string(e.what()) };
        }

        // Deserialize the reflection result
        result = deserializeReflectionResult(&document);

        return Result::Success;
    }
    catch (const std::exception& e)
    {
        return { Result::RuntimeError, "Error loading reflection data: " + std::string(e.what()) };
    }
}

} // namespace aph::reflection
