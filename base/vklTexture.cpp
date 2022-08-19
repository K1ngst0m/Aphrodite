#include "vklTexture.h"
#include <cassert>
#include <cstring>

namespace vkl {
    /**
    * Attach the allocated memory block to the buffer
    *
    * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
    *
    * @return VkResult of the bindBufferMemory call
    */
    VkResult Texture::bind(VkDeviceSize offset) const
    {
        return vkBindImageMemory(device, image, memory, offset);
    }

    /**
    * Setup the default descriptor for this buffer
    *
    * @param size (Optional) Size of the memory range of the descriptor
    * @param offset (Optional) Byte offset from beginning
    *
    */
    void Texture::setupDescriptor(VkImageLayout layout)
    {
        descriptorInfo.sampler = sampler;
        descriptorInfo.imageView = imageView;
        descriptorInfo.imageLayout = layout;
    }

    /**
    * Release all Vulkan resources held by this buffer
    */
    void Texture::destroy() const
    {
        if (sampler){
            vkDestroySampler(device, sampler, nullptr);
        }
        if(imageView){
            vkDestroyImageView(device, imageView, nullptr);
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
