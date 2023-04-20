#include "light.h"

namespace aph
{
Light::Light() : Object(Id::generateNewId<Light>(), ObjectType::LIGHT) {}
}  // namespace aph
