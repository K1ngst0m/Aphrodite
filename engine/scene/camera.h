#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "uniformObject.h"

namespace vkl {
enum class CameraType {
    LOOKAT,
    FIRSTPERSON,
};

enum class CameraDirection {
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class Camera : public UniformObject {
public:
    Camera(IdType id);
    ~Camera() override = default;

    void load() override;
    void update() override;

    void setPosition(glm::vec4 position);
    void setAspectRatio(float aspectRatio);
    void setPerspective(float fov, float aspect, float znear, float zfar);
    void rotate(glm::vec3 delta);
    void setRotation(glm::vec3 rotation);
    void setTranslation(glm::vec3 translation);
    void translate(glm::vec3 delta);

    void setType(CameraType type);

    void setRotationSpeed(float rotationSpeed);
    void setMovementSpeed(float movementSpeed);

    bool  isMoving() const;
    float getNearClip() const;
    float getFarClip() const;
    float getRotationSpeed() const;
    void  processMovement(float deltaTime);

    void setMovement(CameraDirection direction, bool flag);
    void setFlipY(bool val);

private:
    void updateViewMatrix();
    void updateAspectRatio(float aspect);

private:
    std::unordered_map<CameraDirection, bool> keys{
        {CameraDirection::LEFT, false},
        {CameraDirection::RIGHT, false},
        {CameraDirection::UP, false},
        {CameraDirection::DOWN, false}};

    CameraType _cameraType;

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

    float _fov;
    float _znear;
    float _zfar;
};
} // namespace vkl
#endif
