//
// Created by npchitman on 7/7/21.
//

#ifndef APHRODITE_RIGIDBODY2D_H
#define APHRODITE_RIGIDBODY2D_H

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>

#include <glm/glm.hpp>

#include "Aphrodite/Core/Base.h"

namespace Aph {
    enum class Rigidbody2DType {
        Static = b2BodyType::b2_staticBody,
        Kinematic = b2BodyType::b2_kinematicBody,
        Dynamic = b2BodyType::b2_dynamicBody
    };

    class Rigidbody2D {
    public:
        enum class CollisionDetectionType { Discrete = 0,
                                            Continuous };
        enum class SleepType { NeverSleep = 0,
                               StartAwake,
                               StartAsleep };


        struct Rigidbody2DSpecification {
            Rigidbody2DType Type = Rigidbody2DType::Static;
            float LinearDamping = 0.0f;
            float AngularDamping = 0.0f;
            float GravityScale = 1.0f;

            CollisionDetectionType CollisionDetection = CollisionDetectionType::Discrete;
            SleepType SleepingMode = SleepType::StartAwake;

            bool FreezeRotationZ = false;
        };

    public:
        Rigidbody2D(const glm::vec2& position, float rotation, const Rigidbody2DSpecification& specification);
        ~Rigidbody2D() = default;

        inline glm::vec2& GetRuntimePosition() const {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            return (glm::vec2&) m_Body2D->GetPosition();
        }
        inline float GetRuntimeRotation() const {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            return m_Body2D->GetAngle();
        }
        void SetRuntimeTransform(const glm::vec2& position, float rotation);


        inline Ref<Rigidbody2DSpecification> GetSpecification() const { return m_Specification; }
        void SetSpecification(const Rigidbody2DSpecification& specification);

        // Set properties
        inline void SetType(Rigidbody2DType type) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            m_Body2D->SetType((b2BodyType) type);
        }
        inline void SetLinearDamping(float value) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            m_Body2D->SetLinearDamping(value);
        }
        inline void SetAngularDamping(float value) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            m_Body2D->SetAngularDamping(value);
        }
        inline void SetGravityScale(float value) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            m_Body2D->SetGravityScale(value);
        }
        inline void SetCollisionDetection(CollisionDetectionType type) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");

            switch (type) {
                case CollisionDetectionType::Discrete: {
                    m_Body2D->SetBullet(false);
                    break;
                }
                case CollisionDetectionType::Continuous: {
                    m_Body2D->SetBullet(true);
                    break;
                }
            }
        }
        inline void SetSleepingMode(SleepType type) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");

            switch (type) {
                case SleepType::NeverSleep: {
                    m_Body2D->SetSleepingAllowed(false);
                    m_Body2D->SetAwake(true);
                    break;
                }
                case SleepType::StartAsleep: {
                    m_Body2D->SetSleepingAllowed(true);
                    m_Body2D->SetAwake(false);
                    break;
                }
                case SleepType::StartAwake: {
                    m_Body2D->SetSleepingAllowed(true);
                    m_Body2D->SetAwake(true);
                    break;
                }
            }
        }
        inline void SetFreezeRotation(bool flag) {
            APH_CORE_ASSERT(m_Body2D, "Body2D is not set!");
            m_Body2D->SetFixedRotation(flag);
        }

    private:
        b2Vec2 m_Position;
        float m_Rotation;

        Ref<Rigidbody2DSpecification> m_Specification;

        b2Body* m_Body2D{};

        friend class BoxCollider2D;
    };
}// namespace Aph


#endif//APHRODITE_RIGIDBODY2D_H
