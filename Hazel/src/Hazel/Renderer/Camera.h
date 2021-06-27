//
// Created by npchitman on 6/28/21.
//

#ifndef HAZEL_ENGINE_CAMERA_H
#define HAZEL_ENGINE_CAMERA_H

#include <glm/glm.hpp>

namespace Hazel {

    class Camera {
    public:
        Camera() = default;
        explicit Camera(const glm::mat4& projection)
            : m_Projection(projection) {}

        const glm::mat4& GetProjection() const { return m_Projection; }

    protected:
        glm::mat4 m_Projection = glm::mat4(1.0f);
    };

}// namespace Hazel
#endif//HAZEL_ENGINE_CAMERA_H
