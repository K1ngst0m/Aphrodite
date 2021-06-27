//
// Created by npchitman on 6/28/21.
//

#ifndef HAZEL_ENGINE_CAMERA_H
#define HAZEL_ENGINE_CAMERA_H

#include <glm/glm.hpp>

namespace Hazel {

    class Camera {
    public:
        explicit Camera(const glm::mat4& projection)
            : m_Projection(projection) {}

        const glm::mat4& GetProjection() const { return m_Projection; }

    private:
        glm::mat4 m_Projection;
    };

}// namespace Hazel
#endif//HAZEL_ENGINE_CAMERA_H
