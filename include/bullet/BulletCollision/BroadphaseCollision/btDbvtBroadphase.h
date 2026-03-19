


#ifndef BT_DBVT_BROADPHASE_H
#define BT_DBVT_BROADPHASE_H

#include "BulletCollision/BroadphaseCollision/btDbvt.h"
#include "BulletCollision/BroadphaseCollision/btOverlappingPairCache.h"





#define DBVT_BP_PROFILE 0

#define DBVT_BP_PREVENTFALSEUPDATE 0
#define DBVT_BP_ACCURATESLEEPING 0
#define DBVT_BP_ENABLE_BENCHMARK 0

extern btScalar gDbvtMargin;

#if DBVT_BP_PROFILE
#define DBVT_BP_PROFILING_RATE 256
#include "LinearMath/btQuickprof.h"
#endif




struct btDbvtProxy : btBroadphaseProxy
{
	
	
	btDbvtNode* leaf;
	btDbvtProxy* links[2];
	int stage;
	
	btDbvtProxy(const btVector3& aabbMin, const btVector3& aabbMax, void* userPtr, int collisionFilterGroup, int collisionFilterMask) : btBroadphaseProxy(aabbMin, aabbMax, userPtr, collisionFilterGroup, collisionFilterMask)
	{
		links[0] = links[1] = 0;
	}
};

typedef btAlignedObjectArray<btDbvtProxy*> btDbvtProxyArray;




struct btDbvtBroadphase : btBroadphaseInterface
{
	
	enum
	{
		DYNAMIC_SET = 0, 
		FIXED_SET = 1,   
		STAGECOUNT = 2   
	};
	
	btDbvt m_sets[2];                           
	btDbvtProxy* m_stageRoots[STAGECOUNT + 1];  
	btOverlappingPairCache* m_paircache;        
	btScalar m_prediction;                      
	int m_stageCurrent;                         
	int m_fupdates;                             
	int m_dupdates;                             
	int m_cupdates;                             
	int m_newpairs;                             
	int m_fixedleft;                            
	unsigned m_updates_call;                    
	unsigned m_updates_done;                    
	btScalar m_updates_ratio;                   
	int m_pid;                                  
	int m_cid;                                  
	int m_gid;                                  
	bool m_releasepaircache;                    
	bool m_deferedcollide;                      
	bool m_needcleanup;                         
	btAlignedObjectArray<btAlignedObjectArray<const btDbvtNode*> > m_rayTestStacks;
#if DBVT_BP_PROFILE
	btClock m_clock;
	struct
	{
		unsigned long m_total;
		unsigned long m_ddcollide;
		unsigned long m_fdcollide;
		unsigned long m_cleanup;
		unsigned long m_jobcount;
	} m_profiling;
#endif
	
	btDbvtBroadphase(btOverlappingPairCache* paircache = 0);
	~btDbvtBroadphase();
	void collide(btDispatcher* dispatcher);
	void optimize();

	
	btBroadphaseProxy* createProxy(const btVector3& aabbMin, const btVector3& aabbMax, int shapeType, void* userPtr, int collisionFilterGroup, int collisionFilterMask, btDispatcher* dispatcher);
	virtual void destroyProxy(btBroadphaseProxy* proxy, btDispatcher* dispatcher);
	virtual void setAabb(btBroadphaseProxy* proxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* dispatcher);
	virtual void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin = btVector3(0, 0, 0), const btVector3& aabbMax = btVector3(0, 0, 0));
	virtual void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback);

	virtual void getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const;
	virtual void calculateOverlappingPairs(btDispatcher* dispatcher);
	virtual btOverlappingPairCache* getOverlappingPairCache();
	virtual const btOverlappingPairCache* getOverlappingPairCache() const;
	virtual void getBroadphaseAabb(btVector3& aabbMin, btVector3& aabbMax) const;
	virtual void printStats();

	
	virtual void resetPool(btDispatcher* dispatcher);

	void performDeferredRemoval(btDispatcher* dispatcher);

	void setVelocityPrediction(btScalar prediction)
	{
		m_prediction = prediction;
	}
	btScalar getVelocityPrediction() const
	{
		return m_prediction;
	}

	
	
	
	
	void setAabbForceUpdate(btBroadphaseProxy* absproxy, const btVector3& aabbMin, const btVector3& aabbMax, btDispatcher* );

	static void benchmark(btBroadphaseInterface*);
};

#endif
