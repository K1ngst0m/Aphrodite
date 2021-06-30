//
// Created by npchitman on 6/28/21.
//

#ifndef Aphrodite_ENGINE_SCENESERIALIZER_H
#define Aphrodite_ENGINE_SCENESERIALIZER_H

#include "Scene.h"

namespace Aph {
    class SceneSerializer {
    public:
        explicit SceneSerializer(const Ref<Scene>& scene);

        void Serialize(const std::string& filepath);

        void SerializeRuntime(const std::string& filepath);

        bool Deserialize(const std::string& filepath);

        static bool DeserializeRuntime(const std::string& filepath);

    private:
        Ref<Scene> m_Scene;
    };
}


#endif//Aphrodite_ENGINE_SCENESERIALIZER_H
