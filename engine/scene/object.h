#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include <memory>
#include "idObject.h"

namespace aph
{

class Scene;

enum class ObjectType : uint8_t
{
    UNATTACHED,
    LIGHT,
    CAMERA,
    MESH,
    SCENENODE,
};

class Object : public IdObject
{
public:
    template <typename TObject, typename... Args>
    static std::unique_ptr<TObject> Create(Args&&... args)
    {
        auto instance = std::make_unique<TObject>(std::forward<Args>(args)...);
        return instance;
    }
    Object(IdType id, ObjectType type) : IdObject{id}, m_ObjectType{type} {}
    virtual ~Object() = default;

    ObjectType getType() { return m_ObjectType; }

protected:
    ObjectType m_ObjectType{};
};

}  // namespace aph

#endif  // VKLMODEL_H_
