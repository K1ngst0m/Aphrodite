//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_AABB_H
#define APHRODITE_AABB_H

#include <glm/glm.hpp>

namespace Aph{
    struct AABB{
        explicit AABB(const glm::vec3& min = glm::vec3(0.0f),
                      const glm::vec3& max = glm::vec3(0.0f))
                :Min(min), Max(max){ }

        glm::vec3 Min, Max;
    };
}


#endif//APHRODITE_AABB_H
