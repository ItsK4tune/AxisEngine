#include <game/states/placeholder_state.h>

#include <engine/core/application.h>
#include <game/scripts/camera_controller.h>
#include <iostream>

void PlaceHolderState::OnEnter()
{
    m_App->GetSceneManager().LoadScene("scenes/placeholder.scene");

    auto view = m_App->GetScene().registry.view<InfoComponent>();
    for (auto entity : view)
    {
        if (view.get<InfoComponent>(entity).name == "FPSLabel")
        {
            fpsEntity = entity;
            break;
        }
    }

    auto anotherview = m_App->GetScene().registry.view<UITransformComponent>();
    for (auto e : anotherview)
    {
        auto &interact = m_App->GetScene().registry.emplace_or_replace<UIInteractiveComponent>(e);
        interact.onClick = [](entt::entity)
        { std::cout << "Button Clicked!\n"; };
    }

    auto anotheranotherview = m_App->GetScene().registry.view<AnimationComponent, InfoComponent>();
    for (auto entity : anotheranotherview)
    {
        auto [anim, info] = anotheranotherview.get<AnimationComponent, InfoComponent>(entity);

        anim.animator->AddAnimation("dyingAnim", m_App->GetResourceManager().GetAnimation("dyingAnim"));
    }

    auto camEntity = m_App->GetScene().GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto &script = m_App->GetScene().registry.emplace<ScriptComponent>(camEntity);
        script.Bind<CameraController>();
    }
}

void PlaceHolderState::OnUpdate(float dt)
{
    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_P))
    {
        std::cout << "Pause Pressed\n";
        isPaused = !isPaused;
    }

    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_O))
    {

        CursorMode m_CurrentCursorMode = NextCursorMode(m_App->GetMouse().GetCursorMode());

        m_App->GetMouse().SetCursorMode(m_CurrentCursorMode);

        std::cout << "Cursor Mode Changed To: ";
        switch (m_CurrentCursorMode)
        {
        case CursorMode::Normal:
            std::cout << "Normal\n";
            break;
        case CursorMode::Hidden:
            std::cout << "Hidden\n";
            break;
        case CursorMode::Locked:
            std::cout << "Locked\n";
            break;
        case CursorMode::LockedCenter:
            std::cout << "LockedCenter\n";
            break;
        }
    }

    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_I))
    {
        m_App->GetSceneManager().LoadScene("scenes/placeholder2.scene");
    }

    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_U))
    {
        m_App->GetSceneManager().UnloadScene("scenes/placeholder2.scene");
    }

    if (m_App->GetKeyboard().IsKeyDown(GLFW_KEY_Y))
    {
        auto anotheranotherview = m_App->GetScene().registry.view<AnimationComponent, InfoComponent>();
        for (auto entity : anotheranotherview)
        {
            auto [anim, info] = anotheranotherview.get<AnimationComponent, InfoComponent>(entity);

            anim.animator->PlayAnimation("dyingAnim");
        }
    }

    m_App->GetUIInteractSystem().Update(
        m_App->GetScene(),
        dt,
        m_App->GetMouse());

    if (isPaused)
        return;

    m_App->GetScriptSystem().Update(m_App->GetScene(), dt, m_App);

    m_App->GetPhysicsWorld().Update(dt);
    m_App->GetPhysicsSystem().Update(m_App->GetScene());

    m_App->GetAnimationSystem().Update(m_App->GetScene(), dt);

    static float timeAccumulator = 0.0f;
    static int frameCount = 0;

    timeAccumulator += dt;
    frameCount++;

    if (timeAccumulator >= 0.5f)
    {
        int fps = static_cast<int>(frameCount / timeAccumulator);
        std::string fpsText = "FPS: " + std::to_string(fps);

        if (fpsEntity != entt::null && m_App->GetScene().registry.valid(fpsEntity))
        {
            auto &txt = m_App->GetScene().registry.get<UITextComponent>(fpsEntity);
            txt.text = "FPS: " + std::to_string(fps);
        }

        timeAccumulator = 0.0f;
        frameCount = 0;
    }
}

void PlaceHolderState::OnRender()
{
    m_App->GetRenderSystem().Render(m_App->GetScene());

    m_App->GetUIRenderSystem().Render(
        m_App->GetScene(),
        (float)m_App->GetWidth(),
        (float)m_App->GetHeight());
}

void PlaceHolderState::OnExit()
{
    m_App->GetSceneManager().UnloadScene("scenes/placeholder.scene");
}

CursorMode PlaceHolderState::NextCursorMode(CursorMode mode)
{
    switch (mode)
    {
    case CursorMode::Normal:
        return CursorMode::Hidden;
    case CursorMode::Hidden:
        return CursorMode::Locked;
    case CursorMode::Locked:
        return CursorMode::LockedCenter;
    case CursorMode::LockedCenter:
        return CursorMode::Normal;
    }
    return CursorMode::Normal;
}