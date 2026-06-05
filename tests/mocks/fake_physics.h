#pragma once

#include <physics/interface/i_character_controller.h>
#include <physics/interface/i_collision_shape.h>
#include <physics/interface/i_constraint.h>
#include <physics/interface/i_physics_world.h>
#include <physics/interface/i_rigid_body.h>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <vector>

namespace axis_test_mocks
{
class FakeCollisionShape : public ICollisionShape
{
public:
    explicit FakeCollisionShape(CollisionShapeType type) : m_Type(type)
    {
    }

    CollisionShapeType GetType() const override
    {
        return m_Type;
    }

    void SetLocalScaling(const glm::vec3& scaling) override
    {
        m_Scaling = scaling;
    }

    glm::vec3 GetLocalScaling() const override
    {
        return m_Scaling;
    }

private:
    CollisionShapeType m_Type;
    glm::vec3 m_Scaling = glm::vec3(1.0f);
};

class FakeRigidBody : public IRigidBody
{
public:
    void SetLinearVelocity(const glm::vec3& vel) override
    {
        linearVelocity = vel;
    }

    void SetAngularVelocity(const glm::vec3& vel) override
    {
        angularVelocity = vel;
    }

    glm::vec3 GetLinearVelocity() const override
    {
        return linearVelocity;
    }

    glm::vec3 GetAngularVelocity() const override
    {
        return angularVelocity;
    }

    void ApplyCentralForce(const glm::vec3& force) override
    {
        appliedForce = force;
    }

    void ApplyCentralImpulse(const glm::vec3& impulse) override
    {
        appliedImpulse = impulse;
    }

    void ApplyTorque(const glm::vec3& torque) override
    {
        appliedTorque = torque;
    }

    void SetFriction(float value) override
    {
        friction = value;
    }

    void SetRestitution(float value) override
    {
        restitution = value;
    }

    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override
    {
        position = pos;
        rotation = rot;
    }

    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override
    {
        pos = position;
        rot = rotation;
    }

    void Activate(bool forceActivation = false) override
    {
        active = forceActivation || active;
    }

    bool IsActive() const override
    {
        return active;
    }

    bool IsStatic() const override
    {
        return isStatic;
    }

    bool IsKinematic() const override
    {
        return isKinematic;
    }

    bool IsTrigger() const override
    {
        return isTrigger;
    }

    void SetKinematic(bool value) override
    {
        isKinematic = value;
    }

    void SetStatic(bool value) override
    {
        isStatic = value;
    }

    void SetTrigger(bool value) override
    {
        isTrigger = value;
    }

    void SetAlwaysActive(bool value) override
    {
        alwaysActive = value;
    }

    void SetUserPointer(void* ptr) override
    {
        userPointer = ptr;
    }

    void* GetUserPointer() const override
    {
        return userPointer;
    }

    void SetLinearFactor(const glm::vec3& factor) override
    {
        linearFactor = factor;
    }

    void SetAngularFactor(const glm::vec3& factor) override
    {
        angularFactor = factor;
    }

    void SetDamping(float linear, float angular) override
    {
        linearDamping = linear;
        angularDamping = angular;
    }

    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 linearVelocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);
    glm::vec3 appliedForce = glm::vec3(0.0f);
    glm::vec3 appliedImpulse = glm::vec3(0.0f);
    glm::vec3 appliedTorque = glm::vec3(0.0f);
    glm::vec3 linearFactor = glm::vec3(1.0f);
    glm::vec3 angularFactor = glm::vec3(1.0f);
    float friction = 0.0f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool active = false;
    bool isStatic = false;
    bool isKinematic = false;
    bool isTrigger = false;
    bool alwaysActive = false;
    void* userPointer = nullptr;
};

class FakeCharacterController : public ICharacterController
{
public:
    void SetWalkDirection(const glm::vec3& dir) override
    {
        walkDirection = dir;
    }

