#pragma once
#include <axis_all.h>

class CharacterControllerDemoScript : public Scriptable
{
public:
    inline static int s_JumpCount = 0;
    float speed = 8.0f;

    void OnUpdate(float dt) override
    {
        (void)dt;
        if (!HasComponent<CharacterControllerComponent>())
            return;

        auto& cc = GetComponent<CharacterControllerComponent>();
        auto* io = Resolve<IOHandler>();
        const auto keyHeld = [io](Key key) { return io && io->GetKeyboard().GetKey(key); };
        const auto keyDown = [io](Key key) { return io && io->GetKeyboard().IsKeyDown(key); };

        glm::vec3 move(0.0f);
        if (GetAction("PlayerForward") || keyHeld(Key::W))
            move.z -= 1.0f;
        if (GetAction("PlayerBackward") || keyHeld(Key::S))
            move.z += 1.0f;
        if (GetAction("PlayerLeft") || keyHeld(Key::A))
            move.x -= 1.0f;
        if (GetAction("PlayerRight") || keyHeld(Key::D))
            move.x += 1.0f;

        const bool sprintHeld = keyHeld(Key::LeftShift) || keyHeld(Key::RightShift);
        const bool slowHeld = keyHeld(Key::LeftControl) || keyHeld(Key::RightControl);
        float currentSpeed = speed * (sprintHeld ? 1.75f : 1.0f);
        if (slowHeld)
            currentSpeed *= 0.35f;
        cc.useVelocity = true;
        cc.velocity = glm::length(move) > 0.001f ? glm::normalize(move) * currentSpeed : glm::vec3(0.0f);
        cc.walkDirection = glm::vec3(0.0f);

        if ((GetActionDown("PlayerJump") || keyDown(Key::Space)) && cc.isOnGround)
        {
            cc.jumpRequested = true;
            ++s_JumpCount;
        }
    }
};
