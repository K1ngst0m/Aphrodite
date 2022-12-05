#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "idObject.h"

namespace vkl
{

class Scene;

class Object : public IdObject
{
public:
    Object(IdType id) : IdObject(id) {}
    virtual ~Object() = default;
};

}  // namespace vkl

#endif  // VKLMODEL_H_
