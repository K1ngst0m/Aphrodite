//
// Created by npchitman on 6/29/21.
//

#ifndef Aphrodite_ENGINE_MATH_H
#define Aphrodite_ENGINE_MATH_H

#include <glm/glm.hpp>

namespace Aph::Math{
    std::tuple<glm::vec3, glm::vec3, glm::vec3>
            DecomposeTransform(const glm::mat4& transform);
}


#endif//Aphrodite_ENGINE_MATH_H
