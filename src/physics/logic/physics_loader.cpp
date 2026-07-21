#include <physics/logic/physics_loader.h>
#include <core/logic/loader_utils.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/physics_components.h>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <string>

void PhysicsLoader::LoadRigidShape(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld* physics)
{
    LoaderUtils::ValidateKeys(
        node, {"Type", "Size", "Radius", "Height", "Offset", "Rotation", "Friction", "Restitution", "Shapes"},
        "RigidShape");

    auto& rs = scene.GetOrAddComponent<RigidShapeComponent>(entity);
    rs.type = ShapeTypeFromString(node.GetChildValue("Type", "BOX"));

    if (!node.GetChildValue("Size").empty())
    {
        std::stringstream ss(node.GetChildValue("Size"));
        ss >> rs.size.x >> rs.size.y >> rs.size.z;
    }
    rs.radius = LoaderUtils::SafeStof(node.GetChildValue("Radius", "0.5"));
    rs.height = LoaderUtils::SafeStof(node.GetChildValue("Height", "1.0"));

    if (!node.GetChildValue("Offset").empty())
    {
        std::stringstream ss(node.GetChildValue("Offset"));
        ss >> rs.offset.x >> rs.offset.y >> rs.offset.z;
    }
    if (!node.GetChildValue("Rotation").empty())
    {
        std::stringstream ss(node.GetChildValue("Rotation"));
        float rx, ry, rz;
        ss >> rx >> ry >> rz;
        rs.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
    }

    rs.friction = LoaderUtils::SafeStof(node.GetChildValue("Friction", "0.5"));
    rs.restitution = LoaderUtils::SafeStof(node.GetChildValue("Restitution", "0.0"));

    if (auto* shapesNode = const_cast<YAMLNode*>(&node)->GetChild("Shapes"))
    {
        for (auto& shapeNode : shapesNode->children)
        {
            if (shapeNode.key != "Shape")
                continue;
            RigidShapeComponent::ChildShape child;
            child.type = ShapeTypeFromString(shapeNode.GetChildValue("Type", "BOX"));

            std::stringstream posSS(shapeNode.GetChildValue("Position", "0 0 0"));
            posSS >> child.position.x >> child.position.y >> child.position.z;

            std::stringstream rotSS(shapeNode.GetChildValue("Rotation", "0 0 0"));
            float rrx, rry, rrz;
            rotSS >> rrx >> rry >> rrz;
            child.rotation = glm::quat(glm::radians(glm::vec3(rrx, rry, rrz)));

            std::stringstream szSS(shapeNode.GetChildValue("Size", "1 1 1"));
            szSS >> child.size.x >> child.size.y >> child.size.z;

            child.radius = LoaderUtils::SafeStof(shapeNode.GetChildValue("Radius", "0.5"));
            child.height = LoaderUtils::SafeStof(shapeNode.GetChildValue("Height", "1.0"));

            rs.children.push_back(child);
        }
    }
}

