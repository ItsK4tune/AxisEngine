#include <ecs/logic/ui_interact_system.h>
#include <core/logic/config_manager.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/ui_components.h>
#include <platform/logic/io_handler.h>
#include <algorithm>

namespace
{
struct UIRect
{
    glm::vec2 pos = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(0.0f);
};

glm::vec2 EvalUIVec(const glm::vec2& value, const glm::bvec2& isPercent, const glm::vec2& reference)
{
    return glm::vec2(isPercent.x ? (value.x * 0.01f * reference.x) : value.x,
                     isPercent.y ? (value.y * 0.01f * reference.y) : value.y);
}

UIRect CalculateUIRect(entt::registry& registry, entt::entity entity, float screenWidth, float screenHeight)
{
    auto* transform = registry.try_get<UITransformComponent>(entity);
    if (!transform)
        return {};

    glm::vec2 parentSize(screenWidth, screenHeight);
    glm::vec2 parentPos(0.0f);
    if (auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
    {
        if (hierarchy->parent != entt::null && registry.valid(hierarchy->parent) &&
            registry.all_of<UITransformComponent>(hierarchy->parent))
        {
            UIRect parentRect = CalculateUIRect(registry, hierarchy->parent, screenWidth, screenHeight);
            parentPos = parentRect.pos;
            parentSize = parentRect.size;
        }
    }

    glm::vec2 anchorMin = EvalUIVec(transform->anchorMin, transform->anchorMinIsPercent, glm::vec2(1.0f));
    glm::vec2 anchorMax = EvalUIVec(transform->anchorMax, transform->anchorMaxIsPercent, glm::vec2(1.0f));

    glm::vec2 finalMin = parentPos + anchorMin * parentSize +
                         EvalUIVec(transform->offsetMin, transform->offsetMinIsPercent, parentSize);
    glm::vec2 finalMax = parentPos + anchorMax * parentSize +
                         EvalUIVec(transform->offsetMax, transform->offsetMaxIsPercent, parentSize);

    glm::vec2 pos = EvalUIVec(transform->position, transform->positionIsPercent, parentSize);
    finalMin += pos;
    finalMax += pos;

    glm::vec2 size = EvalUIVec(transform->size, transform->sizeIsPercent, parentSize);
    if (anchorMin.x == anchorMax.x)
        finalMax.x = finalMin.x + size.x;
    if (anchorMin.y == anchorMax.y)
        finalMax.y = finalMin.y + size.y;

    return {finalMin, finalMax - finalMin};
}

bool Contains(const UIRect& rect, const glm::vec2& point)
{
    return point.x >= rect.pos.x && point.x <= rect.pos.x + rect.size.x && point.y >= rect.pos.y &&
           point.y <= rect.pos.y + rect.size.y;
}

UIRect ApplyVisualScale(const UIRect& rect, const UITransformComponent& transform, float scale)
{
    if (scale <= 0.0f || scale == 1.0f)
        return rect;

    const glm::vec2 pivot = rect.pos + transform.pivot * rect.size;
    const glm::vec2 scaledSize = rect.size * scale;
    return {pivot - transform.pivot * scaledSize, scaledSize};
}
}  // namespace

REGISTER_SYSTEM(UIInteractSystem)

void UIInteractSystem::Initialize()
{
    ServiceLocator::Instance().Register<UIInteractSystem>(this);
}

void UIInteractSystem::Update(Scene& scene, float dt)
{
    if (!m_Enabled)
        return;

    auto* io = ServiceLocator::Instance().Resolve<IOHandler>();
    if (!io)
        return;

    const float screenWidth = (std::max)(1.0f, static_cast<float>(io->GetMonitorManager().GetWidth()));
    const float screenHeight = (std::max)(1.0f, static_cast<float>(io->GetMonitorManager().GetHeight()));
    float referenceWidth = 1920.0f;
    float referenceHeight = 1080.0f;
    if (auto* config = ServiceLocator::Instance().Resolve<ConfigManager>())
    {
        referenceWidth = (std::max)(1.0f, config->GetConfig().uiReferenceWidth);
        referenceHeight = (std::max)(1.0f, config->GetConfig().uiReferenceHeight);
    }

    const float scaleFactor = (std::min)(screenWidth / referenceWidth, screenHeight / referenceHeight);
    if (scaleFactor <= 0.0f)
        return;
    const float virtualWidth = screenWidth / scaleFactor;
    const float virtualHeight = screenHeight / scaleFactor;

    const auto& mouse = io->GetMouse();
    const glm::vec2 mousePos(static_cast<float>(mouse.GetLastX()) / scaleFactor,
                             static_cast<float>(mouse.GetLastY()) / scaleFactor);
    const bool leftDown = mouse.IsLeftButtonPressed();

    std::vector<entt::entity> entities;
    auto view = scene.registry.view<UITransformComponent, UIInteractiveComponent, InfoComponent>();
    for (auto entity : view)
    {
        const auto& info = view.get<InfoComponent>(entity);
        const auto& interactive = view.get<UIInteractiveComponent>(entity);
        if (info.isActive && interactive.interactable)
            entities.push_back(entity);
    }

    std::sort(entities.begin(), entities.end(), [&](entt::entity a, entt::entity b) {
        return scene.registry.get<UITransformComponent>(a).zIndex >
               scene.registry.get<UITransformComponent>(b).zIndex;
    });

    entt::entity topHovered = entt::null;
    for (auto entity : entities)
    {
        UIRect rect = CalculateUIRect(scene.registry, entity, virtualWidth, virtualHeight);
        if (auto* animation = scene.registry.try_get<UIAnimationComponent>(entity))
            rect = ApplyVisualScale(rect, scene.registry.get<UITransformComponent>(entity), animation->visualScale);
        if (Contains(rect, mousePos))
        {
            topHovered = entity;
            break;
        }
    }

    const float safeDt = (std::max)(0.0f, dt);
    for (auto entity : entities)
    {
        auto& interactive = scene.registry.get<UIInteractiveComponent>(entity);
        const bool hovered = entity == topHovered;
        interactive.clicked = false;

        if (hovered && !interactive.hovered)
        {
            interactive.hovered = true;
            if (interactive.onHoverEnter)
                interactive.onHoverEnter(entity);
        }
        if (hovered)
        {
            if (interactive.onHoverStay)
                interactive.onHoverStay(entity);
        }
        else if (interactive.hovered)
        {
            interactive.hovered = false;
            if (interactive.onHoverExit)
                interactive.onHoverExit(entity);
        }

        if (hovered && leftDown && !interactive.pressed)
        {
            interactive.pressed = true;
            interactive.holdTime = 0.0f;
            if (interactive.onPressed)
                interactive.onPressed(entity);
        }

        if (interactive.pressed && leftDown)
        {
            interactive.holdTime += safeDt;
        }
        else if (interactive.pressed && !leftDown)
        {
            interactive.pressed = false;
            if (interactive.onReleased)
                interactive.onReleased(entity);
            if (hovered)
            {
                interactive.clicked = true;
                if (interactive.onClick)
                    interactive.onClick(entity);
            }
            interactive.holdTime = 0.0f;
        }

        if (auto* animation = scene.registry.try_get<UIAnimationComponent>(entity))
        {
            if (!animation->enabled)
            {
                animation->visualScale = 1.0f;
                continue;
            }

            const float t = glm::clamp(animation->transitionSpeed * safeDt, 0.0f, 1.0f);
            const glm::vec4 targetColor =
                interactive.pressed ? animation->pressedColor : (interactive.hovered ? animation->hoverColor
                                                                                     : animation->normalColor);
            const float targetScale =
                interactive.pressed ? animation->pressedScale : (interactive.hovered ? animation->hoverScale
                                                                                     : animation->normalScale);

            if (animation->animateColor)
            {
                if (auto* renderer = scene.registry.try_get<UIRendererComponent>(entity))
                    renderer->color = glm::mix(renderer->color, targetColor, t);
                if (auto* text = scene.registry.try_get<UITextComponent>(entity))
                    text->color = glm::mix(text->color, targetColor, t);
            }

            if (animation->animateScale)
            {
                animation->currentScale = glm::mix(animation->currentScale, targetScale, t);
                animation->visualScale = animation->currentScale;
            }
            else
            {
                animation->visualScale = 1.0f;
            }
        }
    }
}

std::vector<entt::id_type> UIInteractSystem::GetReadComponents() const
{
    return {entt::type_id<UITransformComponent>().hash(), entt::type_id<HierarchyComponent>().hash(),
            entt::type_id<InfoComponent>().hash()};
}

std::vector<entt::id_type> UIInteractSystem::GetWriteComponents() const
{
    return {entt::type_id<UIInteractiveComponent>().hash(), entt::type_id<UIAnimationComponent>().hash(),
            entt::type_id<UIRendererComponent>().hash(), entt::type_id<UITextComponent>().hash()};
}
