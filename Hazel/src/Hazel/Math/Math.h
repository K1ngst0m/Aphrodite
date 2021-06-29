//
// Created by npchitman on 6/29/21.
//

#ifndef HAZEL_ENGINE_MATH_H
#define HAZEL_ENGINE_MATH_H

#include <glm/glm.hpp>

namespace Hazel::Math{
    bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
}


#endif//HAZEL_ENGINE_MATH_H
