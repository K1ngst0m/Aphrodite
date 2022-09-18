#ifndef VKLLIGHT_H_
#define VKLLIGHT_H_

#include "object.h"

namespace vkl {

    enum class LightType {
        POINT,
        DIRECTIONAL,
    };

    class Light : public UniformBufferObject {
    public:
        Light(SceneManager *manager);

        void setPosition(glm::vec4 value);
        void setDirection(glm::vec4 value);

        void setDiffuse(glm::vec4 value);
        void setSpecular(glm::vec4 value);
        void setType(LightType type);

        void load(vkl::Device *device);
        void update();

    private:
        SceneManager * _manager;

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