    void SetVelocity(const glm::vec3& vel, float timeInterval) override
    {
        velocity = vel;
        velocityInterval = timeInterval;
    }

    void SetFallSpeed(float speed) override
    {
        fallSpeed = speed;
    }

    void SetGravity(const glm::vec3& value) override
    {
        gravity = value;
    }

    void SetJumpSpeed(float speed) override
    {
        jumpSpeed = speed;
    }

    void Jump() override
    {
        jumped = true;
    }

    bool OnGround() const override
    {
        return onGround;
    }

    void SetStepHeight(float height) override
    {
        stepHeight = height;
    }

    void SetMaxSlope(float slopeRadians) override
    {
        maxSlope = slopeRadians;
    }

    void GetWorldTransform(glm::vec3& pos, glm::quat& rot) const override
    {
        pos = position;
        rot = rotation;
    }

    void SetWorldTransform(const glm::vec3& pos, const glm::quat& rot) override
    {
        position = pos;
        rotation = rot;
    }

    void Activate(bool forceActivation = false) override
    {
        active = forceActivation || active;
    }

    void SetUserPointer(void* ptr) override
    {
        userPointer = ptr;
    }

    void* GetUserPointer() const override
    {
        return userPointer;
    }

    glm::vec3 walkDirection = glm::vec3(0.0f);
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float velocityInterval = 0.0f;
    float fallSpeed = 0.0f;
    float jumpSpeed = 0.0f;
    float stepHeight = 0.0f;
    float maxSlope = 0.0f;
    bool jumped = false;
    bool onGround = true;
    bool active = false;
    void* userPointer = nullptr;
};

class FakeConstraint : public IConstraint
{
public:
    void SetBreakingImpulseThreshold(float threshold) override
    {
        breakingImpulseThreshold = threshold;
    }

    float GetBreakingImpulseThreshold() const override
    {
        return breakingImpulseThreshold;
    }

    float GetAppliedImpulse() const override
    {
        return appliedImpulse;
    }

    float breakingImpulseThreshold = 0.0f;
    float appliedImpulse = 0.0f;
};

class FakePhysicsWorld : public IPhysicsWorld
{
public:
    struct ShapeCall
    {
        std::string type;
        glm::vec3 vectorValue = glm::vec3(0.0f);
        float radius = 0.0f;
        float height = 0.0f;
    };

    struct ChildShapeCall
    {
        std::shared_ptr<ICollisionShape> parent;
        std::shared_ptr<ICollisionShape> child;
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    };

    void Initialize() override
    {
        initialized = true;
    }

    void Update(float dt) override
    {
        lastUpdateDt = dt;
        ++updateCount;
    }

    void Clear() override
    {
        clearCalled = true;
    }

    void SetGravity(const glm::vec3& value) override
    {
        gravity = value;
    }

    void SetMode(int value) override
    {
        mode = value;
    }

    void SetSimulationSettings(float fixedTimeStep, int maxSubSteps) override
    {
        this->fixedTimeStep = fixedTimeStep;
        this->maxSubSteps = maxSubSteps;
    }

    void SetSolverIterations(int iterations) override
    {
        solverIterations = iterations;
    }

    void SetCCDEnabled(bool enabled, float threshold = 0.0f) override
    {
        ccdEnabled = enabled;
        ccdThreshold = threshold;
    }

    void AddRigidBody(IRigidBody* body) override
    {
        rigidBodies.push_back(body);
    }

    void RemoveRigidBody(IRigidBody* body) override
    {
        removedRigidBodies.push_back(body);
    }

    void AddCharacterController(ICharacterController* controller) override
    {
        characterControllers.push_back(controller);
    }

    void RemoveCharacterController(ICharacterController* controller) override
    {
        removedCharacterControllers.push_back(controller);
    }

    void AddConstraint(std::shared_ptr<IConstraint> constraint) override
    {
        constraints.push_back(std::move(constraint));
    }

