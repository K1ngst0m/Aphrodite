#include "debug.h"
#include "engine.h"

namespace aph
{

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(::vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                             ::vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                             const ::vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                             void* pUserData)
{
    if (!pCallbackData->pMessage)
    {
        return VK_TRUE;
    }
    static std::mutex errMutex; // Mutex for thread safety
    static uint32_t errCount = 0;

    // Skip general messages if loader logs are disabled
    auto* debugData = static_cast<Engine::DebugCallbackData*>(pUserData);
    if (messageType == ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral && !debugData->enableDeviceInitLogs)
    {
        return VK_FALSE;
    }

    std::stringstream msg;
    if (messageType != ::vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
    {
        uint32_t frameId = debugData->frameId;
        msg << "[frame:" << frameId << "] ";
    }
    else
    {
        msg << "[general] ";
    }

    for (uint32_t idx = 0; idx < pCallbackData->objectCount; idx++)
    {
        auto& obj = pCallbackData->pObjects[idx];
        if (obj.pObjectName)
        {
            msg << "[name: " << obj.pObjectName << "]";
        }
    }

    msg << " >>> ";

    msg << std::string{pCallbackData->pMessage};

    switch (messageSeverity)
    {
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        VK_LOG_DEBUG("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        VK_LOG_INFO("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        VK_LOG_WARN("%s", msg.str());
        break;
    case ::vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
    {
        std::lock_guard<std::mutex> lock(errMutex);
        if (++errCount > 10)
        {
            VK_LOG_ERR("Too many errors, exit.");
            DebugBreak();
        }
        VK_LOG_ERR("%s", msg.str().c_str());
        DebugBreak();
    }
    break;

    default:
        break;
    }
    return VK_FALSE;
}
} // namespace aph
