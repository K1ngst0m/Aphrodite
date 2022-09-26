#include "uniformObject.h"

namespace vkl {
bool UniformObject::isUpdated() const {
    return updated;
}
void UniformObject::setUpdated(bool flag) {
    updated = flag;
}
UniformObject::UniformObject(SceneManager *manager)
    : Object(manager) {
}
void *UniformObject::getData() {
    return data.get();
}
uint32_t UniformObject::getDataSize() {
    return dataSize;
}
}
