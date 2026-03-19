

#ifndef BT_COLLISION_OBJECT_H
#define BT_COLLISION_OBJECT_H

#include "LinearMath/btTransform.h"


#define ACTIVE_TAG 1
#define ISLAND_SLEEPING 2
#define WANTS_DEACTIVATION 3
#define DISABLE_DEACTIVATION 4
#define DISABLE_SIMULATION 5
#define FIXED_BASE_MULTI_BODY 6

struct btBroadphaseProxy;
class btCollisionShape;
struct btCollisionShapeData;
#include "LinearMath/btMotionState.h"
#include "LinearMath/btAlignedAllocator.h"
#include "LinearMath/btAlignedObjectArray.h"

typedef btAlignedObjectArray<class btCollisionObject*> btCollisionObjectArray;

#ifdef BT_USE_DOUBLE_PRECISION
#define btCollisionObjectData btCollisionObjectDoubleData
#define btCollisionObjectDataName "btCollisionObjectDoubleData"
#else
#define btCollisionObjectData btCollisionObjectFloatData
#define btCollisionObjectDataName "btCollisionObjectFloatData"
#endif




ATTRIBUTE_ALIGNED16(class)
btCollisionObject
{
protected:
	btTransform m_worldTransform;

	
	
	btTransform m_interpolationWorldTransform;
	
	
	btVector3 m_interpolationLinearVelocity;
	btVector3 m_interpolationAngularVelocity;

	btVector3 m_anisotropicFriction;
	int m_hasAnisotropicFriction;
	btScalar m_contactProcessingThreshold;

	btBroadphaseProxy* m_broadphaseHandle;
	btCollisionShape* m_collisionShape;
	
	void* m_extensionPointer;

	
	
	
	btCollisionShape* m_rootCollisionShape;

	int m_collisionFlags;

	int m_islandTag1;
	int m_companionId;
	int m_worldArrayIndex;  

	mutable int m_activationState1;
	mutable btScalar m_deactivationTime;

	btScalar m_friction;
	btScalar m_restitution;
	btScalar m_rollingFriction;   
	btScalar m_spinningFriction;  
	btScalar m_contactDamping;
	btScalar m_contactStiffness;

	
	
	int m_internalType;

	

	void* m_userObjectPointer;

	int m_userIndex2;

	int m_userIndex;

	int m_userIndex3;

	
	btScalar m_hitFraction;

	
	btScalar m_ccdSweptSphereRadius;

	
	btScalar m_ccdMotionThreshold;

	
	int m_checkCollideWith;

	btAlignedObjectArray<const btCollisionObject*> m_objectsWithoutCollisionCheck;

	
	int m_updateRevision;

	btVector3 m_customDebugColorRGB;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	enum CollisionFlags
	{
		CF_DYNAMIC_OBJECT = 0,
		CF_STATIC_OBJECT = 1,
		CF_KINEMATIC_OBJECT = 2,
		CF_NO_CONTACT_RESPONSE = 4,
		CF_CUSTOM_MATERIAL_CALLBACK = 8,  
		CF_CHARACTER_OBJECT = 16,
		CF_DISABLE_VISUALIZE_OBJECT = 32,          
		CF_DISABLE_SPU_COLLISION_PROCESSING = 64,  
		CF_HAS_CONTACT_STIFFNESS_DAMPING = 128,
		CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR = 256,
		CF_HAS_FRICTION_ANCHOR = 512,
		CF_HAS_COLLISION_SOUND_TRIGGER = 1024
	};

	enum CollisionObjectTypes
	{
		CO_COLLISION_OBJECT = 1,
		CO_RIGID_BODY = 2,
		
		
		CO_GHOST_OBJECT = 4,
		CO_SOFT_BODY = 8,
		CO_HF_FLUID = 16,
		CO_USER_TYPE = 32,
		CO_FEATHERSTONE_LINK = 64
	};

	enum AnisotropicFrictionFlags
	{
		CF_ANISOTROPIC_FRICTION_DISABLED = 0,
		CF_ANISOTROPIC_FRICTION = 1,
		CF_ANISOTROPIC_ROLLING_FRICTION = 2
	};

	SIMD_FORCE_INLINE bool mergesSimulationIslands() const
	{
		
		return ((m_collisionFlags & (CF_STATIC_OBJECT | CF_KINEMATIC_OBJECT | CF_NO_CONTACT_RESPONSE)) == 0);
	}

	const btVector3& getAnisotropicFriction() const
	{
		return m_anisotropicFriction;
	}
	void setAnisotropicFriction(const btVector3& anisotropicFriction, int frictionMode = CF_ANISOTROPIC_FRICTION)
	{
		m_anisotropicFriction = anisotropicFriction;
		bool isUnity = (anisotropicFriction[0] != 1.f) || (anisotropicFriction[1] != 1.f) || (anisotropicFriction[2] != 1.f);
		m_hasAnisotropicFriction = isUnity ? frictionMode : 0;
	}
	bool hasAnisotropicFriction(int frictionMode = CF_ANISOTROPIC_FRICTION) const
	{
		return (m_hasAnisotropicFriction & frictionMode) != 0;
	}

	
	
	void setContactProcessingThreshold(btScalar contactProcessingThreshold)
	{
		m_contactProcessingThreshold = contactProcessingThreshold;
	}
	btScalar getContactProcessingThreshold() const
	{
		return m_contactProcessingThreshold;
	}

	SIMD_FORCE_INLINE bool isStaticObject() const
	{
		return (m_collisionFlags & CF_STATIC_OBJECT) != 0;
	}

	SIMD_FORCE_INLINE bool isKinematicObject() const
	{
		return (m_collisionFlags & CF_KINEMATIC_OBJECT) != 0;
	}

	SIMD_FORCE_INLINE bool isStaticOrKinematicObject() const
	{
		return (m_collisionFlags & (CF_KINEMATIC_OBJECT | CF_STATIC_OBJECT)) != 0;
	}

	SIMD_FORCE_INLINE bool hasContactResponse() const
	{
		return (m_collisionFlags & CF_NO_CONTACT_RESPONSE) == 0;
	}

	btCollisionObject();

	virtual ~btCollisionObject();

	virtual void setCollisionShape(btCollisionShape * collisionShape)
	{
		m_updateRevision++;
		m_collisionShape = collisionShape;
		m_rootCollisionShape = collisionShape;
	}

	SIMD_FORCE_INLINE const btCollisionShape* getCollisionShape() const
	{
		return m_collisionShape;
	}

	SIMD_FORCE_INLINE btCollisionShape* getCollisionShape()
	{
		return m_collisionShape;
	}

	void setIgnoreCollisionCheck(const btCollisionObject* co, bool ignoreCollisionCheck)
	{
		if (ignoreCollisionCheck)
		{
			
			
			
			
			m_objectsWithoutCollisionCheck.push_back(co);
			
		}
		else
		{
			m_objectsWithoutCollisionCheck.remove(co);
		}
		m_checkCollideWith = m_objectsWithoutCollisionCheck.size() > 0;
	}

        int getNumObjectsWithoutCollision() const
	{
		return m_objectsWithoutCollisionCheck.size();
	}

	const btCollisionObject* getObjectWithoutCollision(int index)
	{
		return m_objectsWithoutCollisionCheck[index];
	}

	virtual bool checkCollideWithOverride(const btCollisionObject* co) const
	{
		int index = m_objectsWithoutCollisionCheck.findLinearSearch(co);
		if (index < m_objectsWithoutCollisionCheck.size())
		{
			return false;
		}
		return true;
	}

	
	
	void* internalGetExtensionPointer() const
	{
		return m_extensionPointer;
	}
	
	
	void internalSetExtensionPointer(void* pointer)
	{
		m_extensionPointer = pointer;
	}

	SIMD_FORCE_INLINE int getActivationState() const { return m_activationState1; }

	void setActivationState(int newState) const;

	void setDeactivationTime(btScalar time)
	{
		m_deactivationTime = time;
	}
	btScalar getDeactivationTime() const
	{
		return m_deactivationTime;
	}

	void forceActivationState(int newState) const;

	void activate(bool forceActivation = false) const;

	SIMD_FORCE_INLINE bool isActive() const
	{
		return ((getActivationState() != FIXED_BASE_MULTI_BODY) && (getActivationState() != ISLAND_SLEEPING) && (getActivationState() != DISABLE_SIMULATION));
	}

	void setRestitution(btScalar rest)
	{
		m_updateRevision++;
		m_restitution = rest;
	}
	btScalar getRestitution() const
	{
		return m_restitution;
	}
	void setFriction(btScalar frict)
	{
		m_updateRevision++;
		m_friction = frict;
	}
	btScalar getFriction() const
	{
		return m_friction;
	}

	void setRollingFriction(btScalar frict)
	{
		m_updateRevision++;
		m_rollingFriction = frict;
	}
	btScalar getRollingFriction() const
	{
		return m_rollingFriction;
	}
	void setSpinningFriction(btScalar frict)
	{
		m_updateRevision++;
		m_spinningFriction = frict;
	}
	btScalar getSpinningFriction() const
	{
		return m_spinningFriction;
	}
	void setContactStiffnessAndDamping(btScalar stiffness, btScalar damping)
	{
		m_updateRevision++;
		m_contactStiffness = stiffness;
		m_contactDamping = damping;

		m_collisionFlags |= CF_HAS_CONTACT_STIFFNESS_DAMPING;

		
		if (m_contactStiffness < SIMD_EPSILON)
		{
			m_contactStiffness = SIMD_EPSILON;
		}
	}

	btScalar getContactStiffness() const
	{
		return m_contactStiffness;
	}

	btScalar getContactDamping() const
	{
		return m_contactDamping;
	}

	
	int getInternalType() const
	{
		return m_internalType;
	}

	btTransform& getWorldTransform()
	{
		return m_worldTransform;
	}

	const btTransform& getWorldTransform() const
	{
		return m_worldTransform;
	}

	void setWorldTransform(const btTransform& worldTrans)
	{
		m_updateRevision++;
		m_worldTransform = worldTrans;
	}

	SIMD_FORCE_INLINE btBroadphaseProxy* getBroadphaseHandle()
	{
		return m_broadphaseHandle;
	}

	SIMD_FORCE_INLINE const btBroadphaseProxy* getBroadphaseHandle() const
	{
		return m_broadphaseHandle;
	}

	void setBroadphaseHandle(btBroadphaseProxy * handle)
	{
		m_broadphaseHandle = handle;
	}

	const btTransform& getInterpolationWorldTransform() const
	{
		return m_interpolationWorldTransform;
	}

	btTransform& getInterpolationWorldTransform()
	{
		return m_interpolationWorldTransform;
	}

	void setInterpolationWorldTransform(const btTransform& trans)
	{
		m_updateRevision++;
		m_interpolationWorldTransform = trans;
	}

	void setInterpolationLinearVelocity(const btVector3& linvel)
	{
		m_updateRevision++;
		m_interpolationLinearVelocity = linvel;
	}

	void setInterpolationAngularVelocity(const btVector3& angvel)
	{
		m_updateRevision++;
		m_interpolationAngularVelocity = angvel;
	}

	const btVector3& getInterpolationLinearVelocity() const
	{
		return m_interpolationLinearVelocity;
	}

	const btVector3& getInterpolationAngularVelocity() const
	{
		return m_interpolationAngularVelocity;
	}

	SIMD_FORCE_INLINE int getIslandTag() const
	{
		return m_islandTag1;
	}

	void setIslandTag(int tag)
	{
		m_islandTag1 = tag;
	}

	SIMD_FORCE_INLINE int getCompanionId() const
	{
		return m_companionId;
	}

	void setCompanionId(int id)
	{
		m_companionId = id;
	}

	SIMD_FORCE_INLINE int getWorldArrayIndex() const
	{
		return m_worldArrayIndex;
	}

	
	void setWorldArrayIndex(int ix)
	{
		m_worldArrayIndex = ix;
	}

	SIMD_FORCE_INLINE btScalar getHitFraction() const
	{
		return m_hitFraction;
	}

	void setHitFraction(btScalar hitFraction)
	{
		m_hitFraction = hitFraction;
	}

	SIMD_FORCE_INLINE int getCollisionFlags() const
	{
		return m_collisionFlags;
	}

	void setCollisionFlags(int flags)
	{
		m_collisionFlags = flags;
	}

	
	btScalar getCcdSweptSphereRadius() const
	{
		return m_ccdSweptSphereRadius;
	}

	
	void setCcdSweptSphereRadius(btScalar radius)
	{
		m_ccdSweptSphereRadius = radius;
	}

	btScalar getCcdMotionThreshold() const
	{
		return m_ccdMotionThreshold;
	}

	btScalar getCcdSquareMotionThreshold() const
	{
		return m_ccdMotionThreshold * m_ccdMotionThreshold;
	}

	
	void setCcdMotionThreshold(btScalar ccdMotionThreshold)
	{
		m_ccdMotionThreshold = ccdMotionThreshold;
	}

	
	void* getUserPointer() const
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

	int getUserIndex3() const
	{
		return m_userIndex3;
	}

	
	void setUserPointer(void* userPointer)
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

	void setUserIndex3(int index)
	{
		m_userIndex3 = index;
	}

	int getUpdateRevisionInternal() const
	{
		return m_updateRevision;
	}

	void setCustomDebugColor(const btVector3& colorRGB)
	{
		m_customDebugColorRGB = colorRGB;
		m_collisionFlags |= CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR;
	}

	void removeCustomDebugColor()
	{
		m_collisionFlags &= ~CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR;
	}

	bool getCustomDebugColor(btVector3 & colorRGB) const
	{
		bool hasCustomColor = (0 != (m_collisionFlags & CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR));
		if (hasCustomColor)
		{
			colorRGB = m_customDebugColorRGB;
		}
		return hasCustomColor;
	}

	inline bool checkCollideWith(const btCollisionObject* co) const
	{
		if (m_checkCollideWith)
			return checkCollideWithOverride(co);

		return true;
	}

	virtual int calculateSerializeBufferSize() const;

	
	virtual const char* serialize(void* dataBuffer, class btSerializer* serializer) const;

	virtual void serializeSingleObject(class btSerializer * serializer) const;
};




