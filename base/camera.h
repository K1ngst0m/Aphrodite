#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum class CameraMovementEnum: uint8_t {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera
{
public:
    // camera Attributes
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;
    // euler Angles
    float m_yaw;
    float m_pitch;
    // camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;
    float m_aspect;

    // constructor with vectors
    Camera(float aspect, glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW,
           float pitch = PITCH);
    // constructor with scalar values
    Camera(float aspect, float posX, float posY, float posZ, float upX, float upY, float upZ,
           float yaw, float pitch);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;

    void move(CameraMovementEnum direction, float deltaTime);

    void ProcessMouseMovement(float xoffset, float yoffset,
                              bool constrainPitch = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

    // Default camera values
    constexpr static float YAW         = -90.0f;
    constexpr static float PITCH       =  0.0f;
    constexpr static float SPEED       =  2.5f;
    constexpr static float SENSITIVITY =  0.1f;
    constexpr static float ZOOM        =  45.0f;
    constexpr static float NEAR         =  0.01f;
    constexpr static float FAR        =  100.0f;
};
#endif
