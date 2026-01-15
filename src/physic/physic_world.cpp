#include <physic/physic_world.h>

PhysicsWorld::PhysicsWorld()
{
    collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
    overlappingPairCache = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(), overlappingPairCache.get(), solver.get(), collisionConfig.get());
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsWorld::~PhysicsWorld()
{
    Clear();
}

void PhysicsWorld::Update(float dt)
{
    dynamicsWorld->stepSimulation(dt, 10);
}

void PhysicsWorld::Clear()
{
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

    for (int i = 0; i < m_collisionShapes.size(); i++)
    {
        delete m_collisionShapes[i];
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

    dynamicsWorld->addRigidBody(body);
    return body;
}

void PhysicsWorld::RegisterShape(btCollisionShape* shape)
{
    if (shape) {
        m_collisionShapes.push_back(shape);
    }
}