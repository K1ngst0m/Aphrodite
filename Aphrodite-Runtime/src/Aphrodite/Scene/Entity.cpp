//
// Created by npchitman on 6/27/21.
//

#include "Entity.h"

#include "pch.h"

namespace Aph {

    Entity::Entity(entt::entity handle, Scene *scene) : m_EntityHandle(handle),
                                                        m_Scene(scene) { }
}// namespace Aph
