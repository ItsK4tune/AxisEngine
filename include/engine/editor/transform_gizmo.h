#pragma once

#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

struct Scene;

enum class TransformGizmoOperation
{
    Translate,
    Rotate,
    Scale
};

enum class TransformGizmoSpace
{
    Local,
    World
};

enum class TransformGizmoPivot
{
    Primary,
    SelectionCenter
};

class TransformGizmo
{
public:
    void DrawAndUpdate(Scene& scene, entt::entity cameraEntity, float viewportX, float viewportY,
                       float viewportWidth, float viewportHeight);
    void CancelDrag();
    void CommitDrag(Scene& scene);

    TransformGizmoOperation GetOperation() const { return m_Operation; }
    TransformGizmoSpace GetSpace() const { return m_Space; }
    TransformGizmoPivot GetPivotMode() const { return m_PivotMode; }
    bool IsDragging() const { return m_ActiveAxis >= 0; }

private:
    struct InitialTransform
    {
        entt::entity entity = entt::null;
        glm::vec3 position{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        glm::vec3 worldPosition{0.0f};
    };

    void DrawToolbar(float viewportX, float viewportY, float viewportWidth);
    void BeginDrag(Scene& scene, int axis, const glm::vec2& mouse, const glm::vec3& pivot,
                   const glm::vec3 (&axes)[3]);
    void ApplyDrag(Scene& scene, const glm::vec2& mouse, const glm::vec2& screenAxis, float worldLength);
    void EndDrag(Scene& scene);

    TransformGizmoOperation m_Operation = TransformGizmoOperation::Translate;
    TransformGizmoSpace m_Space = TransformGizmoSpace::World;
    TransformGizmoPivot m_PivotMode = TransformGizmoPivot::Primary;
    int m_ActiveAxis = -1;
    glm::vec2 m_DragStartMouse{0.0f};
    glm::vec3 m_DragPivot{0.0f};
    glm::vec3 m_DragAxes[3] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)};
    std::vector<InitialTransform> m_InitialTransforms;
};
