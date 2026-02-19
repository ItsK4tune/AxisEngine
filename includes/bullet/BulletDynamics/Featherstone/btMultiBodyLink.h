

#ifndef BT_MULTIBODY_LINK_H
#define BT_MULTIBODY_LINK_H

#include "LinearMath/btQuaternion.h"
#include "LinearMath/btVector3.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"

enum btMultiBodyLinkFlags
{
	BT_MULTIBODYLINKFLAGS_DISABLE_PARENT_COLLISION = 1,
	BT_MULTIBODYLINKFLAGS_DISABLE_ALL_PARENT_COLLISION = 2,
};


#define BT_MULTIBODYLINK_INCLUDE_PLANAR_JOINTS
#define TEST_SPATIAL_ALGEBRA_LAYER







#include "LinearMath/btSpatialAlgebra.h"







struct btMultibodyLink
{
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btScalar m_mass;           
	btVector3 m_inertiaLocal;  

	int m_parent;  

	btQuaternion m_zeroRotParentToThis;  

	btVector3 m_dVector;  
						  

	
	
	
	

	btVector3 m_eVector;

	btSpatialMotionVector m_absFrameTotVelocity, m_absFrameLocVelocity;

	enum eFeatherstoneJointType
	{
		eRevolute = 0,
		ePrismatic = 1,
		eSpherical = 2,
		ePlanar = 3,
		eFixed = 4,
		eInvalid
	};

	
	
	
	
	
	
	
	
	
	
	
	
	
	btSpatialMotionVector m_axes[6];
	void setAxisTop(int dof, const btVector3 &axis) { m_axes[dof].m_topVec = axis; }
	void setAxisBottom(int dof, const btVector3 &axis)
	{
		m_axes[dof].m_bottomVec = axis;
	}
	void setAxisTop(int dof, const btScalar &x, const btScalar &y, const btScalar &z)
	{
		m_axes[dof].m_topVec.setValue(x, y, z);
	}
	void setAxisBottom(int dof, const btScalar &x, const btScalar &y, const btScalar &z)
	{
		m_axes[dof].m_bottomVec.setValue(x, y, z);
	}
	const btVector3 &getAxisTop(int dof) const { return m_axes[dof].m_topVec; }
	const btVector3 &getAxisBottom(int dof) const { return m_axes[dof].m_bottomVec; }

	int m_dofOffset, m_cfgOffset;

	btQuaternion m_cachedRotParentToThis;  
	btVector3 m_cachedRVector;             
    
    
    btQuaternion m_cachedRotParentToThis_interpolate;  
    btVector3 m_cachedRVector_interpolate;             

	btVector3 m_appliedForce;   
	btVector3 m_appliedTorque;  

	btVector3 m_appliedConstraintForce;   
	btVector3 m_appliedConstraintTorque;  

	btScalar m_jointPos[7];
    btScalar m_jointPos_interpolate[7];

	
	
	btScalar m_jointTorque[6];

	class btMultiBodyLinkCollider *m_collider;
	int m_flags;

	int m_dofCount, m_posVarCount;  

	eFeatherstoneJointType m_jointType;

	struct btMultiBodyJointFeedback *m_jointFeedback;

	btTransform m_cachedWorldTransform;  

	const char *m_linkName;   
	const char *m_jointName;  
	const void *m_userPtr;    

