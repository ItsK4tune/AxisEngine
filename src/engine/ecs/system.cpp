#include <engine/ecs/system.h>

// #include <engine/utils/BulletGLM.h>

size_t Scene::createEntity()
{
    transforms.emplace_back();
    renderers.emplace_back();
    rigidbodies.emplace_back();
    animators.emplace_back();
    return transforms.size() - 1;
}

void PhysicsSystem::Update(Scene &scene)
{
    for (size_t i = 0; i < scene.rigidbodies.size(); ++i)
    {
        auto &rb = scene.rigidbodies[i];
        auto &transform = scene.transforms[i];

        if (rb.body)
        {
            btTransform trans;
            if (rb.body->getMotionState())
            {
                rb.body->getMotionState()->getWorldTransform(trans);
            }
            else
            {
                trans = rb.body->getWorldTransform();
            }

            transform.position = BulletGLMHelpers::convert(trans.getOrigin());
            transform.rotation = BulletGLMHelpers::convert(trans.getRotation());
        }
    }
}

void AnimationSystem::Update(Scene &scene, float dt)
{
    for (size_t i = 0; i < scene.animators.size(); ++i)
    {
        if (scene.animators[i].animator)
        {
            scene.animators[i].animator->UpdateAnimation(dt);
        }
    }
}

void RenderSystem::Render(Scene &scene, Shader &shader)
{
    for (size_t i = 0; i < scene.renderers.size(); ++i)
    {
        if (scene.renderers[i].model)
        {
            shader.use();

            glm::mat4 modelMatrix = scene.transforms[i].GetTransformMatrix();
            shader.setMat4("model", modelMatrix);

            if (scene.animators[i].animator)
            {
                auto transforms = scene.animators[i].animator->GetFinalBoneMatrices();
                for (int j = 0; j < transforms.size(); ++j)
                {
                    shader.setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                }
            }
            else
            {
                // Reset bone matrices nếu không có animation (để tránh lỗi shader)
                // Hoặc dùng shader riêng cho static mesh
            }

            scene.renderers[i].model->Draw(shader);
        }
    }
}