

#ifndef BT_MULTIBODY_H
#define BT_MULTIBODY_H

#include "LinearMath/btScalar.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btMatrix3x3.h"
#include "LinearMath/btAlignedObjectArray.h"


#ifdef BT_USE_DOUBLE_PRECISION
#define btMultiBodyData btMultiBodyDoubleData
#define btMultiBodyDataName "btMultiBodyDoubleData"
#define btMultiBodyLinkData btMultiBodyLinkDoubleData
#define btMultiBodyLinkDataName "btMultiBodyLinkDoubleData"
#else
#define btMultiBodyData btMultiBodyFloatData
#define btMultiBodyDataName "btMultiBodyFloatData"
#define btMultiBodyLinkData btMultiBodyLinkFloatData
#define btMultiBodyLinkDataName "btMultiBodyLinkFloatData"
#endif  

#include "btMultiBodyLink.h"
class btMultiBodyLinkCollider;

ATTRIBUTE_ALIGNED16(class)
btMultiBody
{
public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	
	
	

	btMultiBody(int n_links,               
				btScalar mass,             
				const btVector3 &inertia,  
				bool fixedBase,            
				bool canSleep, bool deprecatedMultiDof = true);

	virtual ~btMultiBody();

	
	void setupFixed(int i, 
					btScalar mass,
					const btVector3 &inertia,
					int parent,
					const btQuaternion &rotParentToThis,
					const btVector3 &parentComToThisPivotOffset,
					const btVector3 &thisPivotToThisComOffset, bool deprecatedDisableParentCollision = true);

	void setupPrismatic(int i,
						btScalar mass,
						const btVector3 &inertia,
						int parent,
						const btQuaternion &rotParentToThis,
						const btVector3 &jointAxis,
						const btVector3 &parentComToThisPivotOffset,
						const btVector3 &thisPivotToThisComOffset,
						bool disableParentCollision);

	void setupRevolute(int i,  
					   btScalar mass,
					   const btVector3 &inertia,
					   int parentIndex,
					   const btQuaternion &rotParentToThis,          
					   const btVector3 &jointAxis,                   
					   const btVector3 &parentComToThisPivotOffset,  
					   const btVector3 &thisPivotToThisComOffset,    
					   bool disableParentCollision = false);

	void setupSpherical(int i,  
						btScalar mass,
						const btVector3 &inertia,
						int parent,
						const btQuaternion &rotParentToThis,          
						const btVector3 &parentComToThisPivotOffset,  
						const btVector3 &thisPivotToThisComOffset,    
						bool disableParentCollision = false);

	void setupPlanar(int i,  
					 btScalar mass,
					 const btVector3 &inertia,
					 int parent,
					 const btQuaternion &rotParentToThis,  
					 const btVector3 &rotationAxis,
					 const btVector3 &parentComToThisComOffset,  
					 bool disableParentCollision = false);

	const btMultibodyLink &getLink(int index) const
	{
		return m_links[index];
	}

	btMultibodyLink &getLink(int index)
	{
		return m_links[index];
	}

	void setBaseCollider(btMultiBodyLinkCollider * collider)  
	{
		m_baseCollider = collider;
	}
	const btMultiBodyLinkCollider *getBaseCollider() const
	{
		return m_baseCollider;
	}
	btMultiBodyLinkCollider *getBaseCollider()
	{
		return m_baseCollider;
	}

	const btMultiBodyLinkCollider *getLinkCollider(int index) const
	{
		if (index >= 0 && index < getNumLinks())
		{
			return getLink(index).m_collider;
		}
		return 0;
	}

	btMultiBodyLinkCollider *getLinkCollider(int index)
	{
		if (index >= 0 && index < getNumLinks())
		{
			return getLink(index).m_collider;
		}
		return 0;
	}

	
	
	
	
	
	int getParent(int link_num) const;

	
	
	

	int getNumLinks() const { return m_links.size(); }
	int getNumDofs() const { return m_dofCount; }
	int getNumPosVars() const { return m_posVarCnt; }
	btScalar getBaseMass() const { return m_baseMass; }
	const btVector3 &getBaseInertia() const { return m_baseInertia; }
	btScalar getLinkMass(int i) const;
	const btVector3 &getLinkInertia(int i) const;

	
	
	

	void setBaseMass(btScalar mass) { m_baseMass = mass; }
	void setBaseInertia(const btVector3 &inertia) { m_baseInertia = inertia; }

	
	
	

	const btVector3 &getBasePos() const 
	{ 
		return m_basePos; 
	}  
	const btVector3 getBaseVel() const
	{
		return btVector3(m_realBuf[3], m_realBuf[4], m_realBuf[5]);
	}  
	const btQuaternion &getWorldToBaseRot() const
	{
		return m_baseQuat;
	}
    
    const btVector3 &getInterpolateBasePos() const
    {
        return m_basePos_interpolate;
    }  
    const btQuaternion &getInterpolateWorldToBaseRot() const
    {
        return m_baseQuat_interpolate;
    }
    
    
	btVector3 getBaseOmega() const { return btVector3(m_realBuf[0], m_realBuf[1], m_realBuf[2]); }  

	void setBasePos(const btVector3 &pos)
	{
		m_basePos = pos;
		if(!isBaseKinematic())
			m_basePos_interpolate = pos;
	}

	void setInterpolateBasePos(const btVector3 &pos)
	{
		m_basePos_interpolate = pos;
	}

	void setBaseWorldTransform(const btTransform &tr)
	{
		setBasePos(tr.getOrigin());
		setWorldToBaseRot(tr.getRotation().inverse());
	}

	btTransform getBaseWorldTransform() const
	{
		btTransform tr;
		tr.setOrigin(getBasePos());
		tr.setRotation(getWorldToBaseRot().inverse());
		return tr;
	}

	void setInterpolateBaseWorldTransform(const btTransform &tr)
	{
		setInterpolateBasePos(tr.getOrigin());
		setInterpolateWorldToBaseRot(tr.getRotation().inverse());
	}

	btTransform getInterpolateBaseWorldTransform() const
	{
		btTransform tr;
		tr.setOrigin(getInterpolateBasePos());
		tr.setRotation(getInterpolateWorldToBaseRot().inverse());
		return tr;
	}

	void setBaseVel(const btVector3 &vel)
	{
		m_realBuf[3] = vel[0];
		m_realBuf[4] = vel[1];
		m_realBuf[5] = vel[2];
	}

	void setWorldToBaseRot(const btQuaternion &rot)
	{
		m_baseQuat = rot;  
		if(!isBaseKinematic())
			m_baseQuat_interpolate = rot;
	}

	void setInterpolateWorldToBaseRot(const btQuaternion &rot)
	{
		m_baseQuat_interpolate = rot;
	}

	void setBaseOmega(const btVector3 &omega)
	{
		m_realBuf[0] = omega[0];
		m_realBuf[1] = omega[1];
		m_realBuf[2] = omega[2];
	}

	void saveKinematicState(btScalar timeStep);

	
	
	

	btScalar getJointPos(int i) const;
	btScalar getJointVel(int i) const;

	btScalar *getJointVelMultiDof(int i);
	btScalar *getJointPosMultiDof(int i);

	const btScalar *getJointVelMultiDof(int i) const;
	const btScalar *getJointPosMultiDof(int i) const;

	void setJointPos(int i, btScalar q);
	void setJointVel(int i, btScalar qdot);
	void setJointPosMultiDof(int i, const double *q);
	void setJointVelMultiDof(int i, const double *qdot);
	void setJointPosMultiDof(int i, const float *q);
	void setJointVelMultiDof(int i, const float *qdot);

	
	
	
	
	const btScalar *getVelocityVector() const
	{
		return &m_realBuf[0];
	}
    
    const btScalar *getDeltaVelocityVector() const
    {
        return &m_deltaV[0];
    }
    
    const btScalar *getSplitVelocityVector() const
    {
        return &m_splitV[0];
    }
	

	
	
	
	

	const btVector3 &getRVector(int i) const;              
	const btQuaternion &getParentToLocalRot(int i) const;  
    const btVector3 &getInterpolateRVector(int i) const;              
    const btQuaternion &getInterpolateParentToLocalRot(int i) const;  

	
	
	
	btVector3 localPosToWorld(int i, const btVector3 &local_pos) const;
	btVector3 localDirToWorld(int i, const btVector3 &local_dir) const;
	btVector3 worldPosToLocal(int i, const btVector3 &world_pos) const;
	btVector3 worldDirToLocal(int i, const btVector3 &world_dir) const;

	
	
	
	btMatrix3x3 localFrameToWorld(int i, const btMatrix3x3 &local_frame) const;


	
	
	

	void clearForcesAndTorques();
	void clearConstraintForces();

	void clearVelocities();

	void addBaseForce(const btVector3 &f)
	{
		m_baseForce += f;
	}
	void addBaseTorque(const btVector3 &t) { m_baseTorque += t; }
	void addLinkForce(int i, const btVector3 &f);
	void addLinkTorque(int i, const btVector3 &t);

	void addBaseConstraintForce(const btVector3 &f)
	{
		m_baseConstraintForce += f;
	}
	void addBaseConstraintTorque(const btVector3 &t) { m_baseConstraintTorque += t; }
	void addLinkConstraintForce(int i, const btVector3 &f);
	void addLinkConstraintTorque(int i, const btVector3 &t);

	void addJointTorque(int i, btScalar Q);
	void addJointTorqueMultiDof(int i, int dof, btScalar Q);
	void addJointTorqueMultiDof(int i, const btScalar *Q);

	const btVector3 &getBaseForce() const { return m_baseForce; }
	const btVector3 &getBaseTorque() const { return m_baseTorque; }
	const btVector3 &getLinkForce(int i) const;
	const btVector3 &getLinkTorque(int i) const;
	btScalar getJointTorque(int i) const;
	btScalar *getJointTorqueMultiDof(int i);

	
	
	

	
	
	
	
	
	
	
	
	
	
	
	

	void computeAccelerationsArticulatedBodyAlgorithmMultiDof(btScalar dt,
															  btAlignedObjectArray<btScalar> & scratch_r,
															  btAlignedObjectArray<btVector3> & scratch_v,
															  btAlignedObjectArray<btMatrix3x3> & scratch_m,
															  bool isConstraintPass,
                                                              bool jointFeedbackInWorldSpace,
                                                              bool jointFeedbackInJointFrame
                                                              );

	
	
	
	
	
	
	
	
	

	
	
	
	
	
	
	void calcAccelerationDeltasMultiDof(const btScalar *force, btScalar *output,
										btAlignedObjectArray<btScalar> &scratch_r,
										btAlignedObjectArray<btVector3> &scratch_v) const;

	void applyDeltaVeeMultiDof2(const btScalar *delta_vee, btScalar multiplier)
	{
		for (int dof = 0; dof < 6 + getNumDofs(); ++dof)
		{
			m_deltaV[dof] += delta_vee[dof] * multiplier;
		}
	}
    void applyDeltaSplitVeeMultiDof(const btScalar *delta_vee, btScalar multiplier)
    {
        for (int dof = 0; dof < 6 + getNumDofs(); ++dof)
        {
            m_splitV[dof] += delta_vee[dof] * multiplier;
        }
    }
    void addSplitV()
    {
        applyDeltaVeeMultiDof(&m_splitV[0], 1);
    }
    void substractSplitV()
    {
        applyDeltaVeeMultiDof(&m_splitV[0], -1);
        
        for (int dof = 0; dof < 6 + getNumDofs(); ++dof)
        {
            m_splitV[dof] = 0.f;
        }
    }
	void processDeltaVeeMultiDof2()
	{
		applyDeltaVeeMultiDof(&m_deltaV[0], 1);

		for (int dof = 0; dof < 6 + getNumDofs(); ++dof)
		{
			m_deltaV[dof] = 0.f;
		}
	}

	void applyDeltaVeeMultiDof(const btScalar *delta_vee, btScalar multiplier)
	{
		
		
		

		
		
		
		
		
		

		
		
		
		

		for (int dof = 0; dof < 6 + getNumDofs(); ++dof)
		{
			m_realBuf[dof] += delta_vee[dof] * multiplier;
			btClamp(m_realBuf[dof], -m_maxCoordinateVelocity, m_maxCoordinateVelocity);
		}
	}

	
	void stepPositionsMultiDof(btScalar dt, btScalar *pq = 0, btScalar *pqd = 0);
    
    
    void predictPositionsMultiDof(btScalar dt);

	
	
	

	
	
	

	void fillContactJacobianMultiDof(int link,
									 const btVector3 &contact_point,
									 const btVector3 &normal,
									 btScalar *jac,
									 btAlignedObjectArray<btScalar> &scratch_r,
									 btAlignedObjectArray<btVector3> &scratch_v,
									 btAlignedObjectArray<btMatrix3x3> &scratch_m) const { fillConstraintJacobianMultiDof(link, contact_point, btVector3(0, 0, 0), normal, jac, scratch_r, scratch_v, scratch_m); }

	
	
	void fillConstraintJacobianMultiDof(int link,
										const btVector3 &contact_point,
										const btVector3 &normal_ang,
										const btVector3 &normal_lin,
										btScalar *jac,
										btAlignedObjectArray<btScalar> &scratch_r,
										btAlignedObjectArray<btVector3> &scratch_v,
										btAlignedObjectArray<btMatrix3x3> &scratch_m) const;

	
	
	
	void setCanSleep(bool canSleep)
	{
		if (m_canWakeup)
		{
			m_canSleep = canSleep;
		}
	}

	bool getCanSleep() const
	{
		return m_canSleep;
	}

	bool getCanWakeup() const
	{
		return m_canWakeup;
	}
	
	void setCanWakeup(bool canWakeup) 
	{
		m_canWakeup = canWakeup;
	}
	bool isAwake() const 
	{ 
		return m_awake; 
	}
	void wakeUp();
	void goToSleep();
	void checkMotionAndSleepIfRequired(btScalar timestep);

	bool hasFixedBase() const;

	bool isBaseKinematic() const;

	bool isBaseStaticOrKinematic() const;

	
	void setBaseDynamicType(int dynamicType);

	void setFixedBase(bool fixedBase)
	{
		m_fixedBase = fixedBase;
		if(m_fixedBase)
			setBaseDynamicType(btCollisionObject::CF_STATIC_OBJECT);
		else
			setBaseDynamicType(btCollisionObject::CF_DYNAMIC_OBJECT);
	}

	int getCompanionId() const
	{
		return m_companionId;
	}
	void setCompanionId(int id)
	{
		
		m_companionId = id;
	}

	void setNumLinks(int numLinks)  
	{
		m_links.resize(numLinks);
	}

	btScalar getLinearDamping() const
	{
		return m_linearDamping;
	}
	void setLinearDamping(btScalar damp)
	{
		m_linearDamping = damp;
	}
	btScalar getAngularDamping() const
	{
		return m_angularDamping;
	}
	void setAngularDamping(btScalar damp)
	{
		m_angularDamping = damp;
	}

	bool getUseGyroTerm() const
	{
		return m_useGyroTerm;
	}
	void setUseGyroTerm(bool useGyro)
	{
		m_useGyroTerm = useGyro;
	}
	btScalar getMaxCoordinateVelocity() const
	{
		return m_maxCoordinateVelocity;
	}
	void setMaxCoordinateVelocity(btScalar maxVel)
	{
		m_maxCoordinateVelocity = maxVel;
	}

	btScalar getMaxAppliedImpulse() const
	{
		return m_maxAppliedImpulse;
	}
	void setMaxAppliedImpulse(btScalar maxImp)
	{
		m_maxAppliedImpulse = maxImp;
	}
	void setHasSelfCollision(bool hasSelfCollision)
	{
		m_hasSelfCollision = hasSelfCollision;
	}
	bool hasSelfCollision() const
	{
		return m_hasSelfCollision;
	}

	void finalizeMultiDof();

	void useRK4Integration(bool use) { m_useRK4 = use; }
	bool isUsingRK4Integration() const { return m_useRK4; }
	void useGlobalVelocities(bool use) { m_useGlobalVelocities = use; }
	bool isUsingGlobalVelocities() const { return m_useGlobalVelocities; }

	bool isPosUpdated() const
	{
		return __posUpdated;
	}
	void setPosUpdated(bool updated)
	{
		__posUpdated = updated;
	}

	
	bool internalNeedsJointFeedback() const
	{
		return m_internalNeedsJointFeedback;
	}
	void forwardKinematics(btAlignedObjectArray<btQuaternion>& world_to_local, btAlignedObjectArray<btVector3> & local_origin);

	void compTreeLinkVelocities(btVector3 * omega, btVector3 * vel) const;

	void updateCollisionObjectWorldTransforms(btAlignedObjectArray<btQuaternion> & world_to_local, btAlignedObjectArray<btVector3> & local_origin);
    void updateCollisionObjectInterpolationWorldTransforms(btAlignedObjectArray<btQuaternion> & world_to_local, btAlignedObjectArray<btVector3> & local_origin);

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char *serialize(void *dataBuffer, class btSerializer *serializer) const;

	const char *getBaseName() const
	{
		return m_baseName;
	}
	
	void setBaseName(const char *name)
	{
		m_baseName = name;
	}

	
	void *getUserPointer() const
	{
		return m_userObjectPointer;
	}

	int getUserIndex() const
	{
		return m_userIndex;
	}

	int getUserIndex2() const
	{
		return m_userIndex2;
	}
	
	void setUserPointer(void *userPointer)
	{
		m_userObjectPointer = userPointer;
	}

	
	void setUserIndex(int index)
	{
		m_userIndex = index;
	}

	void setUserIndex2(int index)
	{
		m_userIndex2 = index;
	}

	static void spatialTransform(const btMatrix3x3 &rotation_matrix,  
		const btVector3 &displacement,     
		const btVector3 &top_in,       
		const btVector3 &bottom_in,    
		btVector3 &top_out,         
		btVector3 &bottom_out);      

	void setLinkDynamicType(const int i, int type);

	bool isLinkStaticOrKinematic(const int i) const;

	bool isLinkKinematic(const int i) const;

	bool isLinkAndAllAncestorsStaticOrKinematic(const int i) const;

	bool isLinkAndAllAncestorsKinematic(const int i) const;

	void setSleepThreshold(btScalar sleepThreshold)
	{
		m_sleepEpsilon = sleepThreshold;
	}

	void setSleepTimeout(btScalar sleepTimeout)
	{
		this->m_sleepTimeout = sleepTimeout;
	}


private:
	btMultiBody(const btMultiBody &);     
	void operator=(const btMultiBody &);  

	void solveImatrix(const btVector3 &rhs_top, const btVector3 &rhs_bot, btScalar result[6]) const;
	void solveImatrix(const btSpatialForceVector &rhs, btSpatialMotionVector &result) const;

	void updateLinksDofOffsets()
	{
		int dofOffset = 0, cfgOffset = 0;
		for (int bidx = 0; bidx < m_links.size(); ++bidx)
		{
			m_links[bidx].m_dofOffset = dofOffset;
			m_links[bidx].m_cfgOffset = cfgOffset;
			dofOffset += m_links[bidx].m_dofCount;
			cfgOffset += m_links[bidx].m_posVarCount;
		}
	}

	void mulMatrix(const btScalar *pA, const btScalar *pB, int rowsA, int colsA, int rowsB, int colsB, btScalar *pC) const;

private:
	btMultiBodyLinkCollider *m_baseCollider;  
	const char *m_baseName;                   

	btVector3 m_basePos;      
    btVector3 m_basePos_interpolate;      
	btQuaternion m_baseQuat;  
    btQuaternion m_baseQuat_interpolate;  

	btScalar m_baseMass;      
	btVector3 m_baseInertia;  

	btVector3 m_baseForce;   
	btVector3 m_baseTorque;  

	btVector3 m_baseConstraintForce;   
	btVector3 m_baseConstraintTorque;  

	btAlignedObjectArray<btMultibodyLink> m_links;  

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
    btAlignedObjectArray<btScalar> m_splitV;
	btAlignedObjectArray<btScalar> m_deltaV;
	btAlignedObjectArray<btScalar> m_realBuf;
	btAlignedObjectArray<btVector3> m_vectorBuf;
	btAlignedObjectArray<btMatrix3x3> m_matrixBuf;

	btMatrix3x3 m_cachedInertiaTopLeft;
	btMatrix3x3 m_cachedInertiaTopRight;
	btMatrix3x3 m_cachedInertiaLowerLeft;
	btMatrix3x3 m_cachedInertiaLowerRight;
	bool m_cachedInertiaValid;

	bool m_fixedBase;

	
	bool m_awake;
	bool m_canSleep;
	bool m_canWakeup;
	btScalar m_sleepTimer;
	btScalar m_sleepEpsilon;
	btScalar m_sleepTimeout;

	void *m_userObjectPointer;
	int m_userIndex2;
	int m_userIndex;

	int m_companionId;
	btScalar m_linearDamping;
	btScalar m_angularDamping;
	bool m_useGyroTerm;
	btScalar m_maxAppliedImpulse;
	btScalar m_maxCoordinateVelocity;
	bool m_hasSelfCollision;

	bool __posUpdated;
	int m_dofCount, m_posVarCnt;

	bool m_useRK4, m_useGlobalVelocities;
	
	

	
	bool m_internalNeedsJointFeedback;

  
	bool m_kinematic_calculate_velocity;
};

