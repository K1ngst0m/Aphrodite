#include "instance.h"
#include "api/vulkan/vkUtils.h"
#include "common/logger.h"
#include "physicalDevice.h"

namespace aph::vk
{
Instance::Instance(const CreateInfoType& createInfo, HandleType handle)
    : ResourceHandle(handle, createInfo) {};

Result Instance::Create(const InstanceCreateInfo& createInfo, Instance** ppInstance)
{
    // Get extensions supported by the instance and store for later use
    {
        HashSet<std::string> supportedExtensions{};

        auto getSupportExtension = [&supportedExtensions](std::string layerName)
        {
            auto [res, extensions] = ::vk::enumerateInstanceExtensionProperties(layerName);
            VK_VR(res);
            for (VkExtensionProperties extension : extensions)
            {
                supportedExtensions.insert(extension.extensionName);
            }
        };

        // vulkan implementation and implicit layers
        getSupportExtension("");
        // explicit layers
        for (const auto& layer : createInfo.enabledLayers)
        {
            getSupportExtension(layer);
        }

        bool allExtensionSupported = true;
        for (const auto& requiredExtension : createInfo.enabledExtensions)
        {
            if (!supportedExtensions.contains(requiredExtension))
            {
                VK_LOG_ERR("The instance extension %s is not supported.", requiredExtension);
                allExtensionSupported = false;
            }
        }
        if (!allExtensionSupported)
        {
            return { Result::RuntimeError, "Required instance extensions are not fully supported." };
        }
    }

    // check layer support
    {
        HashSet<std::string> supportedLayers{};
        auto [res, layerProperties] = ::vk::enumerateInstanceLayerProperties();
        VK_VR(res);
        for (const auto& layerPropertie : layerProperties)
        {
            supportedLayers.insert(layerPropertie.layerName);
        }

        bool allLayerSFound = true;
        for (const char* layerName : createInfo.enabledLayers)
        {
            if (!supportedLayers.contains(layerName))
            {
                VK_LOG_ERR("The instance layer %s is not found.", layerName);
                allLayerSFound = false;
            }
            if (!allLayerSFound)
            {
                return { Result::RuntimeError, "Required instance layers are not found." };
            }
        }
    }

    // vk instance creation
    Instance* instance = {};
    {
        ::vk::ApplicationInfo app_info{};
        app_info.setPApplicationName(createInfo.appName.c_str())
            .setPEngineName("Aphrodite")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_4);

        ::vk::InstanceCreateInfo instance_create_info{};
        instance_create_info.setPApplicationInfo(&app_info)
            .setPEnabledLayerNames(createInfo.enabledLayers)
            .setPEnabledExtensionNames(createInfo.enabledExtensions);

#if defined(APH_DEBUG)
        instance_create_info.setPNext(&createInfo.debugCreateInfo);
#endif

        auto [res, instance_handle] = ::vk::createInstance(instance_create_info, vk::vk_allocator());
        VK_VR(res);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_handle);
        instance = new Instance(createInfo, instance_handle);
    }
    APH_ASSERT(instance);

    // query gpu support
    {
        auto [res, gpus] = instance->getHandle().enumeratePhysicalDevices();
        VK_VR(res);
        for (uint32_t idx = 0; const auto& gpu : gpus)
        {
            auto pdImpl = instance->m_physicalDevicePools.allocate(gpu);
            auto gpuProperties = pdImpl->getProperties();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuProperties.GpuVendorPreset.gpuName);
            VK_LOG_INFO("Driver Version: %s", gpuProperties.GpuVendorPreset.gpuDriverVersion);
            instance->m_physicalDevices.push_back(std::move(pdImpl));
            idx++;
        }
    }

    *ppInstance = instance;

#if defined(APH_DEBUG)
    auto [res, debugMessenger] =
        instance->getHandle().createDebugUtilsMessengerEXT(createInfo.debugCreateInfo, vk_allocator());
    VK_VR(res);
    instance->m_debugMessenger = debugMessenger;
#endif

    return Result::Success;
}

void Instance::Destroy(Instance* pInstance)
{
#ifdef APH_DEBUG
    pInstance->getHandle().destroyDebugUtilsMessengerEXT(pInstance->m_debugMessenger, vk_allocator());
#endif
    pInstance->getHandle().destroy(vk_allocator());
    delete pInstance;
}
} // namespace aph::vk
