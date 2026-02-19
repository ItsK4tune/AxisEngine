

#ifndef BT_DYNAMICS_WORLD_H
#define BT_DYNAMICS_WORLD_H

#include "BulletCollision/CollisionDispatch/btCollisionWorld.h"
#include "BulletDynamics/ConstraintSolver/btContactSolverInfo.h"

class btTypedConstraint;
class btActionInterface;
class btConstraintSolver;
class btDynamicsWorld;


typedef void (*btInternalTickCallback)(btDynamicsWorld* world, btScalar timeStep);

enum btDynamicsWorldType
{
	BT_SIMPLE_DYNAMICS_WORLD = 1,
	BT_DISCRETE_DYNAMICS_WORLD = 2,
	BT_CONTINUOUS_DYNAMICS_WORLD = 3,
	BT_SOFT_RIGID_DYNAMICS_WORLD = 4,
	BT_GPU_DYNAMICS_WORLD = 5,
	BT_SOFT_MULTIBODY_DYNAMICS_WORLD = 6,
    BT_DEFORMABLE_MULTIBODY_DYNAMICS_WORLD = 7
};


class btDynamicsWorld : public btCollisionWorld
{
protected:
	btInternalTickCallback m_internalTickCallback;
	btInternalTickCallback m_internalPreTickCallback;
	void* m_worldUserInfo;

	btContactSolverInfo m_solverInfo;

public:
	btDynamicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* broadphase, btCollisionConfiguration* collisionConfiguration)
		: btCollisionWorld(dispatcher, broadphase, collisionConfiguration), m_internalTickCallback(0), m_internalPreTickCallback(0), m_worldUserInfo(0)
	{
	}

	virtual ~btDynamicsWorld()
	{
	}

	
	
	
	
	virtual int stepSimulation(btScalar timeStep, int maxSubSteps = 1, btScalar fixedTimeStep = btScalar(1.) / btScalar(60.)) = 0;

	virtual void debugDrawWorld() = 0;

	virtual void addConstraint(btTypedConstraint* constraint, bool disableCollisionsBetweenLinkedBodies = false)
	{
		(void)constraint;
		(void)disableCollisionsBetweenLinkedBodies;
	}

	virtual void removeConstraint(btTypedConstraint* constraint) { (void)constraint; }

	virtual void addAction(btActionInterface* action) = 0;

	virtual void removeAction(btActionInterface* action) = 0;

	
	
	virtual void setGravity(const btVector3& gravity) = 0;
	virtual btVector3 getGravity() const = 0;

	virtual void synchronizeMotionStates() = 0;

	virtual void addRigidBody(btRigidBody* body) = 0;

	virtual void addRigidBody(btRigidBody* body, int group, int mask) = 0;

	virtual void removeRigidBody(btRigidBody* body) = 0;

	virtual void setConstraintSolver(btConstraintSolver* solver) = 0;

	virtual btConstraintSolver* getConstraintSolver() = 0;

	virtual int getNumConstraints() const { return 0; }

	virtual btTypedConstraint* getConstraint(int index)
	{
		(void)index;
		return 0;
	}

	virtual const btTypedConstraint* getConstraint(int index) const
	{
		(void)index;
		return 0;
	}

	virtual btDynamicsWorldType getWorldType() const = 0;

	virtual void clearForces() = 0;

	
	void setInternalTickCallback(btInternalTickCallback cb, void* worldUserInfo = 0, bool isPreTick = false)
	{
		if (isPreTick)
		{
			m_internalPreTickCallback = cb;
		}
		else
		{
			m_internalTickCallback = cb;
		}
		m_worldUserInfo = worldUserInfo;
	}

	void setWorldUserInfo(void* worldUserInfo)
	{
		m_worldUserInfo = worldUserInfo;
	}

	void* getWorldUserInfo() const
	{
		return m_worldUserInfo;
	}

	btContactSolverInfo& getSolverInfo()
	{
		return m_solverInfo;
	}

	const btContactSolverInfo& getSolverInfo() const
	{
		return m_solverInfo;
	}

	
	virtual void addVehicle(btActionInterface* vehicle) { (void)vehicle; }
	
	virtual void removeVehicle(btActionInterface* vehicle) { (void)vehicle; }
	
	virtual void addCharacter(btActionInterface* character) { (void)character; }
	
	virtual void removeCharacter(btActionInterface* character) { (void)character; }
};


struct btDynamicsWorldDoubleData
{
	btContactSolverInfoDoubleData m_solverInfo;
	btVector3DoubleData m_gravity;
};


struct btDynamicsWorldFloatData
{
	btContactSolverInfoFloatData m_solverInfo;
	btVector3FloatData m_gravity;
};

#endif  
