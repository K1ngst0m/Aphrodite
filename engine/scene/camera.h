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

struct Camera : public Object
{
    Camera(CameraType cameraType);
    ~Camera() override = default;

    CameraType m_cameraType{CameraType::UNDEFINED};

    float     m_aspect{0.0f};
    glm::mat4 m_projection{1.0f};
    glm::mat4 m_view{1.0f};

    union
    {
        struct
        {
            float left{};
            float right{};
            float bottom{};
            float top{};
            float front{};
            float back{};
        } m_ortho;

        struct
        {
            float fov{60.0f};
            float znear{96.0f};
            float zfar{0.01f};
        } m_perspective;
    };
};

enum class Direction
{
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

class CameraController
{
public:
    static std::unique_ptr<CameraController> Create(Camera* camera)
    {
        auto instance = std::unique_ptr<CameraController>(new CameraController(camera));
        return instance;
    }

    void move(Direction direction, bool flag) { m_directions[direction] = flag; }

    void rotate(glm::vec3 delta) { m_direction += delta * m_rotationSpeed; }
    void translate(glm::vec3 delta) { m_position += delta * m_movementSpeed; }
    void update(float deltaTime);

private:
    CameraController(Camera* camera) : m_camera{camera} {}
    void updateProj();
    void updateView();

    Camera* m_camera{};

    bool      m_flipY{true};
    glm::vec3 m_direction{0.0f, 180.0f, 0.0f};
    glm::vec3 m_position{0.0f, 0.0f, -3.0f};
    float     m_rotationSpeed{0.1f};
    float     m_movementSpeed{2.5f};

    std::unordered_map<Direction, bool> m_directions{
        {Direction::LEFT, false}, {Direction::RIGHT, false}, {Direction::UP, false}, {Direction::DOWN, false}};
};

}  // namespace aph
#endif