    void RemoveConstraint(std::shared_ptr<IConstraint> constraint) override
    {
        removedConstraints.push_back(std::move(constraint));
    }

    void DebugDraw() override
    {
        debugDrawCalled = true;
    }

    RayHit Raycast(const glm::vec3&, const glm::vec3&, float, entt::entity ignore = entt::null) override
    {
        lastRaycastIgnore = ignore;
        return nextRayHit;
    }

    void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override
    {
        lastLineFrom = from;
        lastLineTo = to;
        lastLineColor = color;
        ++drawLineCount;
    }

    void SyncRigidBody(IRigidBody* body, const glm::vec3& pos, const glm::quat& rot) override
    {
        syncedRigidBodies.push_back(body);
        lastSyncPosition = pos;
        lastSyncRotation = rot;
    }

    void SetCollisionFilter(CollisionFilterCallback callback) override
    {
        collisionFilter = std::move(callback);
    }

    std::shared_ptr<IRigidBody> CreateRigidBody(float mass, const glm::vec3& startPos, const glm::quat& startRot,
                                                std::shared_ptr<ICollisionShape> shape) override
    {
        lastRigidBodyMass = mass;
        lastRigidBodyPosition = startPos;
        lastRigidBodyRotation = startRot;
        lastRigidBodyShape = std::move(shape);
        auto body = std::make_shared<FakeRigidBody>();
        body->position = startPos;
        body->rotation = startRot;
        createdRigidBodies.push_back(body);
        return body;
    }

