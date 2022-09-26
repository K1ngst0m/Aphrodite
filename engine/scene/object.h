#ifndef VKLMODEL_H_
#define VKLMODEL_H_

#include "common.h"
#include "idObject.h"

namespace vkl {

class SceneManager;

class Object : public IdObject {
public:
    Object(SceneManager *manager, IdType id);
    virtual ~Object() = default;

protected:
    SceneManager *_manager;
};

} // namespace vkl

#endif // VKLMODEL_H_
