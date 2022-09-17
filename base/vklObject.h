#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "vklDevice.h"
#include "vklInit.hpp"
#include "vklMaterial.h"
#include "vklMesh.h"
#include "vklPipeline.h"
#include "vklUtils.h"

namespace vkl {

class SceneManager;

class Object {
public:
    virtual void destroy() = 0;
};

class UniformBufferObject : public Object {
public:
    vkl::UniformBuffer buffer;

    void setupBuffer(vkl::Device *device, VkDeviceSize bufferSize, void *data = nullptr);
    void update(const void *data);
    void destroy() override;
};
} // namespace vkl

#endif // VKLMODEL_H_