struct	btCollisionObjectDoubleData
{
	void					*m_broadphaseHandle;
	void					*m_collisionShape;
	btCollisionShapeData	*m_rootCollisionShape;
	char					*m_name;

	btTransformDoubleData	m_worldTransform;
	btTransformDoubleData	m_interpolationWorldTransform;
	btVector3DoubleData		m_interpolationLinearVelocity;
	btVector3DoubleData		m_interpolationAngularVelocity;
	btVector3DoubleData		m_anisotropicFriction;
	double					m_contactProcessingThreshold;	
	double					m_deactivationTime;
	double					m_friction;
	double					m_rollingFriction;
	double                  m_contactDamping;
	double                  m_contactStiffness;
	double					m_restitution;
	double					m_hitFraction; 
	double					m_ccdSweptSphereRadius;
	double					m_ccdMotionThreshold;
	int						m_hasAnisotropicFriction;
	int						m_collisionFlags;
	int						m_islandTag1;
	int						m_companionId;
	int						m_activationState1;
	int						m_internalType;
	int						m_checkCollideWith;
	int						m_collisionFilterGroup;
	int						m_collisionFilterMask;
	int						m_uniqueId;
};


struct	btCollisionObjectFloatData
{
	void					*m_broadphaseHandle;
	void					*m_collisionShape;
	btCollisionShapeData	*m_rootCollisionShape;
	char					*m_name;

	btTransformFloatData	m_worldTransform;
	btTransformFloatData	m_interpolationWorldTransform;
	btVector3FloatData		m_interpolationLinearVelocity;
	btVector3FloatData		m_interpolationAngularVelocity;
	btVector3FloatData		m_anisotropicFriction;
	float					m_contactProcessingThreshold;	
	float					m_deactivationTime;
	float					m_friction;
	float					m_rollingFriction;
	float                   m_contactDamping;
    float                   m_contactStiffness;
	float					m_restitution;
	float					m_hitFraction; 
	float					m_ccdSweptSphereRadius;
	float					m_ccdMotionThreshold;
	int						m_hasAnisotropicFriction;
	int						m_collisionFlags;
	int						m_islandTag1;
	int						m_companionId;
	int						m_activationState1;
	int						m_internalType;
	int						m_checkCollideWith;
	int						m_collisionFilterGroup;
	int						m_collisionFilterMask;
	int						m_uniqueId;
};


SIMD_FORCE_INLINE int btCollisionObject::calculateSerializeBufferSize() const
{
	return sizeof(btCollisionObjectData);
}

#endif  
