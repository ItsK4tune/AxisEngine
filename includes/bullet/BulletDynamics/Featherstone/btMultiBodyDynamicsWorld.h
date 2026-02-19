

#ifndef BT_MULTIBODY_DYNAMICS_WORLD_H
#define BT_MULTIBODY_DYNAMICS_WORLD_H

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/Featherstone/btMultiBodyInplaceSolverIslandCallback.h"

#define BT_USE_VIRTUAL_CLEARFORCES_AND_GRAVITY

class btMultiBody;
class btMultiBodyConstraint;
class btMultiBodyConstraintSolver;
struct MultiBodyInplaceSolverIslandCallback;



class btMultiBodyDynamicsWorld : public btDiscreteDynamicsWorld
{
protected:
	btAlignedObjectArray<btMultiBody*> m_multiBodies;
	btAlignedObjectArray<btMultiBodyConstraint*> m_multiBodyConstraints;
	btAlignedObjectArray<btMultiBodyConstraint*> m_sortedMultiBodyConstraints;
	btMultiBodyConstraintSolver* m_multiBodyConstraintSolver;
	MultiBodyInplaceSolverIslandCallback* m_solverMultiBodyIslandCallback;

	
	btAlignedObjectArray<btQuaternion> m_scratch_world_to_local;
	btAlignedObjectArray<btVector3> m_scratch_local_origin;
	btAlignedObjectArray<btQuaternion> m_scratch_world_to_local1;
	btAlignedObjectArray<btVector3> m_scratch_local_origin1;
	btAlignedObjectArray<btScalar> m_scratch_r;
	btAlignedObjectArray<btVector3> m_scratch_v;
	btAlignedObjectArray<btMatrix3x3> m_scratch_m;

	virtual void calculateSimulationIslands();
	virtual void updateActivationState(btScalar timeStep);
	

	virtual void serializeMultiBodies(btSerializer* serializer);

public:
	btMultiBodyDynamicsWorld(btDispatcher* dispatcher, btBroadphaseInterface* pairCache, btMultiBodyConstraintSolver* constraintSolver, btCollisionConfiguration* collisionConfiguration);

	virtual ~btMultiBodyDynamicsWorld();
    
    virtual void solveConstraints(btContactSolverInfo& solverInfo);
    
	virtual void addMultiBody(btMultiBody* body, int group = btBroadphaseProxy::DefaultFilter, int mask = btBroadphaseProxy::AllFilter);

	virtual void removeMultiBody(btMultiBody* body);

	virtual int getNumMultibodies() const
	{
		return m_multiBodies.size();
	}

	btMultiBody* getMultiBody(int mbIndex)
	{
		return m_multiBodies[mbIndex];
	}

	const btMultiBody* getMultiBody(int mbIndex) const
	{
		return m_multiBodies[mbIndex];
	}

	virtual void addMultiBodyConstraint(btMultiBodyConstraint* constraint);

	virtual int getNumMultiBodyConstraints() const
	{
		return m_multiBodyConstraints.size();
	}

	virtual btMultiBodyConstraint* getMultiBodyConstraint(int constraintIndex)
	{
		return m_multiBodyConstraints[constraintIndex];
	}

	virtual const btMultiBodyConstraint* getMultiBodyConstraint(int constraintIndex) const
	{
		return m_multiBodyConstraints[constraintIndex];
	}

	virtual void removeMultiBodyConstraint(btMultiBodyConstraint* constraint);

	virtual void integrateTransforms(btScalar timeStep);
    void integrateMultiBodyTransforms(btScalar timeStep);
    void predictMultiBodyTransforms(btScalar timeStep);
    
    virtual void predictUnconstraintMotion(btScalar timeStep);
	virtual void debugDrawWorld();

	virtual void debugDrawMultiBodyConstraint(btMultiBodyConstraint* constraint);

	void forwardKinematics();
	virtual void clearForces();
	virtual void clearMultiBodyConstraintForces();
	virtual void clearMultiBodyForces();
	virtual void applyGravity();

	virtual void serialize(btSerializer* serializer);
	virtual void setMultiBodyConstraintSolver(btMultiBodyConstraintSolver* solver);
	virtual void setConstraintSolver(btConstraintSolver* solver);
	virtual void getAnalyticsData(btAlignedObjectArray<struct btSolverAnalyticsData>& m_islandAnalyticsData) const;
    
    virtual void solveExternalForces(btContactSolverInfo& solverInfo);
    virtual void solveInternalConstraints(btContactSolverInfo& solverInfo);
    void buildIslands();

	virtual void saveKinematicState(btScalar timeStep);
};
#endif  
