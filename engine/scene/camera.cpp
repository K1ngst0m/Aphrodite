#include "camera.h"

namespace vkl {

struct CameraDataLayout {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 position;
    CameraDataLayout(glm::mat4 view, glm::mat4 proj, glm::mat4 viewproj, glm::vec4 viewpos)
        : view(view), proj(proj), viewProj(viewproj), position(viewpos) {
    }
};

Camera::Camera(SceneManager *manager)
    : UniformBufferObject(manager) {
}

void Camera::load() {
    dataSize = sizeof(CameraDataLayout);
    update();
}

void Camera::update() {
    updated = true;
    data    = std::make_shared<CameraDataLayout>(
        this->matrices.view,
        this->matrices.perspective,
        this->matrices.perspective * this->matrices.view,
        glm::vec4(this->position, 1.0f));
}

void Camera::setPosition(glm::vec4 position) {
    this->position = position;
    updateViewMatrix();
}
void Camera::setAspectRatio(float aspect) {
    matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    if (flipY) {
        matrices.perspective[1][1] *= -1.0f;
    }
}
void Camera::setPerspective(float fov, float aspect, float znear, float zfar) {
    this->fov            = fov;
    this->znear          = znear;
    this->zfar           = zfar;
    matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    if (flipY) {
        matrices.perspective[1][1] *= -1.0f;
    }
};
void Camera::setType(CameraType type) {
    this->type = type;
}
void Camera::setMovementSpeed(float movementSpeed) {
    this->movementSpeed = movementSpeed;
}
void Camera::setRotationSpeed(float rotationSpeed) {
    this->rotationSpeed = rotationSpeed;
}
void Camera::updateViewMatrix() {
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = position;
    if (flipY) {
        translation.y *= -1.0f;
    }
    transM = glm::translate(glm::mat4(1.0f), translation);

    if (type == CameraType::FIRSTPERSON) {
        matrices.view = rotM * transM;
    } else {
        matrices.view = transM * rotM;
    }

    viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

    updated = true;
};
bool Camera::isMoving() const {
    return keys.left || keys.right || keys.up || keys.down;
}
float Camera::getNearClip() const {
    return znear;
}
float Camera::getFarClip() const {
    return zfar;
}
void Camera::rotate(glm::vec3 delta) {
    this->rotation += delta;
    updateViewMatrix();
}
void Camera::setRotation(glm::vec3 rotation) {
    this->rotation = rotation;
    updateViewMatrix();
}
void Camera::setTranslation(glm::vec3 translation) {
    this->position = translation;
    updateViewMatrix();
};
void Camera::translate(glm::vec3 delta) {
    this->position += delta;
    updateViewMatrix();
}
void Camera::processMove(float deltaTime) {
    updated = false;
    if (type == CameraType::FIRSTPERSON) {
        if (isMoving()) {
            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
            camFront.y = sin(glm::radians(rotation.x));
            camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
            camFront   = glm::normalize(camFront);

            float moveSpeed = deltaTime * movementSpeed;

            if (keys.up)
                position += camFront * moveSpeed;
            if (keys.down)
                position -= camFront * moveSpeed;
            if (keys.left)
                position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            if (keys.right)
                position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

            updateViewMatrix();
        }
    }
};
float Camera::getRotationSpeed() const {
    return rotationSpeed;
}
} // namespace vkl