struct btMultiBodyLinkDoubleData
{
	btQuaternionDoubleData m_zeroRotParentToThis;
	btVector3DoubleData m_parentComToThisPivotOffset;
	btVector3DoubleData m_thisPivotToThisComOffset;
	btVector3DoubleData m_jointAxisTop[6];
	btVector3DoubleData m_jointAxisBottom[6];

	btVector3DoubleData m_linkInertia;  
	btVector3DoubleData m_absFrameTotVelocityTop;
	btVector3DoubleData m_absFrameTotVelocityBottom;
	btVector3DoubleData m_absFrameLocVelocityTop;
	btVector3DoubleData m_absFrameLocVelocityBottom;

	double m_linkMass;
	int m_parentIndex;
	int m_jointType;

	int m_dofCount;
	int m_posVarCount;
	double m_jointPos[7];
	double m_jointVel[6];
	double m_jointTorque[6];

	double m_jointDamping;
	double m_jointFriction;
	double m_jointLowerLimit;
	double m_jointUpperLimit;
	double m_jointMaxForce;
	double m_jointMaxVelocity;

	char *m_linkName;
	char *m_jointName;
	btCollisionObjectDoubleData *m_linkCollider;
	char *m_paddingPtr;
};

struct btMultiBodyLinkFloatData
{
	btQuaternionFloatData m_zeroRotParentToThis;
	btVector3FloatData m_parentComToThisPivotOffset;
	btVector3FloatData m_thisPivotToThisComOffset;
	btVector3FloatData m_jointAxisTop[6];
	btVector3FloatData m_jointAxisBottom[6];
	btVector3FloatData m_linkInertia;  
	btVector3FloatData m_absFrameTotVelocityTop;
	btVector3FloatData m_absFrameTotVelocityBottom;
	btVector3FloatData m_absFrameLocVelocityTop;
	btVector3FloatData m_absFrameLocVelocityBottom;

