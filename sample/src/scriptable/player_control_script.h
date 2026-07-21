#pragma once
#include <axis_all.h>
#include <glm/gtx/quaternion.hpp>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#endif

class PlayerControlScript : public Scriptable
{
public:
    float speed = 15.0f;
    bool allowMouseColor = true;
    bool allowKeyboardWhileUI = false;
    float verticalVelocity = 0.0f;
    float groundY = 0.75f;
    bool initializedGround = false;

    void OnUpdate(float dt) override
    {
#ifdef ENABLE_EDITOR
        ImGuiIO* imguiIO = ImGui::GetCurrentContext() ? &ImGui::GetIO() : nullptr;
        const bool keyboardCaptured = imguiIO && imguiIO->WantCaptureKeyboard && !allowKeyboardWhileUI;
        const bool mouseCaptured = imguiIO && imguiIO->WantCaptureMouse;
#else
        const bool keyboardCaptured = false;
        const bool mouseCaptured = false;
#endif

        glm::vec3 move = glm::vec3(0.0f);
        if (!keyboardCaptured)
        {
            if (GetAction("PlayerForward"))
                move.z -= 1.0f;
            if (GetAction("PlayerBackward"))
                move.z += 1.0f;
            if (GetAction("PlayerLeft"))
                move.x -= 1.0f;
            if (GetAction("PlayerRight"))
                move.x += 1.0f;
        }

        bool dirty = false;
        if (glm::length(move) > 0.0f)
        {
            move = glm::normalize(move) * speed * dt;
            if (HasComponent<PositionComponent>())
            {
                auto& pos = GetComponent<PositionComponent>();
                pos.value += move;
                dirty = true;
            }
        }

        if (HasComponent<PositionComponent>())
        {
            auto& pos = GetComponent<PositionComponent>();
            if (!initializedGround)
            {
                groundY = pos.value.y;
                initializedGround = true;
            }

            const bool onGround = pos.value.y <= groundY + 0.001f;
            if (!keyboardCaptured && GetActionDown("PlayerJump") && onGround)
            {
                verticalVelocity = 11.0f;
            }

            verticalVelocity -= 28.0f * dt;
            pos.value.y += verticalVelocity * dt;
            if (pos.value.y < groundY)
            {
                pos.value.y = groundY;
                verticalVelocity = 0.0f;
            }
            dirty = true;
        }

        if (allowMouseColor && !mouseCaptured && GetActionDown("PlayerAction"))
        {
            if (HasComponent<MaterialComponent>())
            {
                auto& mat = GetComponent<MaterialComponent>();
                mat.desc.pbr.roughness = static_cast<float>(rand() % 100) / 100.0f;
                mat.desc.pbr.metallic = static_cast<float>(rand() % 100) / 100.0f;
                mat.desc.emission =
                    glm::vec3(static_cast<float>(rand() % 100) / 30.0f, static_cast<float>(rand() % 100) / 30.0f,
                              static_cast<float>(rand() % 100) / 30.0f);
                mat.gpu.dirty = true;
            }
        }

        if (HasComponent<ScaleComponent>())
        {
            auto& scale = GetComponent<ScaleComponent>();
            scale.value = glm::mix(scale.value, glm::vec3(1.5f), 10.0f * dt);
            dirty = true;
        }

        if (dirty)
            MarkTransformDirty();
    }
};
