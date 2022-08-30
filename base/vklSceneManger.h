#ifndef VKLSCENEMANGER_H_
#define VKLSCENEMANGER_H_

#include "vklModel.h"

namespace vkl{
class SceneManager{
public:
    void pushUniform(UniformBufferObject *ubo);

    void pushObject(MeshObject *object, ShaderPass *pass, glm::mat4 transform = glm::mat4(1.0f));

    void drawSceneRecord(VkCommandBuffer commandBuffer, ShaderPass &pass);

    void setupDescriptor(vkl::Device *device, uint32_t setCount, VkDescriptorSetLayout setLayout);

    void destroy(VkDevice device);

private:
    std::vector<vkl::RenderObject*> renderList;
    std::vector<vkl::UniformBufferObject*> ubolist;

    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_globalDescriptorSet;
};
}

#endif // VKLSCENEMANGER_H_
