#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "uniformObject.h"

namespace vkl
{
enum class CameraType
{
    LOOKAT,
    FIRSTPERSON,
};

enum class Direction
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class Camera : public UniformObject
{
public:
    static std::shared_ptr<Camera> Create();
    Camera(IdType id) : UniformObject(id) {}
    ~Camera() override = default;

    void load() override;
    void update(float deltaTime) override;

    void setAspectRatio(float aspectRatio);
    void setPerspective(float fov, float aspect, float znear, float zfar);
    bool isMoving() const;

    void setPosition(glm::vec3 position) { _position = position; }
    void rotate(glm::vec3 delta) { _rotation += delta; }
    void setRotation(glm::vec3 rotation) { _rotation = rotation; }
    void setTranslation(glm::vec3 translation) { _position = translation; };
    void translate(glm::vec3 delta) { _position += delta; }
    void setType(CameraType type) { _cameraType = type; }
    void setRotationSpeed(float rotationSpeed) { _rotationSpeed = rotationSpeed; }
    void setMovementSpeed(float movementSpeed) { _movementSpeed = movementSpeed; }

    float getNearClip() const { return _znear; }
    float getFarClip() const { return _zfar; }
    float getRotationSpeed() const { return _rotationSpeed; }

    void setMovement(Direction direction, bool flag) { keys[direction] = flag; }
    void setFlipY(bool val) { _flipY = val; }

private:
    void updateViewMatrix();
    void processMovement(float deltaTime);
    void updateAspectRatio(float aspect);

private:
    std::unordered_map<Direction, bool> keys{ { Direction::LEFT, false },
                                              { Direction::RIGHT, false },
                                              { Direction::UP, false },
                                              { Direction::DOWN, false } };

    CameraType _cameraType = CameraType::FIRSTPERSON;

    glm::vec3 _rotation = glm::vec3();
    glm::vec3 _position = glm::vec3();

    float _rotationSpeed = 1.0f;
    float _movementSpeed = 1.0f;

    bool _flipY = false;

    struct
    {
        glm::mat4 perspective;
        glm::mat4 view;
    } _matrices;

    float _fov = 60.0f;
    float _znear = 96.0f;
    float _zfar = 0.01f;
};
}  // namespace vkl
#endif
