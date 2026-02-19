

#ifndef BT_COLLISION_WORLD_IMPORTER_H
#define BT_COLLISION_WORLD_IMPORTER_H

#include "LinearMath/btTransform.h"
#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "LinearMath/btHashMap.h"

class btCollisionShape;
class btCollisionObject;
struct btBulletSerializedArrays;

struct ConstraintInput;
class btCollisionWorld;
struct btCollisionShapeData;
class btTriangleIndexVertexArray;
class btStridingMeshInterface;
struct btStridingMeshInterfaceData;
class btGImpactMeshShape;
class btOptimizedBvh;
struct btTriangleInfoMap;
class btBvhTriangleMeshShape;
class btPoint2PointConstraint;
class btHingeConstraint;
class btConeTwistConstraint;
class btGeneric6DofConstraint;
class btGeneric6DofSpringConstraint;
class btSliderConstraint;
class btGearConstraint;
struct btContactSolverInfo;

class btCollisionWorldImporter
{
protected:
	btCollisionWorld* m_collisionWorld;

	int m_verboseMode;

	btAlignedObjectArray<btCollisionShape*> m_allocatedCollisionShapes;
	btAlignedObjectArray<btCollisionObject*> m_allocatedRigidBodies;

	btAlignedObjectArray<btOptimizedBvh*> m_allocatedBvhs;
	btAlignedObjectArray<btTriangleInfoMap*> m_allocatedTriangleInfoMaps;
	btAlignedObjectArray<btTriangleIndexVertexArray*> m_allocatedTriangleIndexArrays;
	btAlignedObjectArray<btStridingMeshInterfaceData*> m_allocatedbtStridingMeshInterfaceDatas;
	btAlignedObjectArray<btCollisionObject*> m_allocatedCollisionObjects;

	btAlignedObjectArray<char*> m_allocatedNames;

	btAlignedObjectArray<int*> m_indexArrays;
	btAlignedObjectArray<short int*> m_shortIndexArrays;
	btAlignedObjectArray<unsigned char*> m_charIndexArrays;

	btAlignedObjectArray<btVector3FloatData*> m_floatVertexArrays;
	btAlignedObjectArray<btVector3DoubleData*> m_doubleVertexArrays;

	btHashMap<btHashPtr, btOptimizedBvh*> m_bvhMap;
	btHashMap<btHashPtr, btTriangleInfoMap*> m_timMap;

	btHashMap<btHashString, btCollisionShape*> m_nameShapeMap;
	btHashMap<btHashString, btCollisionObject*> m_nameColObjMap;

	btHashMap<btHashPtr, const char*> m_objectNameMap;

	btHashMap<btHashPtr, btCollisionShape*> m_shapeMap;
	btHashMap<btHashPtr, btCollisionObject*> m_bodyMap;

	

	char* duplicateName(const char* name);

	btCollisionShape* convertCollisionShape(btCollisionShapeData* shapeData);

public:
	btCollisionWorldImporter(btCollisionWorld* world);

	virtual ~btCollisionWorldImporter();

	bool convertAllObjects(btBulletSerializedArrays* arrays);

	
	
	virtual void deleteAllData();

	void setVerboseMode(int verboseMode)
	{
		m_verboseMode = verboseMode;
	}

	int getVerboseMode() const
	{
		return m_verboseMode;
	}

	
	int getNumCollisionShapes() const;
	btCollisionShape* getCollisionShapeByIndex(int index);
	int getNumRigidBodies() const;
	btCollisionObject* getRigidBodyByIndex(int index) const;

	int getNumBvhs() const;
	btOptimizedBvh* getBvhByIndex(int index) const;
	int getNumTriangleInfoMaps() const;
	btTriangleInfoMap* getTriangleInfoMapByIndex(int index) const;

	
	btCollisionShape* getCollisionShapeByName(const char* name);
	btCollisionObject* getCollisionObjectByName(const char* name);

	const char* getNameForPointer(const void* ptr) const;

	

	

	virtual btCollisionObject* createCollisionObject(const btTransform& startTransform, btCollisionShape* shape, const char* bodyName);

	

	virtual btCollisionShape* createPlaneShape(const btVector3& planeNormal, btScalar planeConstant);
	virtual btCollisionShape* createBoxShape(const btVector3& halfExtents);
	virtual btCollisionShape* createSphereShape(btScalar radius);
	virtual btCollisionShape* createCapsuleShapeX(btScalar radius, btScalar height);
	virtual btCollisionShape* createCapsuleShapeY(btScalar radius, btScalar height);
	virtual btCollisionShape* createCapsuleShapeZ(btScalar radius, btScalar height);

	virtual btCollisionShape* createCylinderShapeX(btScalar radius, btScalar height);
	virtual btCollisionShape* createCylinderShapeY(btScalar radius, btScalar height);
	virtual btCollisionShape* createCylinderShapeZ(btScalar radius, btScalar height);
	virtual btCollisionShape* createConeShapeX(btScalar radius, btScalar height);
	virtual btCollisionShape* createConeShapeY(btScalar radius, btScalar height);
	virtual btCollisionShape* createConeShapeZ(btScalar radius, btScalar height);
	virtual class btTriangleIndexVertexArray* createTriangleMeshContainer();
	virtual btBvhTriangleMeshShape* createBvhTriangleMeshShape(btStridingMeshInterface* trimesh, btOptimizedBvh* bvh);
	virtual btCollisionShape* createConvexTriangleMeshShape(btStridingMeshInterface* trimesh);
#ifdef SUPPORT_GIMPACT_SHAPE_IMPORT
	virtual btGImpactMeshShape* createGimpactShape(btStridingMeshInterface* trimesh);
#endif  
	virtual btStridingMeshInterfaceData* createStridingMeshInterfaceData(btStridingMeshInterfaceData* interfaceData);

	virtual class btConvexHullShape* createConvexHullShape();
	virtual class btCompoundShape* createCompoundShape();
	virtual class btScaledBvhTriangleMeshShape* createScaledTrangleMeshShape(btBvhTriangleMeshShape* meshShape, const btVector3& localScalingbtBvhTriangleMeshShape);

	virtual class btMultiSphereShape* createMultiSphereShape(const btVector3* positions, const btScalar* radi, int numSpheres);

	virtual btTriangleIndexVertexArray* createMeshInterface(btStridingMeshInterfaceData& meshData);

	
	virtual btOptimizedBvh* createOptimizedBvh();
	virtual btTriangleInfoMap* createTriangleInfoMap();
};

#endif  
