#include <engine/ecs/system.h>

void PhysicsSystem::Update(Scene &scene)
{
    auto view = scene.registry.view<RigidBodyComponent, TransformComponent>();

    for (auto entity : view)
    {
        auto &rb = view.get<RigidBodyComponent>(entity);
        auto &transform = view.get<TransformComponent>(entity);

        if (rb.body)
        {
            if (rb.body->isKinematicObject() || !rb.body->isActive())
                continue;

            btTransform trans;
            if (rb.body->getMotionState())
                rb.body->getMotionState()->getWorldTransform(trans);
            else
                trans = rb.body->getWorldTransform();

            transform.position = BulletGLMHelpers::convert(trans.getOrigin());
            transform.rotation = BulletGLMHelpers::convert(trans.getRotation());
        }
    }
}
