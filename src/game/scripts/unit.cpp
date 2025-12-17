#include <game/scripts/unit.h>

#include <engine/utils/bullet_glm_helpers.h>

void Unit::OnCreate()
{
    if (HasComponent<TransformComponent>())
    {
        auto &trans = GetComponent<TransformComponent>();
        trans.position = HexMath::HexToWorld(gridPos);
        SyncPhysics(trans.position);
    }
}

void Unit::OnUpdate(float dt)
{
    if (isMoving)
    {
        auto &trans = GetComponent<TransformComponent>();

        glm::vec3 dir = targetPos - trans.position;

        float distSq = (dir.x * dir.x) + (dir.z * dir.z);

        if (distSq < 0.01f)
        {
            trans.position.x = targetPos.x;
            trans.position.z = targetPos.z;
            isMoving = false;
        }
        else
        {
            glm::vec3 moveDir = glm::normalize(dir);
            trans.position += moveDir * moveSpeed * dt;
        }

        SyncPhysics(trans.position);
    }
}

void Unit::MoveTo(HexCoord newCoords)
{
    gridPos = newCoords;
    targetPos = HexMath::HexToWorld(newCoords);
    isMoving = true;
    std::cout << "[Unit] Unit moving to World Pos: " << targetPos.x << ", " << targetPos.z << "\n";
}

void Unit::SyncPhysics(const glm::vec3 &pos)
{
    if (HasComponent<RigidBodyComponent>())
    {
        auto &rb = GetComponent<RigidBodyComponent>();
        if (rb.body)
        {
            btTransform t = rb.body->getWorldTransform();
            t.setOrigin(BulletGLMHelpers::convert(pos));
            rb.body->setWorldTransform(t);

            if (rb.body->getMotionState())
            {
                rb.body->getMotionState()->setWorldTransform(t);
            }

            rb.body->activate(true);
        }
    }
}