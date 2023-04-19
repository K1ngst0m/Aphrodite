#ifndef CAMERA_H
#define CAMERA_H

#include "object.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace aph
{
enum class CameraType
{
    LOOKAT,
    FIRST_PERSON,
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
    Camera() : Object{Id::generateNewId<Camera>(), ObjectType::CAMERA} {}
    ~Camera() override = default;

    void setAspectRatio(float aspectRatio);
    void setPerspective(float fov, float aspect, float znear, float zfar);
    bool isMoving() const;

    void setPosition(glm::vec3 position) { m_position = position; }
    void rotate(glm::vec3 delta) { m_rotation += delta; }
    void setRotation(glm::vec3 rotation) { m_rotation = rotation; }
    void setTranslation(glm::vec3 translation) { m_position = translation; };
    void translate(glm::vec3 delta) { m_position += delta; }
    void setType(CameraType type) { m_cameraType = type; }
    void setRotationSpeed(float rotationSpeed) { m_rotationSpeed = rotationSpeed; }
    void setMovementSpeed(float movementSpeed) { m_movementSpeed = movementSpeed; }

    float getNearClip() const { return m_znear; }
    float getFarClip() const { return m_zfar; }
    float getRotationSpeed() const { return m_rotationSpeed; }

    void setMovement(Direction direction, bool flag) { m_keys[direction] = flag; }
    void setFlipY(bool val) { m_flipY = val; }

    glm::vec3 getPosition() const { return m_position; }
    float     getFov() const { return m_fov; }

    glm::mat4 getProjMatrix()
    {
        updateViewMatrix();
        return m_matrices.proj;
    }

    glm::mat4 getViewMatrix()
    {
        updateViewMatrix();
        return m_matrices.view;
    }

    void processMovement(float deltaTime);

private:
    void updateViewMatrix();
    void updateAspectRatio(float aspect);

private:
    std::unordered_map<Direction, bool> m_keys{
        {Direction::LEFT, false}, {Direction::RIGHT, false}, {Direction::UP, false}, {Direction::DOWN, false}};

    CameraType m_cameraType{CameraType::FIRST_PERSON};

    glm::vec3 m_rotation{};
    glm::vec3 m_position{};

    float m_rotationSpeed{1.0f};
    float m_movementSpeed{1.0f};

    struct
    {
        glm::mat4 proj;
        glm::mat4 view;
    } m_matrices;

    bool  m_flipY{false};
    float m_fov{60.0f};
    float m_znear{96.0f};
    float m_zfar{0.01f};
    bool  m_updated{false};
};
}  // namespace aph
#endif
