//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_BOXCOLLIDER2D_H
#define APHRODITE_BOXCOLLIDER2D_H

#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>

#include <glm/glm.hpp>

#include "Rigidbody2D.h"

namespace Aph {
    class BoxCollider2D {
    public:
        BoxCollider2D(Ref<Rigidbody2D>& rigidbody2D, glm::vec2& size, glm::vec2& offset, bool isTrigger);
        ~BoxCollider2D();

        void SetSpecification(glm::vec2& size, glm::vec2& offset, bool isTrigger);

    private:
        void CreateFixture(Ref<Rigidbody2D>& rigidbody2D, glm::vec2& size, glm::vec2& offset, bool isTrigger);

    private:
        b2Vec2 m_Size = b2Vec2(1.0f, 1.0f);
        b2Vec2 m_Offset = b2Vec2(0.0f, 0.0f);
        bool m_IsTrigger = false;

        Ref<Rigidbody2D> m_Rigidbody2D;
        b2Fixture* m_Fixture;
    };
}// namespace Aph


#endif//APHRODITE_BOXCOLLIDER2D_H
