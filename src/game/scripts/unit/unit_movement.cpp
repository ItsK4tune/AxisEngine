#include <game/scripts/unit/unit_movement.h>

void UnitMovement::StartPath(const std::vector<HexCoord> &pathHex)
{
    m_MovePath.clear();
    for (auto &h : pathHex)
        m_MovePath.push_back(HexMath::HexToWorld(h));
    m_PathIndex = 0;
    isMoving = true;
}

bool UnitMovement::Update(float dt, TransformComponent &trans, RigidBodyComponent *rb)
{
    if (!isMoving)
        return false;

    if (m_PathIndex < m_MovePath.size())
    {
        glm::vec3 target = m_MovePath[m_PathIndex];
        glm::vec3 dir = target - trans.position;

        if (glm::length2(dir) < 0.01f)
        {
            trans.position = target;
            m_PathIndex++;
            if (m_PathIndex >= m_MovePath.size())
                isMoving = false;
        }
        else
        {
            trans.position += glm::normalize(dir) * moveSpeed * dt;
        }

        if (rb && rb->body)
        {
            btTransform t = rb->body->getWorldTransform();
            t.setOrigin(BulletGLMHelpers::convert(trans.position));
            rb->body->setWorldTransform(t);
            if (rb->body->getMotionState())
                rb->body->getMotionState()->setWorldTransform(t);
            rb->body->activate(true);
        }
        return true;
    }

    isMoving = false;
    return false;
}