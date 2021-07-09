//
// Created by npchitman on 6/28/21.
//

#ifndef Aphrodite_ENGINE_CAMERA_H
#define Aphrodite_ENGINE_CAMERA_H

#include <glm/glm.hpp>

namespace Aph {

    class Camera {
    public:
        Camera() = default;
        explicit Camera(const glm::mat4& projection)
            : m_Projection(projection) {}

        const glm::mat4& GetProjection() const { return m_Projection; }

    protected:
        glm::mat4 m_Projection = glm::mat4(1.0f);
    };

}// namespace Aph
#endif//Aphrodite_ENGINE_CAMERA_H
