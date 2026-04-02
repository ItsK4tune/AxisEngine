#include <ecs/unit/core_components.h>
#include <iostream>
#include <ecs/unit/physics_components.h>
#include <physics/logic/physics_loader.h>
#include <sstream>
#include <string>
#include <physics/strategy/bullet/bullet_glm_helpers.h>
#include <core/logic/logger.h>
#include <core/logic/loader_utils.h>

void PhysicsLoader::LoadRigidShape(Scene &scene, entt::entity entity, const YAMLNode &node, IPhysicsWorld &physics)
{
    LoaderUtils::ValidateKeys(node, {"Type", "Size", "Radius", "Height", "Offset", "Rotation", "Friction", "Restitution", "Shapes"}, "RigidShape");

    auto &rs = scene.registry.get_or_emplace<RigidShapeComponent>(entity);
    rs.type = node.GetChildValue("Type", "BOX");
    
    if (!node.GetChildValue("Size").empty()) {
        std::stringstream ss(node.GetChildValue("Size"));
        ss >> rs.size.x >> rs.size.y >> rs.size.z;
    }
    rs.radius = std::stof(node.GetChildValue("Radius", "0.5"));
    rs.height = std::stof(node.GetChildValue("Height", "1.0"));
    
    if (!node.GetChildValue("Offset").empty()) {
        std::stringstream ss(node.GetChildValue("Offset"));
        ss >> rs.offset.x >> rs.offset.y >> rs.offset.z;
    }
    if (!node.GetChildValue("Rotation").empty()) {
        std::stringstream ss(node.GetChildValue("Rotation"));
        float rx, ry, rz; ss >> rx >> ry >> rz;
        rs.rotation = glm::quat(glm::radians(glm::vec3(rx, ry, rz)));
    }

    rs.friction = std::stof(node.GetChildValue("Friction", "0.5"));
    rs.restitution = std::stof(node.GetChildValue("Restitution", "0.0"));

    if (auto* shapesNode = const_cast<YAMLNode*>(&node)->GetChild("Shapes")) {
        for (auto& shapeNode : shapesNode->children) {
            if (shapeNode.key != "Shape") continue;
            RigidShapeComponent::ChildShape child;
            child.type = shapeNode.GetChildValue("Type", "BOX");
            
            std::stringstream posSS(shapeNode.GetChildValue("Position", "0 0 0"));
            posSS >> child.position.x >> child.position.y >> child.position.z;
            
            std::stringstream rotSS(shapeNode.GetChildValue("Rotation", "0 0 0"));
            float rrx, rry, rrz; rotSS >> rrx >> rry >> rrz;
            child.rotation = glm::quat(glm::radians(glm::vec3(rrx, rry, rrz)));

            std::stringstream szSS(shapeNode.GetChildValue("Size", "1 1 1"));
            szSS >> child.size.x >> child.size.y >> child.size.z;
            
            child.radius = std::stof(shapeNode.GetChildValue("Radius", "0.5"));
            child.height = std::stof(shapeNode.GetChildValue("Height", "1.0"));
            
            rs.children.push_back(child);
        }
    }
}

