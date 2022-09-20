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

        void load() override;
        void update() override;

        void *getData() override;
        uint32_t getDataSize() override ;

        void destroy() override;

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

    private:
        SceneManager * _manager;

        uint32_t dataSize = 0;
        void * data = nullptr;
        bool isloaded = false;
    };
}

#endif // VKLLIGHT_H_
