//
// Created by npchitman on 6/28/21.
//

#ifndef Aphrodite_ENGINE_SCENEHIERARCHYPANEL_H
#define Aphrodite_ENGINE_SCENEHIERARCHYPANEL_H

#include "Aphrodite/Core/Base.h"
#include "Aphrodite/Scene/Entity.h"
#include "Aphrodite/Scene/Scene.h"

namespace Aph::Editor {
    class SceneHierarchy {
    public:
        SceneHierarchy() = default;
        explicit SceneHierarchy(const Ref<Scene>& context);

        void SetContext(const Ref<Scene>& context);

        void OnImGuiRender();

        Entity GetSelectedEntity() const { return m_SelectionContext; }
        void SetSelectedEntity(Entity entity);

    private:
        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
    };
}// namespace Aph


#endif//Aphrodite_ENGINE_SCENEHIERARCHYPANEL_H
