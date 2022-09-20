#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "api/vulkan/mesh.h"

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

    virtual void load() = 0;
    virtual void update() = 0;
    virtual void* getData() = 0;
    virtual uint32_t getDataSize() = 0;

    void destroy() override = 0;

    bool isNeedUpdate() const;
    void setNeedUpdate(bool flag);

protected:
    bool needUpdate = false;
};
} // namespace vkl

#endif // VKLMODEL_H_
