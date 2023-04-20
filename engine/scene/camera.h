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

class Camera : public Object
{
public:
    Camera(CameraType cameraType = CameraType::UNDEFINED) : Object{Id::generateNewId<Camera>(), ObjectType::CAMERA}, m_cameraType(cameraType) {}
    ~Camera() override = default;

    virtual void setAspectRatio(float aspect) { m_aspect = aspect; };
    virtual void updateViewMatrix();

    float      getRotationSpeed() const { return m_rotationSpeed; }
    float      getMovementSpeed() const { return m_movementSpeed; }
    bool       getFlipY() const { return m_flipY; }
    glm::vec3  getRotation() const { return m_rotation; }
    glm::vec3  getPosition() const { return m_position; }
    glm::mat4  getProjMatrix() const { return m_projection; }
    glm::mat4  getViewMatrix() const { return m_view; }
    CameraType getType() const { return m_cameraType; }

    void setType(CameraType type) { m_cameraType = type; }
    void setFlipY(bool val) { m_flipY = val; }
    void setPosition(glm::vec3 position) { m_position = position; }
    void setRotation(glm::vec3 rotation) { m_rotation = rotation; }
    void setRotationSpeed(float rotationSpeed) { m_rotationSpeed = rotationSpeed; }
    void setMovementSpeed(float movementSpeed) { m_movementSpeed = movementSpeed; }

    void move(Direction direction, bool flag) { m_keys[direction] = flag; }
    void rotate(glm::vec3 delta) { m_rotation += delta * m_rotationSpeed; }
    void translate(glm::vec3 delta) { m_position += delta * m_movementSpeed; }

    void updateMovement(float deltaTime);

protected:
    std::unordered_map<Direction, bool> m_keys{
        {Direction::LEFT, false}, {Direction::RIGHT, false}, {Direction::UP, false}, {Direction::DOWN, false}};

    CameraType m_cameraType{CameraType::UNDEFINED};

    glm::mat4 m_projection{};
    glm::vec3 m_rotation{};
    glm::vec3 m_position{};
    glm::mat4 m_view{};

    float m_rotationSpeed{1.0f};
    float m_movementSpeed{1.0f};

    bool  m_flipY{false};
    float m_aspect{0.0f};
};

class PerspectiveCamera : public Camera
{
public:
    PerspectiveCamera(): Camera(CameraType::PERSPECTIVE){}
    ~PerspectiveCamera() override = default;
    float getFov() const { return m_perspective.fov; }
    float getZNear() const { return m_perspective.znear; }
    float getZFar() const { return m_perspective.zfar; }

    void setAspectRatio(float aspect) override;
    void setZNear(float value);
    void setZFar(float value);
    void setFov(float value);

private:
    void updatePerspective(float fov, float aspect, float znear, float zfar);

    struct
    {
        float fov{60.0f};
        float znear{96.0f};
        float zfar{0.01f};
    } m_perspective;
};

class OrthoCamera : public Camera
{
public:
    OrthoCamera(): Camera(CameraType::ORTHO){}
    ~OrthoCamera() override = default;

private:
    void updatePerspective(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax);
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
