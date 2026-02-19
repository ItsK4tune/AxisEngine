

#ifndef BT_CONVEX_HULL_COMPUTER_H
#define BT_CONVEX_HULL_COMPUTER_H

#include "btVector3.h"
#include "btAlignedObjectArray.h"




class btConvexHullComputer
{
private:
	btScalar compute(const void* coords, bool doubleCoords, int stride, int count, btScalar shrink, btScalar shrinkClamp);

public:
	class Edge
	{
	private:
		int next;
		int reverse;
		int targetVertex;

		friend class btConvexHullComputer;

	public:
		int getSourceVertex() const
		{
			return (this + reverse)->targetVertex;
		}

		int getTargetVertex() const
		{
			return targetVertex;
		}

		const Edge* getNextEdgeOfVertex() const  
		{
			return this + next;
		}

		const Edge* getNextEdgeOfFace() const  
		{
			return (this + reverse)->getNextEdgeOfVertex();
		}

		const Edge* getReverseEdge() const
		{
			return this + reverse;
		}
	};

	
	btAlignedObjectArray<btVector3> vertices;

	
	btAlignedObjectArray<int> original_vertex_index;

	
	btAlignedObjectArray<Edge> edges;

	
	btAlignedObjectArray<int> faces;

	
	btScalar compute(const float* coords, int stride, int count, btScalar shrink, btScalar shrinkClamp)
	{
		return compute(coords, false, stride, count, shrink, shrinkClamp);
	}

	
	btScalar compute(const double* coords, int stride, int count, btScalar shrink, btScalar shrinkClamp)
	{
		return compute(coords, true, stride, count, shrink, shrinkClamp);
	}
};

#endif  