void PhysicsLoader::LoadRigidBody(Scene &scene, entt::entity entity, const YAMLNode &node, IPhysicsWorld &physics)
{
    LoaderUtils::ValidateKeys(node, {"Mass", "BodyType", "LinearFactor", "AngularFactor", "LinearDamping", "AngularDamping", "LinearVelocity", "AngularVelocity", "AttachToParent", "ParentMatter", "ChildrenMatter", "CollisionEnabled", "Type", "Size", "Radius", "Height", "Offset", "Rotation", "Restitution", "Friction", "Shapes"}, "RigidBody");

    // Check if we are loading the "unified" legacy format or just the dynamic part
    bool isLegacy = !node.GetChildValue("Type").empty();
    if (isLegacy) {
        LoadRigidShape(scene, entity, node, physics);
    }

    auto &rb = scene.registry.get_or_emplace<RigidBodyComponent>(entity);
    rb.mass = std::stof(node.GetChildValue("Mass", "1.0"));
    
    std::string bodyType = node.GetChildValue("BodyType", "UNKNOWN");
    if (bodyType == "STATIC") { rb.isStatic = true; rb.mass = 0.0f; }
    else if (bodyType == "KINEMATIC") { rb.isKinematic = true; rb.mass = 0.0f; }
    else if (bodyType == "DYNAMIC" || (bodyType == "UNKNOWN" && rb.mass > 0.0f)) { rb.isStatic = false; }
    else if (bodyType == "UNKNOWN" && rb.mass <= 0.0f) { rb.isStatic = true; }

    if (!node.GetChildValue("LinearFactor").empty()) {
        std::stringstream ss(node.GetChildValue("LinearFactor"));
        ss >> rb.linearFactor.x >> rb.linearFactor.y >> rb.linearFactor.z;
    }
    if (!node.GetChildValue("AngularFactor").empty()) {
        std::stringstream ss(node.GetChildValue("AngularFactor"));
        ss >> rb.angularFactor.x >> rb.angularFactor.y >> rb.angularFactor.z;
    }
    
    rb.linearDamping = std::stof(node.GetChildValue("LinearDamping", "0.0"));
    rb.angularDamping = std::stof(node.GetChildValue("AngularDamping", "0.0"));

    rb.isAttachedToParent = node.GetChildValue("AttachToParent", "false") == "true";
    rb.isParentMatter = node.GetChildValue("ParentMatter", "false") == "true";
    rb.isChildrenMatter = node.GetChildValue("ChildrenMatter", "false") == "true";
    rb.isCollisionEnabled = node.GetChildValue("CollisionEnabled", "true") == "true";

    // Initial velocities if any
    glm::vec3 linVel(0.0f), angVel(0.0f);
    if (!node.GetChildValue("LinearVelocity").empty()) {
        std::stringstream ss(node.GetChildValue("LinearVelocity"));
        ss >> linVel.x >> linVel.y >> linVel.z;
    }
    if (!node.GetChildValue("AngularVelocity").empty()) {
        std::stringstream ss(node.GetChildValue("AngularVelocity"));
        ss >> angVel.x >> angVel.y >> angVel.z;
    }

    // We don't create the IRigidBody here anymore because it requires a Shape.
    // The PhysicsSystem will handle IRigidBody creation when RigidShape and RigidBody are both present.
    // Or if it's legacy, we could do it, but let's centralize it in the System for consistency.
}

void PhysicsLoader::LoadCharacterController(Scene &scene, entt::entity entity, const YAMLNode &node, IPhysicsWorld &physics)
{
    LoaderUtils::ValidateKeys(node, {"Radius", "Height", "StepHeight", "MaxSlope"}, "CharacterController");

    float radius = std::stof(node.GetChildValue("Radius", "0.5"));
    float height = std::stof(node.GetChildValue("Height", "1.0"));
    float stepHeight = std::stof(node.GetChildValue("StepHeight", "0.35"));
    float maxSlope = std::stof(node.GetChildValue("MaxSlope", "45.0"));

    auto &pos = scene.registry.get<PositionComponent>(entity);
    auto &rot = scene.registry.get<RotationComponent>(entity);
    
    auto& cc = scene.registry.emplace<CharacterControllerComponent>(entity);
    cc.stepHeight = stepHeight;
    cc.maxSlope = maxSlope;

    auto shape = physics.CreateCapsuleShape(radius, height);
    cc.controller = physics.CreateCharacterController(shape, stepHeight);

    if (cc.controller)
    {
        cc.controller->SetMaxSlope(glm::radians(maxSlope));
        cc.controller->SetWorldTransform(pos.value, rot.value);
        physics.AddCharacterController(cc.controller.get());
    }
}

