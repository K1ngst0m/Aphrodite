#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "uniformObject.h"

namespace vkl {

enum class LightType {
    POINT,
    DIRECTIONAL,
    SPOT,
};

class Light : public UniformObject {
public:
    static std::shared_ptr<Light> Create();
    Light(IdType id);
    ~Light() override;

    void setPosition(glm::vec3 value);
    void setDirection(glm::vec3 value);

    void setColor(glm::vec3 value);
    void setType(LightType type);

    void load() override;
    void update(float deltaTime) override;

private:
    glm::vec3 _color;
    glm::vec3 _position;
    glm::vec3 _direction;
    LightType _type;
};
} // namespace vkl

#endif // VKLLIGHT_H_
