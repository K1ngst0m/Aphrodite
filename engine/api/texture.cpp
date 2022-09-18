#include "texture.h"
#include <cassert>
#include <cstring>

namespace vkl {
    /**
    * Attach the allocated memory block to the image
    *
    * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
    *
    * @return VkResult of the bindImageMemory call
    */
    VkResult Texture::bind(VkDeviceSize offset) const
    {
        return vkBindImageMemory(device, image, memory, offset);
    }

    /**
    * Setup the default descriptor for this image
    */
    void Texture::setupDescriptor(VkImageLayout layout)
    {
        descriptorInfo.sampler = sampler;
        descriptorInfo.imageView = view;
        descriptorInfo.imageLayout = layout;
    }

    /**
    * Release all Vulkan resources held by this image
    */
    void Texture::destroy() const
    {
        if (sampler){
            vkDestroySampler(device, sampler, nullptr);
        }
        if(view){
            vkDestroyImageView(device, view, nullptr);
        }
        if (image)
        {
            vkDestroyImage(device, image, nullptr);
        }
        if (memory)
        {
            vkFreeMemory(device, memory, nullptr);
        }
    }
}
