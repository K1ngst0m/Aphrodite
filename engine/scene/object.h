#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "api/mesh.h"

namespace vkl {

class SceneManager;

class Object {
public:
    Object(SceneManager * manager)
        :_manager(manager)
    {}

    virtual void destroy() = 0;
protected:
    SceneManager * _manager;
};

class UniformBufferObject : public Object {
public:
    UniformBufferObject(SceneManager *manager);

    vkl::UniformBuffer buffer;

    void setupBuffer(vkl::Device *device, uint32_t bufferSize, void *data = nullptr);
    void updateBuffer(void *data);

    void destroy() override;
};
} // namespace vkl

#endif // VKLMODEL_H_
