//
// Created by npchitman on 6/28/21.
//

#ifndef HAZEL_ENGINE_SCRIPTABLEENTITY_H
#define HAZEL_ENGINE_SCRIPTABLEENTITY_H

#include "Hazel/Scene/Entity.h"

namespace Hazel{
    class ScriptableEntity {
    public:
        ScriptableEntity() = default;
        virtual ~ScriptableEntity() = default;

        explicit ScriptableEntity(const Entity& mEntity) : m_Entity(mEntity) {}
        template<typename T>
        T& GetComponent(){
            return m_Entity.GetComponent<T>();
        }

    protected:
        virtual void OnCreate() {}
        virtual void OnDestroy() {}
        virtual void OnUpdate(Timestep ts){}

    private:
        Entity m_Entity;
        friend class Scene;
    };
}


#endif//HAZEL_ENGINE_SCRIPTABLEENTITY_H