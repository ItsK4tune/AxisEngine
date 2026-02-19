


#ifndef BT_GJK_EPA2_H
#define BT_GJK_EPA2_H

#include "BulletCollision/CollisionShapes/btConvexShape.h"


struct btGjkEpaSolver2
{
	struct sResults
	{
		enum eStatus
		{
			Separated,   
			Penetrating, 
			GJK_Failed,  
			EPA_Failed   
		} status;
		btVector3 witnesses[2];
		btVector3 normal;
		btScalar distance;
	};

	static int StackSizeRequirement();

	static bool Distance(const btConvexShape* shape0, const btTransform& wtrs0,
						 const btConvexShape* shape1, const btTransform& wtrs1,
						 const btVector3& guess,
						 sResults& results);

	static bool Penetration(const btConvexShape* shape0, const btTransform& wtrs0,
							const btConvexShape* shape1, const btTransform& wtrs1,
							const btVector3& guess,
							sResults& results,
							bool usemargins = true);
#ifndef __SPU__
	static btScalar SignedDistance(const btVector3& position,
								   btScalar margin,
								   const btConvexShape* shape,
								   const btTransform& wtrs,
								   sResults& results);

	static bool SignedDistance(const btConvexShape* shape0, const btTransform& wtrs0,
							   const btConvexShape* shape1, const btTransform& wtrs1,
							   const btVector3& guess,
							   sResults& results);
#endif  
};

#endif  
