#pragma once
#include <vector>
#include <engine/ecs/component.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/shader.h>

using EntityID = size_t;

struct Scene
{
    std::vector<TransformComponent> transforms;
    std::vector<MeshRendererComponent> renderers;
    std::vector<RigidBodyComponent> rigidbodies;
    std::vector<AnimationComponent> animators;

    size_t createEntity();
};

class PhysicsSystem {
public:
    void Update(Scene& scene);
};

class AnimationSystem
{
public:
    void Update(Scene &scene, float dt);
};

class RenderSystem
{
public:
    void Render(Scene &scene, Shader &shader);
};