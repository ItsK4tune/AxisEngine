#include <editor/transform_gizmo.h>

#ifdef ENABLE_EDITOR

#include <core/logic/config_manager.h>
#include <core/logic/service_locator.h>
#include <ecs/unit/core_components.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <platform/interface/i_ui_input_capture.h>
#include <scene/logic/scene.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>

namespace
{
struct TransformCommandValue
{
    entt::entity entity = entt::null;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

class TransformEntitiesCommand final : public IEditorCommand
{
public:
    TransformEntitiesCommand(std::vector<TransformCommandValue> before, std::vector<TransformCommandValue> after)
        : m_Before(std::move(before)), m_After(std::move(after))
    {
    }

    const char* GetName() const override { return "Transform gizmo"; }
    bool Redo(Scene& scene) override { return Apply(scene, m_After); }
    bool Undo(Scene& scene) override { return Apply(scene, m_Before); }

private:
    static bool Apply(Scene& scene, const std::vector<TransformCommandValue>& values)
    {
        bool applied = false;
        for (const auto& value : values)
        {
            if (!scene.IsValid(value.entity))
                continue;
            if (auto* position = scene.TryGetComponent<PositionComponent>(value.entity))
                position->prev = position->value = value.position;
            if (auto* rotation = scene.TryGetComponent<RotationComponent>(value.entity))
                rotation->prev = rotation->value = value.rotation;
            if (auto* scale = scene.TryGetComponent<ScaleComponent>(value.entity))
                scale->prev = scale->value = value.scale;
            scene.MarkTransformDirty(value.entity);
            applied = true;
        }
        return applied;
    }

    std::vector<TransformCommandValue> m_Before;
    std::vector<TransformCommandValue> m_After;
};

bool ProjectPoint(const glm::mat4& viewProjection, const glm::vec3& point, float width, float height,
                  glm::vec2& output)
{
    const glm::vec4 clip = viewProjection * glm::vec4(point, 1.0f);
    if (clip.w <= 0.0001f)
        return false;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    output.x = (ndc.x * 0.5f + 0.5f) * width;
    output.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * height;
    return ndc.z >= -1.0f && ndc.z <= 1.0f;
}

float DistanceToSegment(const glm::vec2& point, const glm::vec2& a, const glm::vec2& b)
{
    const glm::vec2 ab = b - a;
    const float denominator = glm::dot(ab, ab);
    if (denominator < 0.0001f)
        return std::numeric_limits<float>::max();
    const float t = std::clamp(glm::dot(point - a, ab) / denominator, 0.0f, 1.0f);
    return glm::length(point - (a + ab * t));
}

glm::vec3 ToLocalPosition(Scene& scene, entt::entity entity, const glm::vec3& worldPosition)
{
    const auto* hierarchy = scene.TryGetComponent<HierarchyComponent>(entity);
    if (!hierarchy || hierarchy->parent == entt::null || !scene.IsValid(hierarchy->parent))
        return worldPosition;
    if (const auto* parentWorld = scene.TryGetComponent<WorldTransformComponent>(hierarchy->parent))
        return glm::vec3(glm::inverse(parentWorld->worldMatrix) * glm::vec4(worldPosition, 1.0f));
    return worldPosition;
}
}  // namespace

void TransformGizmo::DrawAndUpdate(Scene& scene, entt::entity cameraEntity, float viewportX, float viewportY,
                                   float viewportWidth, float viewportHeight)
{
    if (const auto* config = ServiceLocator::Instance().Resolve<ConfigManager>();
        config && !config->GetConfigSnapshot()->debug.gizmos)
    {
        CancelDrag();
        return;
    }
    DrawToolbar(viewportX, viewportY, viewportWidth);

    auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
    if (!selection || selection->Empty() || viewportWidth <= 0.0f || viewportHeight <= 0.0f)
        return;

    const entt::entity primary = selection->GetPrimary();
    if (!scene.IsValid(primary))
        return;
    const auto* primaryPosition = scene.TryGetComponent<PositionComponent>(primary);
    if (!primaryPosition)
        return;

    glm::vec3 pivot = primaryPosition->value;
    if (const auto* world = scene.TryGetComponent<WorldTransformComponent>(primary))
        pivot = glm::vec3(world->worldMatrix[3]);
    if (m_PivotMode == TransformGizmoPivot::SelectionCenter)
    {
        pivot = glm::vec3(0.0f);
        size_t count = 0;
        for (const entt::entity entity : selection->GetAll())
        {
            if (const auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
            {
                pivot += glm::vec3(world->worldMatrix[3]);
                ++count;
            }
            else if (const auto* position = scene.TryGetComponent<PositionComponent>(entity))
            {
                pivot += position->value;
                ++count;
            }
        }
        if (count > 0)
            pivot /= static_cast<float>(count);
    }

    if (cameraEntity == entt::null)
        return;
    const auto* camera = scene.TryGetComponent<CameraComponent>(cameraEntity);
    const auto* cameraPosition = scene.TryGetComponent<PositionComponent>(cameraEntity);
    const auto* cameraRotation = scene.TryGetComponent<RotationComponent>(cameraEntity);
    if (!camera || !cameraPosition || !cameraRotation)
        return;

    const glm::vec3 front = cameraRotation->value * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 up = cameraRotation->value * glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(cameraPosition->value, cameraPosition->value + front, up);
    const float aspect = viewportWidth / viewportHeight;
    const glm::mat4 projection =
        camera->isOrthographic
            ? glm::ortho(-camera->orthoSize * aspect, camera->orthoSize * aspect, -camera->orthoSize,
                         camera->orthoSize, camera->nearPlane, camera->farPlane)
            : glm::perspective(glm::radians(camera->fov), aspect, camera->nearPlane, camera->farPlane);
    const glm::mat4 viewProjection = projection * view;

    glm::vec3 axes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)};
    if (m_Space == TransformGizmoSpace::Local)
    {
        const glm::quat rotation =
            scene.TryGetComponent<RotationComponent>(primary)
                ? scene.GetComponent<RotationComponent>(primary).value
                : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        for (glm::vec3& axis : axes)
            axis = glm::normalize(rotation * axis);
    }

