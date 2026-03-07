#include <ecs/component.h>
#include <iostream>
#include <systems/physics/physics_loader.h>
#include <sstream>
#include <string>
#include <core/utils/bullet_glm_helpers.h>
#include <core/utils/logger.h>

void PhysicsLoader::LoadRigidBody(Scene &scene, entt::entity entity, const YAMLNode &node, IPhysicsWorld &physics)
{
    std::vector<std::string> allowed = {"Type", "Mass", "Size", "Radius", "Height", "Offset", "Restitution", "AngularFactor", "LinearFactor", "BodyType", "Shapes"};
    for (const auto& child : node.children)
    {
        if (std::find(allowed.begin(), allowed.end(), child.key) == allowed.end())
        {
            LOGGER_WARN("PhysicsLoader") << "Unknown key '" << child.key << "' in component 'RigidBody'";
        }
    }

    std::string type = node.GetChildValue("Type", "BOX");
    float mass = std::stof(node.GetChildValue("Mass", "1.0"));
    if (mass < 0.0f) LOGGER_WARN("PhysicsLoader") << "RigidBody Mass should not be negative: " << mass;

    auto &pos = scene.registry.get<PositionComponent>(entity);
    auto &rot = scene.registry.get<RotationComponent>(entity);
    auto &rb = scene.registry.emplace<RigidBodyComponent>(entity);

    std::shared_ptr<ICollisionShape> finalShape = nullptr;

    if (type == "COMPOUND")
    {
        auto compound = physics.CreateCompoundShape();
        if (auto* shapesNode = const_cast<YAMLNode*>(&node)->GetChild("Shapes"))
        {
            for (auto& shapeNode : shapesNode->children)
            {
                if (shapeNode.key != "Shape") continue;
                
                std::string shapeType = shapeNode.GetChildValue("Type", "BOX");
                
                std::stringstream posSS(shapeNode.GetChildValue("Position", "0 0 0"));
                float lx, ly, lz; posSS >> lx >> ly >> lz;
                glm::vec3 localPos(lx, ly, lz);
                
                std::stringstream rotSS(shapeNode.GetChildValue("Rotation", "0 0 0"));
                float lrx, lry, lrz; rotSS >> lrx >> lry >> lrz;
                glm::quat localRot = glm::quat(glm::vec3(glm::radians(lrx), glm::radians(lry), glm::radians(lrz)));

                std::shared_ptr<ICollisionShape> childShape = nullptr;

                if (shapeType == "BOX")
                {
                    std::stringstream szSS(shapeNode.GetChildValue("Size", "1 1 1"));
                    float x, y, z; szSS >> x >> y >> z;
                    childShape = physics.CreateBoxShape(glm::vec3(x, y, z));
                }
                else if (shapeType == "SPHERE")
                {
                    float r = std::stof(shapeNode.GetChildValue("Radius", "1.0"));
                    childShape = physics.CreateSphereShape(r);
                }
                else if (shapeType == "CAPSULE")
                {
                    float r = std::stof(shapeNode.GetChildValue("Radius", "0.5"));
                    float h = std::stof(shapeNode.GetChildValue("Height", "1.0"));
                    childShape = physics.CreateCapsuleShape(r, h);
                }

                if (childShape)
                {
                    physics.AddChildShape(compound, childShape, localPos, localRot);
                }
            }
        }
        finalShape = compound;
    }
    else if (type == "CAPSULE")
    {
        float r = std::stof(node.GetChildValue("Radius", "0.5"));
        float h = std::stof(node.GetChildValue("Height", "1.0"));
        finalShape = physics.CreateCapsuleShape(r, h);
    }
    else if (type == "BOX")
    {
        std::stringstream szSS(node.GetChildValue("Size", "1 1 1"));
        float x, y, z; szSS >> x >> y >> z;
        finalShape = physics.CreateBoxShape(glm::vec3(x, y, z));
    }

    glm::vec3 centerOffset(0.0f);
    if (!node.GetChildValue("Offset").empty()) {
        std::stringstream offSS(node.GetChildValue("Offset"));
        float ox, oy, oz; offSS >> ox >> oy >> oz;
        centerOffset = glm::vec3(ox, oy, oz);
    }

    float restitution = std::stof(node.GetChildValue("Restitution", "0.0"));
    
    bool hasRotFactor = false;
    if (!node.GetChildValue("AngularFactor").empty()) {
        std::stringstream angSS(node.GetChildValue("AngularFactor"));
        float x, y, z; angSS >> x >> y >> z;
        rb.angularFactor = glm::vec3(x, y, z);
        hasRotFactor = true;
    }
    
    bool hasPosFactor = false;
    if (!node.GetChildValue("LinearFactor").empty()) {
        std::stringstream linSS(node.GetChildValue("LinearFactor"));
        float x, y, z; linSS >> x >> y >> z;
        rb.linearFactor = glm::vec3(x, y, z);
        hasPosFactor = true;
    }

    rb.isParentMatter = node.GetChildValue("ParentMatter", "false") == "true" || node.GetChildValue("ParentMatter", "0") == "1";
    rb.isChildrenMatter = node.GetChildValue("ChildrenMatter", "false") == "true" || node.GetChildValue("ChildrenMatter", "0") == "1";
    rb.isAttachedToParent = node.GetChildValue("AttachToParent", "false") == "true" || node.GetChildValue("AttachToParent", "0") == "1";

    std::string bodyType = node.GetChildValue("BodyType", "UNKNOWN");

    if (finalShape && glm::length(centerOffset) > 0.001f)
    {
        auto compound = physics.CreateCompoundShape();
        physics.AddChildShape(compound, finalShape, centerOffset, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        finalShape = compound;
    }

    if (finalShape)
    {
        if (bodyType == "UNKNOWN")
        {
            if (mass > 0.0f) bodyType = "DYNAMIC";
            else bodyType = "STATIC";
        }

        if (bodyType == "STATIC" || bodyType == "KINEMATIC") mass = 0.0f;

        rb.body = physics.CreateRigidBody(mass, pos.value, rot.value, finalShape);

        if (rb.body)
        {
            if (bodyType == "KINEMATIC") rb.body->SetKinematic(true);
            rb.body->SetUserPointer((void *)(uintptr_t)entity);

            if (type == "CAPSULE" || type == "PLAYER")
            {
                 if (!hasRotFactor) rb.angularFactor = glm::vec3(0, 1, 0);
            }

            rb.body->SetAngularFactor(rb.angularFactor);
            rb.body->SetLinearFactor(rb.linearFactor);

            if (restitution > 0.0f) rb.body->SetRestitution(restitution);

            physics.AddRigidBody(rb.body.get());
        }
    }
}
