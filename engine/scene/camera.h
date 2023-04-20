#ifndef CAMERA_H
#define CAMERA_H

#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace aph
{
enum class CameraType
{
    UNDEFINED,
    ORTHO,
    PERSPECTIVE,
};

enum class Direction
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

struct Camera : public Object
{
    Camera(CameraType cameraType = CameraType::UNDEFINED) :
        Object{Id::generateNewId<Camera>(), ObjectType::CAMERA},
        m_cameraType(cameraType)
    {
    }
    ~Camera() override = default;

    void move(Direction direction, bool flag) { m_keys[direction] = flag; }
    void rotate(glm::vec3 delta) { m_rotation += delta * m_rotationSpeed; }
    void translate(glm::vec3 delta) { m_position += delta * m_movementSpeed; }

    virtual void updateProj() = 0;
    virtual void updateView();
    void         updateMovement(float deltaTime);

    CameraType m_cameraType{CameraType::UNDEFINED};
    glm::mat4  m_projection{1.0f};
    glm::vec3  m_rotation{1.0f};
    glm::vec3  m_position{1.0f};
    glm::mat4  m_view{1.0f};

    float m_rotationSpeed{1.0f};
    float m_movementSpeed{1.0f};

    bool  m_flipY{false};
    float m_aspect{0.0f};

protected:
    std::unordered_map<Direction, bool> m_keys{
        {Direction::LEFT, false}, {Direction::RIGHT, false}, {Direction::UP, false}, {Direction::DOWN, false}};
};

struct PerspectiveCamera : public Camera
{
    PerspectiveCamera() : Camera(CameraType::PERSPECTIVE) {}
    ~PerspectiveCamera() override = default;
    void updateProj() override;

    struct
    {
        float fov{60.0f};
        float znear{96.0f};
        float zfar{0.01f};
    } m_perspective;
};

struct OrthoCamera : public Camera
{
    OrthoCamera() : Camera(CameraType::ORTHO) {}
    ~OrthoCamera() override = default;
    void updateProj() override;

    struct
    {
        float left{};
        float right{};
        float bottom{};
        float top{};
        float front{};
        float back{};
    } m_ortho;
};
}  // namespace aph
#endif
