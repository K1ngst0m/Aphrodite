#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "object.h"

namespace vkl {
enum class CameraType {
    LOOKAT,
    FIRSTPERSON,
};

class Camera : public UniformBufferObject {
public:
    Camera(SceneManager *manager);
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
    void  processMove(float deltaTime);

    struct
    {
        bool left  = false;
        bool right = false;
        bool up    = false;
        bool down  = false;
    } keys;

private:
    void updateViewMatrix();
    void updateAspectRatio(float aspect);

private:
    CameraType type;

    glm::vec3 rotation = glm::vec3();
    glm::vec3 position = glm::vec3();
    glm::vec4 viewPos  = glm::vec4();

    float rotationSpeed = 1.0f;
    float movementSpeed = 1.0f;

    bool flipY = true;

    struct
    {
        glm::mat4 perspective;
        glm::mat4 view;
    } matrices;

    float fov;
    float znear, zfar;
};
} // namespace vkl
#endif
