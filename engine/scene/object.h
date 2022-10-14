#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "sceneManager.h"

namespace vkl {

class SceneManager;

class Object : public IdObject {
public:
    Object(IdType id);
    virtual ~Object() = default;
};

} // namespace vkl

#endif // VKLMODEL_H_
