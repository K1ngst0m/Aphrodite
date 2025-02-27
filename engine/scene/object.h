#pragma once

#include "idObject.h"

namespace aph
{
enum class ObjectType : uint8_t
{
    UNATTACHED,
    LIGHT,
    CAMERA,
    MESH,
    SCENENODE,
};

template <typename T>
class Object : public IdObject
{
public:
    Object(ObjectType type) :
        IdObject{Id::generateNewId<T>()}, m_ObjectType{type} {}
    ~Object() override = default;

    ObjectType getType() { return m_ObjectType; }

protected:
    ObjectType m_ObjectType{};
};

}  // namespace aph
