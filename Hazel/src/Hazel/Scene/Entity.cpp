//
// Created by npchitman on 6/27/21.
//

#include "Entity.h"

#include "hzpch.h"

namespace Hazel {

    Entity::Entity(entt::entity handle, Scene *scene) : m_EntityHandle(handle),
                                                        m_Scene(scene) {
    }
}// namespace Hazel