    std::shared_ptr<ICollisionShape> CreateBoxShape(const glm::vec3& halfExtents) override
    {
        shapeCalls.push_back({"Box", halfExtents, 0.0f, 0.0f});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Box);
    }

    std::shared_ptr<ICollisionShape> CreateSphereShape(float radius) override
    {
        shapeCalls.push_back({"Sphere", glm::vec3(0.0f), radius, 0.0f});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Sphere);
    }

    std::shared_ptr<ICollisionShape> CreateCapsuleShape(float radius, float height) override
    {
        shapeCalls.push_back({"Capsule", glm::vec3(0.0f), radius, height});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Capsule);
    }

    std::shared_ptr<ICollisionShape> CreateCylinderShape(float radius, float height) override
    {
        shapeCalls.push_back({"Cylinder", glm::vec3(0.0f), radius, height});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Cylinder);
    }

    std::shared_ptr<ICharacterController> CreateCharacterController(std::shared_ptr<ICollisionShape>,
                                                                    float stepHeight = 0.35f) override
    {
        lastControllerStepHeight = stepHeight;
        auto controller = std::make_shared<FakeCharacterController>();
        createdCharacterControllers.push_back(controller);
        return controller;
    }

    std::shared_ptr<ICollisionShape> CreateCompoundShape() override
    {
        shapeCalls.push_back({"Compound", glm::vec3(0.0f), 0.0f, 0.0f});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::CompoundHull);
    }

    std::shared_ptr<ICollisionShape> CreateMeshShape(const std::vector<float>& vertices,
                                                     const std::vector<uint32_t>& indices) override
    {
        meshVertexCount = vertices.size();
        meshIndexCount = indices.size();
        shapeCalls.push_back({"Mesh", glm::vec3(0.0f), 0.0f, 0.0f});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Mesh);
    }

    std::shared_ptr<ICollisionShape> CreateHeightfieldShape(const std::vector<float>& heights, int width, int length,
                                                            float minHeight, float maxHeight) override
    {
        heightfieldSampleCount = heights.size();
        heightfieldWidth = width;
        heightfieldLength = length;
        heightfieldMin = minHeight;
        heightfieldMax = maxHeight;
        shapeCalls.push_back({"Heightfield", glm::vec3(0.0f), 0.0f, 0.0f});
        return std::make_shared<FakeCollisionShape>(CollisionShapeType::Heightfield);
    }

    void AddChildShape(std::shared_ptr<ICollisionShape> parent, std::shared_ptr<ICollisionShape> child,
                       const glm::vec3& pos, const glm::quat& rot) override
    {
        childShapeCalls.push_back({std::move(parent), std::move(child), pos, rot});
    }

    std::shared_ptr<IConstraint> CreatePoint2PointConstraint(std::shared_ptr<IRigidBody>, std::shared_ptr<IRigidBody>,
                                                             const glm::vec3&, const glm::vec3&) override
    {
        return std::make_shared<FakeConstraint>();
    }

    std::shared_ptr<IConstraint> CreateFixedConstraint(std::shared_ptr<IRigidBody>, std::shared_ptr<IRigidBody>,
                                                       const glm::vec3&, const glm::vec3&, const glm::quat&,
                                                       const glm::quat&) override
    {
        ++fixedConstraintCreateCount;
        return std::make_shared<FakeConstraint>();
    }

    std::shared_ptr<IConstraint> CreateHingeConstraint(std::shared_ptr<IRigidBody>, std::shared_ptr<IRigidBody>,
                                                       const glm::vec3&, const glm::vec3&, const glm::vec3&,
                                                       const glm::vec3&) override
    {
        return std::make_shared<FakeConstraint>();
    }

    void CollectActiveCollisions(std::vector<CollisionInfo>& out) override
    {
        out = activeCollisions;
    }

    std::vector<CollisionInfo> GetActiveCollisions() override
    {
        return activeCollisions;
    }

    bool initialized = false;
    bool clearCalled = false;
    bool debugDrawCalled = false;
    bool ccdEnabled = false;
    int mode = 0;
    int maxSubSteps = 0;
    int solverIterations = 0;
    int updateCount = 0;
    int drawLineCount = 0;
    int fixedConstraintCreateCount = 0;
    float fixedTimeStep = 0.0f;
    float ccdThreshold = 0.0f;
    float lastUpdateDt = 0.0f;
    float lastRigidBodyMass = 0.0f;
    float lastControllerStepHeight = 0.0f;
    glm::vec3 gravity = glm::vec3(0.0f);
    glm::vec3 lastRigidBodyPosition = glm::vec3(0.0f);
    glm::quat lastRigidBodyRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 lastSyncPosition = glm::vec3(0.0f);
    glm::quat lastSyncRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 lastLineFrom = glm::vec3(0.0f);
    glm::vec3 lastLineTo = glm::vec3(0.0f);
    glm::vec3 lastLineColor = glm::vec3(0.0f);
    entt::entity lastRaycastIgnore = entt::null;
    RayHit nextRayHit;
    CollisionFilterCallback collisionFilter;
    std::shared_ptr<ICollisionShape> lastRigidBodyShape;
    std::vector<ShapeCall> shapeCalls;
    std::vector<ChildShapeCall> childShapeCalls;
    std::vector<IRigidBody*> rigidBodies;
    std::vector<IRigidBody*> removedRigidBodies;
    std::vector<ICharacterController*> characterControllers;
    std::vector<ICharacterController*> removedCharacterControllers;
    std::vector<std::shared_ptr<IConstraint>> constraints;
    std::vector<std::shared_ptr<IConstraint>> removedConstraints;
    std::vector<std::shared_ptr<FakeRigidBody>> createdRigidBodies;
    std::vector<std::shared_ptr<FakeCharacterController>> createdCharacterControllers;
    std::vector<IRigidBody*> syncedRigidBodies;
    std::vector<CollisionInfo> activeCollisions;
    size_t meshVertexCount = 0;
    size_t meshIndexCount = 0;
    size_t heightfieldSampleCount = 0;
    int heightfieldWidth = 0;
    int heightfieldLength = 0;
    float heightfieldMin = 0.0f;
    float heightfieldMax = 0.0f;
};
}  // namespace axis_test_mocks
