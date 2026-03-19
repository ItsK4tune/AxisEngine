

#ifndef BT_SIMPLE_DYNAMICS_WORLD_H
#define BT_SIMPLE_DYNAMICS_WORLD_H

#include "btDynamicsWorld.h"

class btDispatcher;
class btOverlappingPairCache;
class btConstraintSolver;



class btSimpleDynamicsWorld : public btDynamicsWorld
{
protected:
	btConstraintSolver* m_constraintSolver;

	bool m_ownsConstraintSolver;

	void predictUnconstraintMotion(btScalar timeStep);

	void integrateTransforms(btScalar timeStep);

	btVector3 m_gravity;

public:
	
	btSimpleDynamicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* pairCache, btConstraintSolver* constraintSolver, btCollisionConfiguration* collisionConfiguration);

	virtual ~btSimpleDynamicsWorld();

	
	virtual int stepSimulation(btScalar timeStep, int maxSubSteps = 1, btScalar fixedTimeStep = btScalar(1.) / btScalar(60.));

	virtual void setGravity(const btVector3& gravity);

	virtual btVector3 getGravity() const;

	virtual void addRigidBody(btRigidBody* body);

	virtual void addRigidBody(btRigidBody* body, int group, int mask);

	virtual void removeRigidBody(btRigidBody* body);

	virtual void debugDrawWorld();

	virtual void addAction(btActionInterface* action);

	virtual void removeAction(btActionInterface* action);

	
	virtual void removeCollisionObject(btCollisionObject* collisionObject);

	virtual void updateAabbs();

	virtual void synchronizeMotionStates();

	virtual void setConstraintSolver(btConstraintSolver* solver);

	virtual btConstraintSolver* getConstraintSolver();

	virtual btDynamicsWorldType getWorldType() const
	{
		return BT_SIMPLE_DYNAMICS_WORLD;
	}

	virtual void clearForces();
};

#endif  
