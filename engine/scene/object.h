#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "idObject.h"

namespace vkl
{

class Scene;

enum class ObjectType : uint8_t
{
    UNATTACHED,
    ENTITY,
    LIGHT,
    CAMERA,
    MESH,
};

class Object : public IdObject
{
public:
    Object(IdType id, ObjectType type) : IdObject(id), m_type(type) {}
    virtual ~Object() = default;

    ObjectType getType() { return m_type; }

protected:
    ObjectType m_type;
};

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

#endif  // VKLMODEL_H_
