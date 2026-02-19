

#ifndef BT_KINEMATIC_CHARACTER_CONTROLLER_H
#define BT_KINEMATIC_CHARACTER_CONTROLLER_H

#include "LinearMath/btVector3.h"

#include "btCharacterControllerInterface.h"

#include "BulletCollision/BroadphaseCollision/btCollisionAlgorithm.h"

class btCollisionShape;
class btConvexShape;
class btRigidBody;
class btCollisionWorld;
class btCollisionDispatcher;
class btPairCachingGhostObject;




ATTRIBUTE_ALIGNED16(class)
btKinematicCharacterController : public btCharacterControllerInterface
{
protected:
	btScalar m_halfHeight;

	btPairCachingGhostObject* m_ghostObject;
	btConvexShape* m_convexShape;  

	btScalar m_maxPenetrationDepth;
	btScalar m_verticalVelocity;
	btScalar m_verticalOffset;
	btScalar m_fallSpeed;
	btScalar m_jumpSpeed;
	btScalar m_SetjumpSpeed;
	btScalar m_maxJumpHeight;
	btScalar m_maxSlopeRadians;  
	btScalar m_maxSlopeCosine;   
	btScalar m_gravity;

	btScalar m_turnAngle;

	btScalar m_stepHeight;

	btScalar m_addedMargin;  

	
	btVector3 m_walkDirection;
	btVector3 m_normalizedDirection;
	btVector3 m_AngVel;

	btVector3 m_jumpPosition;

	
	btVector3 m_currentPosition;
	btScalar m_currentStepOffset;
	btVector3 m_targetPosition;

	btQuaternion m_currentOrientation;
	btQuaternion m_targetOrientation;

	
	btManifoldArray m_manifoldArray;

	bool m_touchingContact;
	btVector3 m_touchingNormal;

	btScalar m_linearDamping;
	btScalar m_angularDamping;

	bool m_wasOnGround;
	bool m_wasJumping;
	bool m_useGhostObjectSweepTest;
	bool m_useWalkDirection;
	btScalar m_velocityTimeInterval;
	btVector3 m_up;
	btVector3 m_jumpAxis;

	static btVector3* getUpAxisDirections();
	bool m_interpolateUp;
	bool full_drop;
	bool bounce_fix;

	btVector3 computeReflectionDirection(const btVector3& direction, const btVector3& normal);
	btVector3 parallelComponent(const btVector3& direction, const btVector3& normal);
	btVector3 perpindicularComponent(const btVector3& direction, const btVector3& normal);

	bool recoverFromPenetration(btCollisionWorld * collisionWorld);
	void stepUp(btCollisionWorld * collisionWorld);
	void updateTargetPositionBasedOnCollision(const btVector3& hit_normal, btScalar tangentMag = btScalar(0.0), btScalar normalMag = btScalar(1.0));
	void stepForwardAndStrafe(btCollisionWorld * collisionWorld, const btVector3& walkMove);
	void stepDown(btCollisionWorld * collisionWorld, btScalar dt);

	virtual bool needsCollision(const btCollisionObject* body0, const btCollisionObject* body1);

	void setUpVector(const btVector3& up);

	btQuaternion getRotation(btVector3 & v0, btVector3 & v1) const;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btKinematicCharacterController(btPairCachingGhostObject * ghostObject, btConvexShape * convexShape, btScalar stepHeight, const btVector3& up = btVector3(1.0, 0.0, 0.0));
	~btKinematicCharacterController();

	
	virtual void updateAction(btCollisionWorld * collisionWorld, btScalar deltaTime)
	{
		preStep(collisionWorld);
		playerStep(collisionWorld, deltaTime);
	}

	
	void debugDraw(btIDebugDraw * debugDrawer);

	void setUp(const btVector3& up);

	const btVector3& getUp() { return m_up; }

	
	
	
	
	
	virtual void setWalkDirection(const btVector3& walkDirection);

	
	
	
	
	
	virtual void setVelocityForTimeInterval(const btVector3& velocity,
											btScalar timeInterval);

	virtual void setAngularVelocity(const btVector3& velocity);
	virtual const btVector3& getAngularVelocity() const;

	virtual void setLinearVelocity(const btVector3& velocity);
	virtual btVector3 getLinearVelocity() const;

	void setLinearDamping(btScalar d) { m_linearDamping = btClamped(d, (btScalar)btScalar(0.0), (btScalar)btScalar(1.0)); }
	btScalar getLinearDamping() const { return m_linearDamping; }
	void setAngularDamping(btScalar d) { m_angularDamping = btClamped(d, (btScalar)btScalar(0.0), (btScalar)btScalar(1.0)); }
	btScalar getAngularDamping() const { return m_angularDamping; }

	void reset(btCollisionWorld * collisionWorld);
	void warp(const btVector3& origin);

	void preStep(btCollisionWorld * collisionWorld);
	void playerStep(btCollisionWorld * collisionWorld, btScalar dt);

	void setStepHeight(btScalar h);
	btScalar getStepHeight() const { return m_stepHeight; }
	void setFallSpeed(btScalar fallSpeed);
	btScalar getFallSpeed() const { return m_fallSpeed; }
	void setJumpSpeed(btScalar jumpSpeed);
	btScalar getJumpSpeed() const { return m_jumpSpeed; }
	void setMaxJumpHeight(btScalar maxJumpHeight);
	bool canJump() const;

	void jump(const btVector3& v = btVector3(0, 0, 0));

	void applyImpulse(const btVector3& v) { jump(v); }

	void setGravity(const btVector3& gravity);
	btVector3 getGravity() const;

	
	
	void setMaxSlope(btScalar slopeRadians);
	btScalar getMaxSlope() const;

	void setMaxPenetrationDepth(btScalar d);
	btScalar getMaxPenetrationDepth() const;

	btPairCachingGhostObject* getGhostObject();
	void setUseGhostSweepTest(bool useGhostObjectSweepTest)
	{
		m_useGhostObjectSweepTest = useGhostObjectSweepTest;
	}

	bool onGround() const;
	void setUpInterpolate(bool value);
};

#endif  
