

#ifndef BT_SIMPLEX_SOLVER_INTERFACE_H
#define BT_SIMPLEX_SOLVER_INTERFACE_H

#include "LinearMath/btVector3.h"

#define NO_VIRTUAL_INTERFACE 1
#ifdef NO_VIRTUAL_INTERFACE
#include "btVoronoiSimplexSolver.h"
#define btSimplexSolverInterface btVoronoiSimplexSolver
#else




class btSimplexSolverInterface
{
public:
	virtual ~btSimplexSolverInterface(){};

	virtual void reset() = 0;

	virtual void addVertex(const btVector3& w, const btVector3& p, const btVector3& q) = 0;

	virtual bool closest(btVector3& v) = 0;

	virtual btScalar maxVertex() = 0;

	virtual bool fullSimplex() const = 0;

	virtual int getSimplex(btVector3* pBuf, btVector3* qBuf, btVector3* yBuf) const = 0;

	virtual bool inSimplex(const btVector3& w) = 0;

	virtual void backup_closest(btVector3& v) = 0;

	virtual bool emptySimplex() const = 0;

	virtual void compute_points(btVector3& p1, btVector3& p2) = 0;

	virtual int numVertices() const = 0;
};
#endif
#endif  