void PhysicsLoader::LoadRigidBody(Scene& scene, entt::entity entity, const YAMLNode& node, IPhysicsWorld* physics)
{
    LoaderUtils::ValidateKeys(node, {"Mass",           "BodyType",       "LinearFactor",   "AngularFactor",
                                     "LinearDamping",  "AngularDamping", "LinearVelocity", "AngularVelocity",
                                     "AttachToParent", "ParentMatter",   "ChildrenMatter", "CollisionEnabled",
                                     "IsTrigger",      "Type",           "Size",           "Radius",
                                     "Height",         "Offset",         "Rotation",       "Restitution",
                                     "Friction",       "Shapes"},
                              "RigidBody");

    // Check if we are loading the "unified" legacy format or just the dynamic part
    bool isLegacy = !node.GetChildValue("Type").empty();
    if (isLegacy)
    {
        LoadRigidShape(scene, entity, node, physics);
    }

    auto& rb = scene.GetOrAddComponent<RigidBodyComponent>(entity);
    rb.mass = LoaderUtils::SafeStof(node.GetChildValue("Mass", "1.0"));

    std::string bodyType = node.GetChildValue("BodyType", "UNKNOWN");
    if (bodyType == "STATIC")
    {
        rb.isStatic = true;
        rb.mass = 0.0f;
    }
    else if (bodyType == "KINEMATIC")
    {
        rb.isKinematic = true;
        rb.mass = 0.0f;
    }
    else if (bodyType == "DYNAMIC" || (bodyType == "UNKNOWN" && rb.mass > 0.0f))
    {
        rb.isStatic = false;
    }
    else if (bodyType == "UNKNOWN" && rb.mass <= 0.0f)
    {
        rb.isStatic = true;
    }

    if (!node.GetChildValue("LinearFactor").empty())
    {
        std::stringstream ss(node.GetChildValue("LinearFactor"));
        ss >> rb.linearFactor.x >> rb.linearFactor.y >> rb.linearFactor.z;
    }
    if (!node.GetChildValue("AngularFactor").empty())
    {
        std::stringstream ss(node.GetChildValue("AngularFactor"));
        ss >> rb.angularFactor.x >> rb.angularFactor.y >> rb.angularFactor.z;
    }

    rb.linearDamping = LoaderUtils::SafeStof(node.GetChildValue("LinearDamping", "0.0"));
    rb.angularDamping = LoaderUtils::SafeStof(node.GetChildValue("AngularDamping", "0.0"));

    rb.isAttachedToParent = node.GetChildValue("AttachToParent", "false") == "true";
    rb.isParentMatter = node.GetChildValue("ParentMatter", "false") == "true";
    rb.isChildrenMatter = node.GetChildValue("ChildrenMatter", "false") == "true";
    rb.isCollisionEnabled = node.GetChildValue("CollisionEnabled", "true") == "true";
    rb.isTrigger = node.GetChildValue("IsTrigger", "false") == "true" || node.GetChildValue("IsTrigger", "0") == "1";

    // Initial velocities if any
    glm::vec3 linVel(0.0f), angVel(0.0f);
    if (!node.GetChildValue("LinearVelocity").empty())
    {
        std::stringstream ss(node.GetChildValue("LinearVelocity"));
        ss >> linVel.x >> linVel.y >> linVel.z;
    }
    if (!node.GetChildValue("AngularVelocity").empty())
    {
        std::stringstream ss(node.GetChildValue("AngularVelocity"));
        ss >> angVel.x >> angVel.y >> angVel.z;
    }
    rb.initialLinearVelocity = linVel;
    rb.initialAngularVelocity = angVel;
    if (rb.body)
    {
        rb.body->SetLinearVelocity(linVel);
        rb.body->SetAngularVelocity(angVel);
        rb.body->Activate(true);
    }

    // Body creation belongs to PhysicsSystem once shape and body components
    // are both present; loaders only construct deterministic authoring data.
}

void PhysicsLoader::LoadCharacterController(Scene& scene, entt::entity entity, const YAMLNode& node,
                                            IPhysicsWorld* physics)
{
    LoaderUtils::ValidateKeys(node, {"Radius", "Height", "StepHeight", "MaxSlope"}, "CharacterController");

    float radius = LoaderUtils::SafeStof(node.GetChildValue("Radius", "0.5"));
    float height = LoaderUtils::SafeStof(node.GetChildValue("Height", "1.0"));
    float stepHeight = LoaderUtils::SafeStof(node.GetChildValue("StepHeight", "0.35"));
    float maxSlope = LoaderUtils::SafeStof(node.GetChildValue("MaxSlope", "45.0"));

    auto& pos = scene.GetComponent<PositionComponent>(entity);
    auto& rot = scene.GetComponent<RotationComponent>(entity);

    auto& cc = scene.AddComponent<CharacterControllerComponent>(entity);
    cc.radius = radius;
    cc.height = height;
    cc.stepHeight = stepHeight;
    cc.maxSlope = maxSlope;

    if (!physics)
        return;

    auto shape = physics->CreateCapsuleShape(radius, height);
    cc.controller = physics->CreateCharacterController(shape, stepHeight);

    if (cc.controller)
    {
        cc.controller->SetMaxSlope(glm::radians(maxSlope));
        cc.controller->SetWorldTransform(pos.value, rot.value);
        cc.controller->SetUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(entity) + 1));
        physics->AddCharacterController(cc.controller.get());
    }
}
