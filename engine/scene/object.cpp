#include "object.h"

namespace vkl {
bool UniformBufferObject::isNeedUpdate() const {
    return needUpdate;
}
void UniformBufferObject::setNeedUpdate(bool flag) {
    needUpdate = flag;
}
UniformBufferObject::UniformBufferObject(SceneManager *manager)
    : Object(manager) {
}
} // namespace vkl