    const float distance = glm::length(cameraPosition->value - pivot);
    const float worldLength = camera->isOrthographic ? camera->orthoSize * 0.22f : std::max(0.25f, distance * 0.12f);
    glm::vec2 originScreen;
    if (!ProjectPoint(viewProjection, pivot, viewportWidth, viewportHeight, originScreen))
        return;
    originScreen += glm::vec2(viewportX, viewportY);

    glm::vec2 endScreen[3];
    bool visible[3]{};
    for (int axis = 0; axis < 3; ++axis)
        visible[axis] = ProjectPoint(viewProjection, pivot + axes[axis] * worldLength,
                                     viewportWidth, viewportHeight, endScreen[axis]);
    for (int axis = 0; axis < 3; ++axis)
        if (visible[axis])
            endScreen[axis] += glm::vec2(viewportX, viewportY);

    static constexpr ImU32 colors[3] = {
        IM_COL32(235, 65, 65, 255), IM_COL32(70, 220, 90, 255), IM_COL32(70, 130, 245, 255)};
    // Draw over the game framebuffer but below every ImGui window. Foreground
    // draw lists make handles bleed through opaque editor panels.
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    for (int axis = 0; axis < 3; ++axis)
    {
        if (!visible[axis])
            continue;
        const ImU32 color = m_ActiveAxis == axis ? IM_COL32(255, 220, 70, 255) : colors[axis];
        drawList->AddLine(ImVec2(originScreen.x, originScreen.y), ImVec2(endScreen[axis].x, endScreen[axis].y),
                          color, m_ActiveAxis == axis ? 4.0f : 3.0f);
        if (m_Operation == TransformGizmoOperation::Scale)
            drawList->AddRectFilled(ImVec2(endScreen[axis].x - 5.0f, endScreen[axis].y - 5.0f),
                                    ImVec2(endScreen[axis].x + 5.0f, endScreen[axis].y + 5.0f), color);
        else
            drawList->AddCircleFilled(ImVec2(endScreen[axis].x, endScreen[axis].y), 5.0f, color);
    }

    const ImGuiIO& io = ImGui::GetIO();
    const glm::vec2 mouse(io.MousePos.x, io.MousePos.y);
    const auto* uiCapture = ServiceLocator::Instance().Resolve<IUIInputCapture>();
    const bool pointerCapturedByPanel = uiCapture && uiCapture->WantsPointerInput();
    const bool mouseInViewport = mouse.x >= viewportX && mouse.y >= viewportY &&
                                 mouse.x < viewportX + viewportWidth && mouse.y < viewportY + viewportHeight;
    if (m_ActiveAxis < 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        mouseInViewport && !pointerCapturedByPanel)
    {
        float bestDistance = 10.0f;
        int bestAxis = -1;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (!visible[axis])
                continue;
            const float hitDistance = DistanceToSegment(mouse, originScreen, endScreen[axis]);
            if (hitDistance < bestDistance)
            {
                bestDistance = hitDistance;
                bestAxis = axis;
            }
        }
        if (bestAxis >= 0)
            BeginDrag(scene, bestAxis, mouse, pivot, axes);
    }

    if (m_ActiveAxis >= 0)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const glm::vec2 screenAxis = endScreen[m_ActiveAxis] - originScreen;
            ApplyDrag(scene, mouse, screenAxis, worldLength);
        }
        else
        {
            EndDrag(scene);
        }
    }
}

