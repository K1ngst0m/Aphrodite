#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "common.h"

namespace vkl {

class SceneManager;

class Object {
public:
    Object(SceneManager *manager);
    virtual ~Object() = default;

protected:
    SceneManager *_manager;
};

} // namespace vkl

#endif // VKLMODEL_H_
