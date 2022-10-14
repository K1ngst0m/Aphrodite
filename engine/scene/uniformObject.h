#ifndef UNIFORMOBJECT_H_
#define UNIFORMOBJECT_H_

#include "object.h"

namespace vkl {
class UniformObject : public Object {
public:
    UniformObject(IdType id);
    ~UniformObject() override = default;

    virtual void     load()   = 0;
    virtual void     update(float deltaTime) = 0;
    virtual void    *getData();
    virtual uint32_t getDataSize();

    bool isUpdated() const;
    void setUpdated(bool flag);

protected:
    size_t                dataSize = 0;
    std::shared_ptr<void> data     = nullptr;

    bool updated = false;
};
}

#endif // UNIFORMOBJECT_H_
