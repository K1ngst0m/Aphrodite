#include "object.h"

namespace vkl {
bool UniformBufferObject::isUpdated() const {
    return updated;
}
void UniformBufferObject::setUpdated(bool flag) {
    updated = flag;
}
UniformBufferObject::UniformBufferObject(SceneManager *manager)
    : Object(manager) {
}
void *UniformBufferObject::getData() {
    return data.get();
}
uint32_t UniformBufferObject::getDataSize() {
    return dataSize;
}
} // namespace vkl
