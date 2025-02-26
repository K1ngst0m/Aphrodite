#include "instance.h"
#include "api/vulkan/vkUtils.h"
#include "physicalDevice.h"
#include "common/logger.h"

namespace aph::vk
{
Instance::Instance(const CreateInfoType& createInfo, HandleType handle) : ResourceHandle(handle, createInfo) {};

Result Instance::Create(const InstanceCreateInfo& createInfo, Instance** ppInstance)
{
    // Get extensions supported by the instance and store for later use
    {
        HashSet<std::string> supportedExtensions{};

        auto getSupportExtension = [&supportedExtensions](std::string layerName) {
            auto extensions = ::vk::enumerateInstanceExtensionProperties(layerName);
            for(VkExtensionProperties extension : extensions)
            {
                supportedExtensions.insert(extension.extensionName);
            }
        };

        // vulkan implementation and implicit layers
        getSupportExtension("");
        // explicit layers
        for(const auto& layer : createInfo.enabledLayers)
        {
            getSupportExtension(layer);
        }

        bool allExtensionSupported = true;
        for(const auto& requiredExtension : createInfo.enabledExtensions)
        {
            if(!supportedExtensions.contains(requiredExtension))
            {
                VK_LOG_ERR("The instance extension %s is not supported.", requiredExtension);
                allExtensionSupported = false;
            }
        }
        if(!allExtensionSupported)
        {
            return {Result::RuntimeError, "Required instance extensions are not fully supported."};
        }
    }

    // check layer support
    {
        HashSet<std::string> supportedLayers{};
        for(const auto& layerProperties : ::vk::enumerateInstanceLayerProperties())
        {
            supportedLayers.insert(layerProperties.layerName);
        }

        bool allLayerSFound = true;
        for(const char* layerName : createInfo.enabledLayers)
        {
            if(!supportedLayers.contains(layerName))
            {
                VK_LOG_ERR("The instance layer %s is not found.", layerName);
                allLayerSFound = false;
            }
            if(!allLayerSFound)
            {
                return {Result::RuntimeError, "Required instance layers are not found."};
            }
        }
    }

    // vk instance creation
    ::vk::Instance instance_handle;
    {
        ::vk::ApplicationInfo app_info{};
        app_info.setPApplicationName(createInfo.appName.c_str())
            .setPEngineName("Aphrodite")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_3);

        ::vk::InstanceCreateInfo instance_create_info{};
        instance_create_info.setPApplicationInfo(&app_info)
            .setPEnabledLayerNames(createInfo.enabledLayers)
            .setPEnabledExtensionNames(createInfo.enabledExtensions);

    #if defined(APH_DEBUG)
        instance_create_info.setPNext(&createInfo.debugCreateInfo);
    #endif

        instance_handle = ::vk::createInstance(instance_create_info, vk::vk_allocator());
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_handle);
        volkLoadInstance(static_cast<VkInstance>(instance_handle));
    }

    Instance* instance = new Instance(createInfo, instance_handle);

    // query gpu support
    {
        auto gpus = instance_handle.enumeratePhysicalDevices();
        for(uint32_t idx = 0; const auto& gpu : gpus)
        {
            auto pdImpl      = std::make_unique<PhysicalDevice>(gpu);
            auto gpuSettings = pdImpl->getSettings();
            VK_LOG_INFO(" == Device Info [%d] ==", idx);
            VK_LOG_INFO("Device Name: %s", gpuSettings.GpuVendorPreset.gpuName);
            VK_LOG_INFO("Driver Version: %s", gpuSettings.GpuVendorPreset.gpuDriverVersion);
            instance->m_physicalDevices.push_back(std::move(pdImpl));
            idx++;
        }
    }

    *ppInstance = instance;

#if defined(APH_DEBUG)
    instance->m_debugMessenger =
        instance_handle.createDebugUtilsMessengerEXT(createInfo.debugCreateInfo, vk_allocator());
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
}  // namespace aph::vk
