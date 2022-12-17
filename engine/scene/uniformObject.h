#ifndef UNIFORMOBJECT_H_
#define UNIFORMOBJECT_H_

#include "object.h"

namespace vkl
{
class UniformObject : public Object
{
public:
    UniformObject(IdType id, ObjectType type) : Object(id, type) {}
    ~UniformObject() override = default;

    virtual void load() = 0;
    virtual void update(float deltaTime) = 0;

    virtual bool isUpdated() const { return updated; }
    virtual void setUpdated(bool flag) { updated = flag; }
    virtual void *getData() { return data.get(); }
    virtual uint32_t getDataSize() { return dataSize; }

protected:
    size_t dataSize = 0;
    std::shared_ptr<void> data = nullptr;

    bool updated = false;
};
}  // namespace vkl

#endif  // UNIFORMOBJECT_H_
