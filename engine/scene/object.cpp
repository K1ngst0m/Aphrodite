#include "object.h"

namespace vkl {
Object::Object(SceneManager *manager, IdType id)
    : IdObject(id), _manager(manager) {
}
} // namespace vkl
