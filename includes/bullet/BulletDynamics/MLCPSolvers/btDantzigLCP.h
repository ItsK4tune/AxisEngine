



#ifndef _BT_LCP_H_
#define _BT_LCP_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "LinearMath/btScalar.h"
#include "LinearMath/btAlignedObjectArray.h"

struct btDantzigScratchMemory
{
	btAlignedObjectArray<btScalar> m_scratch;
	btAlignedObjectArray<btScalar> L;
	btAlignedObjectArray<btScalar> d;
	btAlignedObjectArray<btScalar> delta_w;
	btAlignedObjectArray<btScalar> delta_x;
	btAlignedObjectArray<btScalar> Dell;
	btAlignedObjectArray<btScalar> ell;
	btAlignedObjectArray<btScalar *> Arows;
	btAlignedObjectArray<int> p;
	btAlignedObjectArray<int> C;
	btAlignedObjectArray<bool> state;
};


bool btSolveDantzigLCP(int n, btScalar *A, btScalar *x, btScalar *b, btScalar *w,
					   int nub, btScalar *lo, btScalar *hi, int *findex, btDantzigScratchMemory &scratch);

#endif  
