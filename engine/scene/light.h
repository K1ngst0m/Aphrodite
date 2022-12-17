#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "object.h"

namespace vkl
{

enum class LightType : uint32_t
{
    POINT = 0,
    DIRECTIONAL = 1,
    SPOT = 2,
};

class Light : public UniformObject
{
public:
    static std::shared_ptr<Light> Create();

    Light(IdType id) : UniformObject(id, ObjectType::LIGHT) {}
    ~Light() override = default;

    void setPosition(glm::vec3 value) { _position = value; }
    void setDirection(glm::vec3 value) { _direction = value; }

    void setColor(glm::vec3 value) { _color = value; }
    void setType(LightType type) { _type = type; }

    void load() override;
    void update(float deltaTime) override;

private:
    glm::vec3 _color = glm::vec3(1.0f);
    glm::vec3 _position = glm::vec3{ 1.2f, 1.0f, 2.0f };
    glm::vec3 _direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    LightType _type = LightType::DIRECTIONAL;
};
}  // namespace vkl

#endif  // VKLLIGHT_H_
