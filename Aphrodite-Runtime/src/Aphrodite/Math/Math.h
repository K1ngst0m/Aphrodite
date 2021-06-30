//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_MATH_H
#define Aphrodite_ENGINE_MATH_H

#include <glm/glm.hpp>

namespace Aph::Math{
    bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
}


#endif//Aphrodite_ENGINE_MATH_H
