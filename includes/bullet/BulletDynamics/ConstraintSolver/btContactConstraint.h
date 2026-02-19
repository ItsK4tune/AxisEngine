

#ifndef BT_CONTACT_CONSTRAINT_H
#define BT_CONTACT_CONSTRAINT_H

#include "LinearMath/btVector3.h"
#include "btJacobianEntry.h"
#include "btTypedConstraint.h"
#include "BulletCollision/NarrowPhaseCollision/btPersistentManifold.h"


ATTRIBUTE_ALIGNED16(class)
btContactConstraint : public btTypedConstraint
{
protected:
	btPersistentManifold m_contactManifold;

protected:
	btContactConstraint(btPersistentManifold * contactManifold, btRigidBody & rbA, btRigidBody & rbB);

public:
	void setContactManifold(btPersistentManifold * contactManifold);

	btPersistentManifold* getContactManifold()
	{
		return &m_contactManifold;
	}

	const btPersistentManifold* getContactManifold() const
	{
		return &m_contactManifold;
	}

	virtual ~btContactConstraint();

	virtual void getInfo1(btConstraintInfo1 * info);

	virtual void getInfo2(btConstraintInfo2 * info);

	
	virtual void buildJacobian();
};


btScalar resolveSingleCollision(btRigidBody* body1, class btCollisionObject* colObj2, const btVector3& contactPositionWorld, const btVector3& contactNormalOnB, const struct btContactSolverInfo& solverInfo, btScalar distance);


void resolveSingleBilateral(btRigidBody& body1, const btVector3& pos1,
							btRigidBody& body2, const btVector3& pos2,
							btScalar distance, const btVector3& normal, btScalar& impulse, btScalar timeStep);

#endif  