	int m_dofCount;
	float m_linkMass;
	int m_parentIndex;
	int m_jointType;

	float m_jointPos[7];
	float m_jointVel[6];
	float m_jointTorque[6];
	int m_posVarCount;
	float m_jointDamping;
	float m_jointFriction;
	float m_jointLowerLimit;
	float m_jointUpperLimit;
	float m_jointMaxForce;
	float m_jointMaxVelocity;

	char *m_linkName;
	char *m_jointName;
	btCollisionObjectFloatData *m_linkCollider;
	char *m_paddingPtr;
};


struct btMultiBodyDoubleData
{
	btVector3DoubleData m_baseWorldPosition;
	btQuaternionDoubleData m_baseWorldOrientation;
	btVector3DoubleData m_baseLinearVelocity;
	btVector3DoubleData m_baseAngularVelocity;
	btVector3DoubleData m_baseInertia;  
	double m_baseMass;
	int m_numLinks;
	char m_padding[4];

	char *m_baseName;
	btMultiBodyLinkDoubleData *m_links;
	btCollisionObjectDoubleData *m_baseCollider;
};


struct btMultiBodyFloatData
{
	btVector3FloatData m_baseWorldPosition;
	btQuaternionFloatData m_baseWorldOrientation;
	btVector3FloatData m_baseLinearVelocity;
	btVector3FloatData m_baseAngularVelocity;

	btVector3FloatData m_baseInertia;  
	float m_baseMass;
	int m_numLinks;

	char *m_baseName;
	btMultiBodyLinkFloatData *m_links;
	btCollisionObjectFloatData *m_baseCollider;
};

#endif