void TransformGizmo::DrawToolbar(float viewportX, float viewportY, float viewportWidth)
{
    ImGui::SetNextWindowPos(ImVec2(viewportX + viewportWidth * 0.5f, viewportY + 8.0f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("Transform Gizmo", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
    if (ImGui::RadioButton("Move", m_Operation == TransformGizmoOperation::Translate))
        m_Operation = TransformGizmoOperation::Translate;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", m_Operation == TransformGizmoOperation::Rotate))
        m_Operation = TransformGizmoOperation::Rotate;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", m_Operation == TransformGizmoOperation::Scale))
        m_Operation = TransformGizmoOperation::Scale;
    ImGui::SameLine();
    if (ImGui::Button(m_Space == TransformGizmoSpace::World ? "World" : "Local"))
        m_Space = m_Space == TransformGizmoSpace::World ? TransformGizmoSpace::Local : TransformGizmoSpace::World;
    ImGui::SameLine();
    if (ImGui::Button(m_PivotMode == TransformGizmoPivot::Primary ? "Pivot" : "Center"))
        m_PivotMode =
            m_PivotMode == TransformGizmoPivot::Primary ? TransformGizmoPivot::SelectionCenter
                                                       : TransformGizmoPivot::Primary;
    ImGui::End();
}

void TransformGizmo::BeginDrag(Scene& scene, int axis, const glm::vec2& mouse, const glm::vec3& pivot,
                               const glm::vec3 (&axes)[3])
{
    auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
    if (!selection)
        return;

    m_ActiveAxis = axis;
    m_DragStartMouse = mouse;
    m_DragPivot = pivot;
    std::copy(std::begin(axes), std::end(axes), std::begin(m_DragAxes));
    m_InitialTransforms.clear();
    for (const entt::entity entity : selection->GetAll())
    {
        const auto* position = scene.TryGetComponent<PositionComponent>(entity);
        if (!position)
            continue;
        InitialTransform initial;
        initial.entity = entity;
        initial.position = position->value;
        initial.worldPosition = position->value;
        if (const auto* world = scene.TryGetComponent<WorldTransformComponent>(entity))
            initial.worldPosition = glm::vec3(world->worldMatrix[3]);
        if (const auto* rotation = scene.TryGetComponent<RotationComponent>(entity))
            initial.rotation = rotation->value;
        if (const auto* scale = scene.TryGetComponent<ScaleComponent>(entity))
            initial.scale = scale->value;
        m_InitialTransforms.push_back(initial);
    }
}

void TransformGizmo::ApplyDrag(Scene& scene, const glm::vec2& mouse, const glm::vec2& screenAxis,
                               float worldLength)
{
    const float screenLength = glm::length(screenAxis);
    if (screenLength < 1.0f || m_ActiveAxis < 0)
        return;
    const glm::vec2 screenDirection = screenAxis / screenLength;
    const float pixels = glm::dot(mouse - m_DragStartMouse, screenDirection);
    float amount = pixels / screenLength * worldLength;

    auto* config = ServiceLocator::Instance().Resolve<ConfigManager>();
    const auto snapshot = config ? config->GetConfigSnapshot() : nullptr;
    const bool snap = snapshot && snapshot->debug.gridSnapEnabled;

    if (m_Operation == TransformGizmoOperation::Rotate)
    {
        float radians = pixels * 0.01f;
        if (snap)
        {
            const float step = glm::radians(std::max(0.1f, snapshot->debug.gridSnapRotation));
            radians = std::round(radians / step) * step;
        }
        glm::vec3 localAxis(0.0f);
        localAxis[m_ActiveAxis] = 1.0f;
        const glm::quat delta =
            glm::angleAxis(radians, m_Space == TransformGizmoSpace::Local
                                         ? localAxis
                                         : glm::normalize(m_DragAxes[m_ActiveAxis]));
        const glm::quat pivotDelta =
            glm::angleAxis(radians, glm::normalize(m_DragAxes[m_ActiveAxis]));
        for (const InitialTransform& initial : m_InitialTransforms)
        {
            if (auto* rotation = scene.TryGetComponent<RotationComponent>(initial.entity))
            {
                rotation->value = glm::normalize(
                    m_Space == TransformGizmoSpace::World ? delta * initial.rotation : initial.rotation * delta);
                rotation->prev = rotation->value;
            }
            if (m_PivotMode == TransformGizmoPivot::SelectionCenter)
            {
                const glm::vec3 worldPosition =
                    m_DragPivot + pivotDelta * (initial.worldPosition - m_DragPivot);
                if (auto* position = scene.TryGetComponent<PositionComponent>(initial.entity))
                    position->prev = position->value = ToLocalPosition(scene, initial.entity, worldPosition);
            }
            scene.MarkTransformDirty(initial.entity);
        }
        return;
    }

    if (m_Operation == TransformGizmoOperation::Scale)
    {
        float factor = std::max(0.01f, 1.0f + amount / std::max(worldLength, 0.001f));
        if (snap)
        {
            const float step = std::max(0.01f, snapshot->debug.gridSnapScale);
            factor = std::max(0.01f, std::round(factor / step) * step);
        }
        for (const InitialTransform& initial : m_InitialTransforms)
        {
            if (auto* scale = scene.TryGetComponent<ScaleComponent>(initial.entity))
            {
                scale->value = initial.scale;
                scale->value[m_ActiveAxis] = std::max(0.001f, initial.scale[m_ActiveAxis] * factor);
                scale->prev = scale->value;
            }
            if (m_PivotMode == TransformGizmoPivot::SelectionCenter)
            {
                const glm::vec3 axis = glm::normalize(m_DragAxes[m_ActiveAxis]);
                const glm::vec3 offset = initial.worldPosition - m_DragPivot;
                const glm::vec3 worldPosition =
                    initial.worldPosition + axis * glm::dot(offset, axis) * (factor - 1.0f);
                if (auto* position = scene.TryGetComponent<PositionComponent>(initial.entity))
                    position->prev = position->value = ToLocalPosition(scene, initial.entity, worldPosition);
            }
            scene.MarkTransformDirty(initial.entity);
        }
        return;
    }

    if (snap)
    {
        const float step = std::max(0.001f, snapshot->debug.gridSnapTranslation);
        amount = std::round(amount / step) * step;
    }
    const glm::vec3 worldDelta = m_DragAxes[m_ActiveAxis] * amount;
    for (const InitialTransform& initial : m_InitialTransforms)
    {
        if (auto* position = scene.TryGetComponent<PositionComponent>(initial.entity))
            position->prev = position->value =
                ToLocalPosition(scene, initial.entity, initial.worldPosition + worldDelta);
        scene.MarkTransformDirty(initial.entity);
    }
}

void TransformGizmo::CancelDrag()
{
    m_ActiveAxis = -1;
    m_InitialTransforms.clear();
}

void TransformGizmo::CommitDrag(Scene& scene)
{
    EndDrag(scene);
}

void TransformGizmo::EndDrag(Scene& scene)
{
    if (m_ActiveAxis < 0)
        return;

    std::vector<TransformCommandValue> before;
    std::vector<TransformCommandValue> after;
    before.reserve(m_InitialTransforms.size());
    after.reserve(m_InitialTransforms.size());
    bool changed = false;
    for (const InitialTransform& initial : m_InitialTransforms)
    {
        if (!scene.IsValid(initial.entity))
            continue;
        TransformCommandValue previous{initial.entity, initial.position, initial.rotation, initial.scale};
        TransformCommandValue current = previous;
        if (const auto* position = scene.TryGetComponent<PositionComponent>(initial.entity))
            current.position = position->value;
        if (const auto* rotation = scene.TryGetComponent<RotationComponent>(initial.entity))
            current.rotation = rotation->value;
        if (const auto* scale = scene.TryGetComponent<ScaleComponent>(initial.entity))
            current.scale = scale->value;
        changed = changed || glm::length(previous.position - current.position) > 0.000001f ||
                  std::abs(glm::dot(previous.rotation, current.rotation)) < 0.999999f ||
                  glm::length(previous.scale - current.scale) > 0.000001f;
        before.push_back(previous);
        after.push_back(current);
    }
    if (changed)
        EditorSystem::CommitExecutedCommand(
            scene, std::make_unique<TransformEntitiesCommand>(std::move(before), std::move(after)));
    CancelDrag();
}

#endif
