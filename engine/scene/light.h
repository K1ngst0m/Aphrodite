#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "uniformObject.h"

namespace vkl {

    enum class LightType {
        POINT,
        DIRECTIONAL,
    };

    class Light : public UniformObject {
    public:
        Light(IdType id);
        ~Light() override;

        void setPosition(glm::vec4 value);
        void setDirection(glm::vec4 value);

        void setDiffuse(glm::vec4 value);
        void setSpecular(glm::vec4 value);
        void setType(LightType type);

        void load() override;
        void update(float deltaTime) override;

    private:
        // common
        glm::vec4 _diffuse;
        glm::vec4 _specular;

        // point
        glm::vec4 _position;
        glm::vec4 _attenuationFactor;

        // directional
        glm::vec4 _direction;

        LightType _type;
    };
}

#endif // VKLLIGHT_H_
