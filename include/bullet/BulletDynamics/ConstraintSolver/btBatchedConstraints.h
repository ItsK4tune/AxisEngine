

#ifndef BT_BATCHED_CONSTRAINTS_H
#define BT_BATCHED_CONSTRAINTS_H

#include "LinearMath/btThreads.h"
#include "LinearMath/btAlignedObjectArray.h"
#include "BulletDynamics/ConstraintSolver/btSolverBody.h"
#include "BulletDynamics/ConstraintSolver/btSolverConstraint.h"

class btIDebugDraw;

struct btBatchedConstraints
{
	enum BatchingMethod
	{
		BATCHING_METHOD_SPATIAL_GRID_2D,
		BATCHING_METHOD_SPATIAL_GRID_3D,
		BATCHING_METHOD_COUNT
	};
	struct Range
	{
		int begin;
		int end;

		Range() : begin(0), end(0) {}
		Range(int _beg, int _end) : begin(_beg), end(_end) {}
	};

	btAlignedObjectArray<int> m_constraintIndices;
	btAlignedObjectArray<Range> m_batches;        
	btAlignedObjectArray<Range> m_phases;         
	btAlignedObjectArray<char> m_phaseGrainSize;  
	btAlignedObjectArray<int> m_phaseOrder;       
	btIDebugDraw* m_debugDrawer;

	static bool s_debugDrawBatches;

	btBatchedConstraints() { m_debugDrawer = NULL; }
	void setup(btConstraintArray* constraints,
			   const btAlignedObjectArray<btSolverBody>& bodies,
			   BatchingMethod batchingMethod,
			   int minBatchSize,
			   int maxBatchSize,
			   btAlignedObjectArray<char>* scratchMemory);
	bool validate(btConstraintArray* constraints, const btAlignedObjectArray<btSolverBody>& bodies) const;
};

#endif  