	btScalar m_jointDamping;      
	btScalar m_jointFriction;     
	btScalar m_jointLowerLimit;   
	btScalar m_jointUpperLimit;   
	btScalar m_jointMaxForce;     
	btScalar m_jointMaxVelocity;  

	
	btMultibodyLink()
		: m_mass(1),
		  m_parent(-1),
		  m_zeroRotParentToThis(0, 0, 0, 1),
		  m_cachedRotParentToThis(0, 0, 0, 1),
          m_cachedRotParentToThis_interpolate(0, 0, 0, 1),
		  m_collider(0),
		  m_flags(0),
		  m_dofCount(0),
		  m_posVarCount(0),
		  m_jointType(btMultibodyLink::eInvalid),
		  m_jointFeedback(0),
		  m_linkName(0),
		  m_jointName(0),
		  m_userPtr(0),
		  m_jointDamping(0),
		  m_jointFriction(0),
		  m_jointLowerLimit(0),
		  m_jointUpperLimit(0),
		  m_jointMaxForce(0),
		  m_jointMaxVelocity(0)
	{
		m_inertiaLocal.setValue(1, 1, 1);
		setAxisTop(0, 0., 0., 0.);
		setAxisBottom(0, 1., 0., 0.);
		m_dVector.setValue(0, 0, 0);
		m_eVector.setValue(0, 0, 0);
		m_cachedRVector.setValue(0, 0, 0);
        m_cachedRVector_interpolate.setValue(0, 0, 0);
		m_appliedForce.setValue(0, 0, 0);
		m_appliedTorque.setValue(0, 0, 0);
		m_appliedConstraintForce.setValue(0, 0, 0);
		m_appliedConstraintTorque.setValue(0, 0, 0);
		
		m_jointPos[0] = m_jointPos[1] = m_jointPos[2] = m_jointPos[4] = m_jointPos[5] = m_jointPos[6] = 0.f;
		m_jointPos[3] = 1.f;  
		m_jointTorque[0] = m_jointTorque[1] = m_jointTorque[2] = m_jointTorque[3] = m_jointTorque[4] = m_jointTorque[5] = 0.f;
		m_cachedWorldTransform.setIdentity();
	}

	
	void updateCacheMultiDof(btScalar *pq = 0)
	{
        btScalar *pJointPos = (pq ? pq : &m_jointPos[0]);
        btQuaternion& cachedRot = m_cachedRotParentToThis;
        btVector3& cachedVector = m_cachedRVector;
		switch (m_jointType)
		{
			case eRevolute:
			{
				cachedRot = btQuaternion(getAxisTop(0), -pJointPos[0]) * m_zeroRotParentToThis;
				cachedVector = m_dVector + quatRotate(m_cachedRotParentToThis, m_eVector);

				break;
			}
			case ePrismatic:
			{
				
				cachedVector = m_dVector + quatRotate(m_cachedRotParentToThis, m_eVector) + pJointPos[0] * getAxisBottom(0);

				break;
			}
			case eSpherical:
			{
				cachedRot = btQuaternion(pJointPos[0], pJointPos[1], pJointPos[2], -pJointPos[3]) * m_zeroRotParentToThis;
				cachedVector = m_dVector + quatRotate(cachedRot, m_eVector);

				break;
			}
			case ePlanar:
			{
				cachedRot = btQuaternion(getAxisTop(0), -pJointPos[0]) * m_zeroRotParentToThis;
				cachedVector = quatRotate(btQuaternion(getAxisTop(0), -pJointPos[0]), pJointPos[1] * getAxisBottom(1) + pJointPos[2] * getAxisBottom(2)) + quatRotate(cachedRot, m_eVector);

				break;
			}
			case eFixed:
			{
				cachedRot = m_zeroRotParentToThis;
				cachedVector = m_dVector + quatRotate(cachedRot, m_eVector);

				break;
			}
			default:
			{
				
				btAssert(0);
			}
		}
        m_cachedRotParentToThis_interpolate = m_cachedRotParentToThis;
        m_cachedRVector_interpolate = m_cachedRVector;
	}
    
    void updateInterpolationCacheMultiDof()
    {
        btScalar *pJointPos = &m_jointPos_interpolate[0];
        
        btQuaternion& cachedRot = m_cachedRotParentToThis_interpolate;
        btVector3& cachedVector = m_cachedRVector_interpolate;
        switch (m_jointType)
        {
            case eRevolute:
            {
                cachedRot = btQuaternion(getAxisTop(0), -pJointPos[0]) * m_zeroRotParentToThis;
                cachedVector = m_dVector + quatRotate(m_cachedRotParentToThis, m_eVector);
                
                break;
            }
            case ePrismatic:
            {
                
                cachedVector = m_dVector + quatRotate(m_cachedRotParentToThis, m_eVector) + pJointPos[0] * getAxisBottom(0);
                
                break;
            }
            case eSpherical:
            {
                cachedRot = btQuaternion(pJointPos[0], pJointPos[1], pJointPos[2], -pJointPos[3]) * m_zeroRotParentToThis;
                cachedVector = m_dVector + quatRotate(cachedRot, m_eVector);
                
                break;
            }
            case ePlanar:
            {
                cachedRot = btQuaternion(getAxisTop(0), -pJointPos[0]) * m_zeroRotParentToThis;
                cachedVector = quatRotate(btQuaternion(getAxisTop(0), -pJointPos[0]), pJointPos[1] * getAxisBottom(1) + pJointPos[2] * getAxisBottom(2)) + quatRotate(cachedRot, m_eVector);
                
                break;
            }
            case eFixed:
            {
                cachedRot = m_zeroRotParentToThis;
                cachedVector = m_dVector + quatRotate(cachedRot, m_eVector);
                
                break;
            }
            default:
            {
                
                btAssert(0);
            }
        }
    }

 

};

#endif  
