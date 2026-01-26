#include <physic/physic_world.h>
#include <set>
#include <utils/logger.h>

PhysicsWorld::PhysicsWorld()
{
    collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
    overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfig.get());
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

    debugDrawer = std::make_unique<DebugDrawer>();
    debugDrawer->Init();
    dynamicsWorld->setDebugDrawer(debugDrawer.get());
}

void PhysicsWorld::SetMode(int mode)
{
    if (!dynamicsWorld)
        return;

    auto& solverInfo = dynamicsWorld->getSolverInfo();

    switch (mode)
    {
    case 0: // FAST
        solverInfo.m_numIterations = 2; // Extreme optimization
        solverInfo.m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING; 
        solverInfo.m_splitImpulse = 0;
        m_linearSleepingThreshold = 2.0f; // Sleep aggressively
        m_angularSleepingThreshold = 2.0f;
        m_timeStep = 1.0f / 30.0f; // Half rate updates
        m_maxSubSteps = 2;
        break;
    
    case 2: // ACCURATE
        solverInfo.m_numIterations = 40;
        solverInfo.m_globalCfm = 0.00001f; // Better stability
        solverInfo.m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
        solverInfo.m_splitImpulse = 1;
        m_linearSleepingThreshold = 0.01f; // Sleep rarely
        m_angularSleepingThreshold = 0.01f;
        m_timeStep = 1.0f / 120.0f; // Double rate updates
        m_maxSubSteps = 10;
        break;

    case 1: // BALANCED
    default:
        solverInfo.m_numIterations = 10;
        solverInfo.m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
        solverInfo.m_splitImpulse = 1;
        m_linearSleepingThreshold = 0.2f;
        m_angularSleepingThreshold = 0.2f;
        m_timeStep = 1.0f / 60.0f;
        m_maxSubSteps = 4;
        break;
    }

    // Apply new sleeping thresholds to all existing bodies
    int numObjects = dynamicsWorld->getNumCollisionObjects();
    auto objectArray = dynamicsWorld->getCollisionObjectArray();
    for (int i = 0; i < numObjects; i++)
    {
        btRigidBody* body = btRigidBody::upcast(objectArray[i]);
        if (body)
        {
            body->setSleepingThresholds(m_linearSleepingThreshold, m_angularSleepingThreshold);
        }
    }
    
    LOGGER_INFO("PhysicsWorld") << "Set physics mode: " << mode;
}

PhysicsWorld::~PhysicsWorld()
{
    Clear();
}

void PhysicsWorld::Update(float dt)
{
    // Use fixed timestep for stability and performance control
    dynamicsWorld->stepSimulation(dt, m_maxSubSteps, m_timeStep);
}

void PhysicsWorld::Clear()
{

    for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
    {
        btTypedConstraint* constraint = dynamicsWorld->getConstraint(i);
        dynamicsWorld->removeConstraint(constraint);
        delete constraint;
    }

    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }

    std::set<btCollisionShape*> uniqueShapes(m_collisionShapes.begin(), m_collisionShapes.end());
    for (btCollisionShape* shape : uniqueShapes)
    {
        delete shape;
    }
    m_collisionShapes.clear();
}

btDiscreteDynamicsWorld *PhysicsWorld::GetWorld() { return dynamicsWorld.get(); }

btRigidBody *PhysicsWorld::CreateRigidBody(float mass, const btTransform &startTransform, btCollisionShape *shape)
{
    m_collisionShapes.push_back(shape);

    bool isDynamic = (mass != 0.f);
    btVector3 localInertia(0, 0, 0);
    if (isDynamic)
        shape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
    btRigidBody *body = new btRigidBody(rbInfo);
    
    body->setSleepingThresholds(m_linearSleepingThreshold, m_angularSleepingThreshold);

    dynamicsWorld->addRigidBody(body);
    return body;
}

void PhysicsWorld::RegisterShape(btCollisionShape* shape)
{
    if (shape) {
        m_collisionShapes.push_back(shape);
    }
}

void PhysicsWorld::AddConstraint(btTypedConstraint* constraint, bool disableCollisionsBetweenLinkedBodies)
{
    dynamicsWorld->addConstraint(constraint, disableCollisionsBetweenLinkedBodies);
}

void PhysicsWorld::RemoveConstraint(btTypedConstraint* constraint)
{
    dynamicsWorld->removeConstraint(constraint);
}