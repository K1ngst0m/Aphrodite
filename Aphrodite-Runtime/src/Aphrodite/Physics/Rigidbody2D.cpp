//
// Created by npchitman on 7/7/21.
//

#include "Rigidbody2D.h"

#include "Physics2D.h"

namespace Aph {
    Rigidbody2D::Rigidbody2D(const glm::vec2& position, const float rotation,
                             const Aph::Rigidbody2D::Rigidbody2DSpecification& specification)
        : m_Rotation(rotation) {
        m_Position.Set(position.x, position.y);

        b2BodyDef bodyDef;
        bodyDef.position = m_Position;

        m_Body2D = Physics2D::GetWorld()->CreateBody(&bodyDef);
        m_Body2D->SetTransform(m_Position, m_Rotation);

        SetSpecification(specification);
    }

    void Rigidbody2D::SetRuntimeTransform(const glm::vec2& position, float rotation)
    {
        APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
        m_Rotation = rotation;

        m_Position.Set(position.x, position.y);
        m_Body2D->SetTransform(m_Position, m_Rotation);
    }

    void Rigidbody2D::SetSpecification (const Rigidbody2D::Rigidbody2DSpecification& specification) {
        m_Specification = CreateRef<Rigidbody2DSpecification>(specification);

        SetType(m_Specification->Type);
        SetLinearDamping(m_Specification->LinearDamping);
        SetAngularDamping(m_Specification->AngularDamping);
        SetGravityScale(m_Specification->GravityScale);
        SetCollisionDetection(m_Specification->CollisionDetection);
        SetSleepingMode(m_Specification->SleepingMode);
        SetFreezeRotation(m_Specification->FreezeRotationZ);
    }
}// namespace Aph