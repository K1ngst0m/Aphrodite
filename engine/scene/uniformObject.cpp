#include "uniformObject.h"

namespace vkl {
UniformObject::UniformObject(SceneManager *manager, IdType id)
    : Object(manager, id) {
}
bool UniformObject::isUpdated() const {
    return updated;
}
void UniformObject::setUpdated(bool flag) {
    updated = flag;
}
void *UniformObject::getData() {
    return data.get();
}
uint32_t UniformObject::getDataSize() {
    return dataSize;
}
}
