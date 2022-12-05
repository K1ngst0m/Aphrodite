#include "camera.h"

#include <glm/gtx/string_cast.hpp>

namespace vkl
{

struct CameraDataLayout
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 position;
};

void Camera::load()
{
    updateViewMatrix();
    dataSize = sizeof(CameraDataLayout);
    data = std::make_shared<CameraDataLayout>();
    auto pData = std::static_pointer_cast<CameraDataLayout>(data);
    pData->view = _matrices.view;
    pData->proj = _matrices.perspective;
    pData->position = glm::vec4(_position, 1.0f);
}

void Camera::update(float deltaTime)
{
    processMovement(deltaTime);
    updateViewMatrix();

    updated = true;
    auto pData = std::static_pointer_cast<CameraDataLayout>(data);
    pData->view = _matrices.view;
    pData->proj = _matrices.perspective;
    pData->position = glm::vec4(_position, 1.0f);
}

void Camera::setAspectRatio(float aspect)
{
    _matrices.perspective = glm::perspective(glm::radians(_fov), aspect, _znear, _zfar);
    if(_flipY)
    {
        _matrices.perspective[1][1] *= -1.0f;
    }
}

void Camera::setPerspective(float fov, float aspect, float znear, float zfar)
{
    _fov = fov;
    _znear = znear;
    _zfar = zfar;
    _matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
    if(_flipY)
    {
        _matrices.perspective[1][1] *= -1.0f;
    }
};

void Camera::updateViewMatrix()
{
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, glm::radians(_rotation.x * (_flipY ? -1.0f : 1.0f)),
                       glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = -_position;
    if(_flipY)
    {
        translation.y *= -1.0f;
    }
    transM = glm::translate(glm::mat4(1.0f), translation);

    if(_cameraType == CameraType::FIRSTPERSON)
    {
        _matrices.view = rotM * transM;
    }
    else
    {
        _matrices.view = transM * rotM;
    }

    updated = true;
};

bool Camera::isMoving() const
{
    return std::any_of(keys.begin(), keys.end(), [](const auto &key) -> bool { return key.second; });
}

void Camera::processMovement(float deltaTime)
{
    updated = false;
    if(_cameraType == CameraType::FIRSTPERSON)
    {
        if(isMoving())
        {
            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(_rotation.x)) * sin(glm::radians(_rotation.y));
            camFront.y = sin(glm::radians(_rotation.x));
            camFront.z = cos(glm::radians(_rotation.x)) * cos(glm::radians(_rotation.y));
            camFront = glm::normalize(camFront);

            float moveSpeed = deltaTime * _movementSpeed;

            if(keys[Direction::UP])
                _position += camFront * moveSpeed;
            if(keys[Direction::DOWN])
                _position -= camFront * moveSpeed;
            if(keys[Direction::LEFT])
                _position -=
                    glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            if(keys[Direction::RIGHT])
                _position +=
                    glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
        }
    }
};

std::shared_ptr<Camera> Camera::Create()
{
    auto instance = std::make_shared<Camera>(Id::generateNewId<Camera>());
    return instance;
}
}  // namespace vkl
