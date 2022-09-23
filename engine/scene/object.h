#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "common.h"

namespace vkl {

class SceneManager;

class Object {
public:
    Object(SceneManager *manager)
        : _manager(manager) {
    }
    virtual ~Object() = default;

protected:
    SceneManager *_manager;
};

class UniformBufferObject : public Object {
public:
    UniformBufferObject(SceneManager *manager);
    ~UniformBufferObject() override = default;

    virtual void     load()   = 0;
    virtual void     update() = 0;
    virtual void    *getData();
    virtual uint32_t getDataSize();

    bool isUpdated() const;
    void setNeedUpdate(bool flag);

protected:
    size_t                dataSize = 0;
    std::shared_ptr<void> data     = nullptr;

    bool updated = false;
};
} // namespace vkl

#endif // VKLMODEL_H_
