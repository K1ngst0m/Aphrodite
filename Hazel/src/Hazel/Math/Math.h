//
// Created by npchitman on 6/29/21.
//

#ifndef HAZEL_ENGINE_MATH_H
#define HAZEL_ENGINE_MATH_H

#include <glm/glm.hpp>

namespace Hazel::Math{
    bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);
}


#endif//HAZEL_ENGINE_MATH_H
