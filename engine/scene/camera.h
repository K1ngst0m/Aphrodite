#pragma once

#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace aph
{
enum class CameraType
{
    Orthographic,
    Perspective,
};

struct Orthographic
{
    float left   = {};
    float right  = {};
    float bottom = {};
    float top    = {};
    float znear  = {1.0f};
    float zfar   = {1000.0f};
};

struct Perspective
{
    float aspect = {16.0f / 9.0f};
    float fov    = {60.0f};
    float znear  = {1.0f};
    float zfar   = {1000.0f};
};

class Camera : public Object<Camera>
{
public:
    Camera(CameraType cameraType) : Object{ObjectType::CAMERA}, m_cameraType(cameraType) {}

    CameraType getType() const { return m_cameraType; }

    glm::mat4 getProjection();
    glm::mat4 getView();

    Camera& setProjection(Perspective perspective);
    Camera& setProjection(Orthographic orthographic);
    Camera& setProjection(glm::mat4 value);

    Camera& setLookAt(const glm::vec3& eye, const glm::vec3& at, const glm::vec3& up);
    Camera& setView(glm::mat4 value);

    Camera& setPosition(glm::vec4 value);

    ~Camera() override = default;

private:
    void updateProjection();
    void updateView();

private:
    CameraType m_cameraType{CameraType::Perspective};

    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};

    glm::vec4 m_position{0.0f};
    glm::quat m_orientation{1.0f, 0.0f, 0.0f, 0.0f};

    bool m_flipY{true};

    Orthographic m_orthographic;
    Perspective  m_perspective;

    struct
    {
        bool projection = true;
        bool view       = true;
    } m_dirty;
};

enum class Direction
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

}  // namespace aph
