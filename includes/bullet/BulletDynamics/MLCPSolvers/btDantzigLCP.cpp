



#include "btDantzigLCP.h"

#include <string.h>  

bool s_error = false;




#define btLCP_FAST  


#define BTROWPTRS
#define BTATYPE btScalar **
#define BTAROW(i) (m_A[i])






#define BTNUB_OPTIMIZATIONS



static void btSolveL1_1(const btScalar *L, btScalar *B, int n, int lskip1)
{
	
	btScalar Z11, m11, Z21, m21, p1, q1, p2, *ex;
	const btScalar *ell;
	int i, j;
	
	for (i = 0; i < n; i += 2)
	{
		
		
		Z11 = 0;
		Z21 = 0;
		ell = L + i * lskip1;
		ex = B;
		
		for (j = i - 2; j >= 0; j -= 2)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			m11 = p1 * q1;
			p2 = ell[lskip1];
			m21 = p2 * q1;
			Z11 += m11;
			Z21 += m21;
			
			p1 = ell[1];
			q1 = ex[1];
			m11 = p1 * q1;
			p2 = ell[1 + lskip1];
			m21 = p2 * q1;
			
			ell += 2;
			ex += 2;
			Z11 += m11;
			Z21 += m21;
			
		}
		
		j += 2;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			m11 = p1 * q1;
			p2 = ell[lskip1];
			m21 = p2 * q1;
			
			ell += 1;
			ex += 1;
			Z11 += m11;
			Z21 += m21;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
		p1 = ell[lskip1];
		Z21 = ex[1] - Z21 - p1 * Z11;
		ex[1] = Z21;
		
	}
}



static void btSolveL1_2(const btScalar *L, btScalar *B, int n, int lskip1)
{
	
	btScalar Z11, m11, Z12, m12, Z21, m21, Z22, m22, p1, q1, p2, q2, *ex;
	const btScalar *ell;
	int i, j;
	
	for (i = 0; i < n; i += 2)
	{
		
		
		Z11 = 0;
		Z12 = 0;
		Z21 = 0;
		Z22 = 0;
		ell = L + i * lskip1;
		ex = B;
		
		for (j = i - 2; j >= 0; j -= 2)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			m11 = p1 * q1;
			q2 = ex[lskip1];
			m12 = p1 * q2;
			p2 = ell[lskip1];
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z12 += m12;
			Z21 += m21;
			Z22 += m22;
			
			p1 = ell[1];
			q1 = ex[1];
			m11 = p1 * q1;
			q2 = ex[1 + lskip1];
			m12 = p1 * q2;
			p2 = ell[1 + lskip1];
			m21 = p2 * q1;
			m22 = p2 * q2;
			
			ell += 2;
			ex += 2;
			Z11 += m11;
			Z12 += m12;
			Z21 += m21;
			Z22 += m22;
			
		}
		
		j += 2;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			m11 = p1 * q1;
			q2 = ex[lskip1];
			m12 = p1 * q2;
			p2 = ell[lskip1];
			m21 = p2 * q1;
			m22 = p2 * q2;
			
			ell += 1;
			ex += 1;
			Z11 += m11;
			Z12 += m12;
			Z21 += m21;
			Z22 += m22;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
		Z12 = ex[lskip1] - Z12;
		ex[lskip1] = Z12;
		p1 = ell[lskip1];
		Z21 = ex[1] - Z21 - p1 * Z11;
		ex[1] = Z21;
		Z22 = ex[1 + lskip1] - Z22 - p1 * Z12;
		ex[1 + lskip1] = Z22;
		
	}
}

void btFactorLDLT(btScalar *A, btScalar *d, int n, int nskip1)
{
	int i, j;
	btScalar sum, *ell, *dee, dd, p1, p2, q1, q2, Z11, m11, Z21, m21, Z22, m22;
	if (n < 1) return;

	for (i = 0; i <= n - 2; i += 2)
	{
		
		btSolveL1_2(A, A + i * nskip1, i, nskip1);
		
		
		Z11 = 0;
		Z21 = 0;
		Z22 = 0;
		ell = A + i * nskip1;
		dee = d;
		for (j = i - 6; j >= 0; j -= 6)
		{
			p1 = ell[0];
			p2 = ell[nskip1];
			dd = dee[0];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[0] = q1;
			ell[nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			p1 = ell[1];
			p2 = ell[1 + nskip1];
			dd = dee[1];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[1] = q1;
			ell[1 + nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			p1 = ell[2];
			p2 = ell[2 + nskip1];
			dd = dee[2];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[2] = q1;
			ell[2 + nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			p1 = ell[3];
			p2 = ell[3 + nskip1];
			dd = dee[3];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[3] = q1;
			ell[3 + nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			p1 = ell[4];
			p2 = ell[4 + nskip1];
			dd = dee[4];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[4] = q1;
			ell[4 + nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			p1 = ell[5];
			p2 = ell[5 + nskip1];
			dd = dee[5];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[5] = q1;
			ell[5 + nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			ell += 6;
			dee += 6;
		}
		
		j += 6;
		for (; j > 0; j--)
		{
			p1 = ell[0];
			p2 = ell[nskip1];
			dd = dee[0];
			q1 = p1 * dd;
			q2 = p2 * dd;
			ell[0] = q1;
			ell[nskip1] = q2;
			m11 = p1 * q1;
			m21 = p2 * q1;
			m22 = p2 * q2;
			Z11 += m11;
			Z21 += m21;
			Z22 += m22;
			ell++;
			dee++;
		}
		
		Z11 = ell[0] - Z11;
		Z21 = ell[nskip1] - Z21;
		Z22 = ell[1 + nskip1] - Z22;
		dee = d + i;
		
		
		dee[0] = btRecip(Z11);
		
		sum = 0;
		q1 = Z21;
		q2 = q1 * dee[0];
		Z21 = q2;
		sum += q1 * q2;
		dee[1] = btRecip(Z22 - sum);
		
		ell[nskip1] = Z21;
	}
	
	switch (n - i)
	{
		case 0:
			break;

		case 1:
			btSolveL1_1(A, A + i * nskip1, i, nskip1);
			
			
			Z11 = 0;
			ell = A + i * nskip1;
			dee = d;
			for (j = i - 6; j >= 0; j -= 6)
			{
				p1 = ell[0];
				dd = dee[0];
				q1 = p1 * dd;
				ell[0] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				p1 = ell[1];
				dd = dee[1];
				q1 = p1 * dd;
				ell[1] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				p1 = ell[2];
				dd = dee[2];
				q1 = p1 * dd;
				ell[2] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				p1 = ell[3];
				dd = dee[3];
				q1 = p1 * dd;
				ell[3] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				p1 = ell[4];
				dd = dee[4];
				q1 = p1 * dd;
				ell[4] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				p1 = ell[5];
				dd = dee[5];
				q1 = p1 * dd;
				ell[5] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				ell += 6;
				dee += 6;
			}
			
			j += 6;
			for (; j > 0; j--)
			{
				p1 = ell[0];
				dd = dee[0];
				q1 = p1 * dd;
				ell[0] = q1;
				m11 = p1 * q1;
				Z11 += m11;
				ell++;
				dee++;
			}
			
			Z11 = ell[0] - Z11;
			dee = d + i;
			
			
			dee[0] = btRecip(Z11);
			
			break;

			
	}
}



void btSolveL1(const btScalar *L, btScalar *B, int n, int lskip1)
{
	
	btScalar Z11, Z21, Z31, Z41, p1, q1, p2, p3, p4, *ex;
	const btScalar *ell;
	int lskip2, lskip3, i, j;
	
	lskip2 = 2 * lskip1;
	lskip3 = 3 * lskip1;
	
	for (i = 0; i <= n - 4; i += 4)
	{
		
		
		Z11 = 0;
		Z21 = 0;
		Z31 = 0;
		Z41 = 0;
		ell = L + i * lskip1;
		ex = B;
		
		for (j = i - 12; j >= 0; j -= 12)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			p2 = ell[lskip1];
			p3 = ell[lskip2];
			p4 = ell[lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[1];
			q1 = ex[1];
			p2 = ell[1 + lskip1];
			p3 = ell[1 + lskip2];
			p4 = ell[1 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[2];
			q1 = ex[2];
			p2 = ell[2 + lskip1];
			p3 = ell[2 + lskip2];
			p4 = ell[2 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[3];
			q1 = ex[3];
			p2 = ell[3 + lskip1];
			p3 = ell[3 + lskip2];
			p4 = ell[3 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[4];
			q1 = ex[4];
			p2 = ell[4 + lskip1];
			p3 = ell[4 + lskip2];
			p4 = ell[4 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[5];
			q1 = ex[5];
			p2 = ell[5 + lskip1];
			p3 = ell[5 + lskip2];
			p4 = ell[5 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[6];
			q1 = ex[6];
			p2 = ell[6 + lskip1];
			p3 = ell[6 + lskip2];
			p4 = ell[6 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[7];
			q1 = ex[7];
			p2 = ell[7 + lskip1];
			p3 = ell[7 + lskip2];
			p4 = ell[7 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[8];
			q1 = ex[8];
			p2 = ell[8 + lskip1];
			p3 = ell[8 + lskip2];
			p4 = ell[8 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[9];
			q1 = ex[9];
			p2 = ell[9 + lskip1];
			p3 = ell[9 + lskip2];
			p4 = ell[9 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[10];
			q1 = ex[10];
			p2 = ell[10 + lskip1];
			p3 = ell[10 + lskip2];
			p4 = ell[10 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			p1 = ell[11];
			q1 = ex[11];
			p2 = ell[11 + lskip1];
			p3 = ell[11 + lskip2];
			p4 = ell[11 + lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			ell += 12;
			ex += 12;
			
		}
		
		j += 12;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			p2 = ell[lskip1];
			p3 = ell[lskip2];
			p4 = ell[lskip3];
			
			Z11 += p1 * q1;
			Z21 += p2 * q1;
			Z31 += p3 * q1;
			Z41 += p4 * q1;
			
			ell += 1;
			ex += 1;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
		p1 = ell[lskip1];
		Z21 = ex[1] - Z21 - p1 * Z11;
		ex[1] = Z21;
		p1 = ell[lskip2];
		p2 = ell[1 + lskip2];
		Z31 = ex[2] - Z31 - p1 * Z11 - p2 * Z21;
		ex[2] = Z31;
		p1 = ell[lskip3];
		p2 = ell[1 + lskip3];
		p3 = ell[2 + lskip3];
		Z41 = ex[3] - Z41 - p1 * Z11 - p2 * Z21 - p3 * Z31;
		ex[3] = Z41;
		
	}
	
	for (; i < n; i++)
	{
		
		
		Z11 = 0;
		ell = L + i * lskip1;
		ex = B;
		
		for (j = i - 12; j >= 0; j -= 12)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			
			Z11 += p1 * q1;
			
			p1 = ell[1];
			q1 = ex[1];
			
			Z11 += p1 * q1;
			
			p1 = ell[2];
			q1 = ex[2];
			
			Z11 += p1 * q1;
			
			p1 = ell[3];
			q1 = ex[3];
			
			Z11 += p1 * q1;
			
			p1 = ell[4];
			q1 = ex[4];
			
			Z11 += p1 * q1;
			
			p1 = ell[5];
			q1 = ex[5];
			
			Z11 += p1 * q1;
			
			p1 = ell[6];
			q1 = ex[6];
			
			Z11 += p1 * q1;
			
			p1 = ell[7];
			q1 = ex[7];
			
			Z11 += p1 * q1;
			
			p1 = ell[8];
			q1 = ex[8];
			
			Z11 += p1 * q1;
			
			p1 = ell[9];
			q1 = ex[9];
			
			Z11 += p1 * q1;
			
			p1 = ell[10];
			q1 = ex[10];
			
			Z11 += p1 * q1;
			
			p1 = ell[11];
			q1 = ex[11];
			
			Z11 += p1 * q1;
			
			ell += 12;
			ex += 12;
			
		}
		
		j += 12;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			
			Z11 += p1 * q1;
			
			ell += 1;
			ex += 1;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
	}
}



void btSolveL1T(const btScalar *L, btScalar *B, int n, int lskip1)
{
	
	btScalar Z11, m11, Z21, m21, Z31, m31, Z41, m41, p1, q1, p2, p3, p4, *ex;
	const btScalar *ell;
	int lskip2, i, j;
	
	
	L = L + (n - 1) * (lskip1 + 1);
	B = B + n - 1;
	lskip1 = -lskip1;
	
	lskip2 = 2 * lskip1;
	
	
	for (i = 0; i <= n - 4; i += 4)
	{
		
		
		Z11 = 0;
		Z21 = 0;
		Z31 = 0;
		Z41 = 0;
		ell = L - i;
		ex = B;
		
		for (j = i - 4; j >= 0; j -= 4)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			p2 = ell[-1];
			p3 = ell[-2];
			p4 = ell[-3];
			
			m11 = p1 * q1;
			m21 = p2 * q1;
			m31 = p3 * q1;
			m41 = p4 * q1;
			ell += lskip1;
			Z11 += m11;
			Z21 += m21;
			Z31 += m31;
			Z41 += m41;
			
			p1 = ell[0];
			q1 = ex[-1];
			p2 = ell[-1];
			p3 = ell[-2];
			p4 = ell[-3];
			
			m11 = p1 * q1;
			m21 = p2 * q1;
			m31 = p3 * q1;
			m41 = p4 * q1;
			ell += lskip1;
			Z11 += m11;
			Z21 += m21;
			Z31 += m31;
			Z41 += m41;
			
			p1 = ell[0];
			q1 = ex[-2];
			p2 = ell[-1];
			p3 = ell[-2];
			p4 = ell[-3];
			
			m11 = p1 * q1;
			m21 = p2 * q1;
			m31 = p3 * q1;
			m41 = p4 * q1;
			ell += lskip1;
			Z11 += m11;
			Z21 += m21;
			Z31 += m31;
			Z41 += m41;
			
			p1 = ell[0];
			q1 = ex[-3];
			p2 = ell[-1];
			p3 = ell[-2];
			p4 = ell[-3];
			
			m11 = p1 * q1;
			m21 = p2 * q1;
			m31 = p3 * q1;
			m41 = p4 * q1;
			ell += lskip1;
			ex -= 4;
			Z11 += m11;
			Z21 += m21;
			Z31 += m31;
			Z41 += m41;
			
		}
		
		j += 4;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			p2 = ell[-1];
			p3 = ell[-2];
			p4 = ell[-3];
			
			m11 = p1 * q1;
			m21 = p2 * q1;
			m31 = p3 * q1;
			m41 = p4 * q1;
			ell += lskip1;
			ex -= 1;
			Z11 += m11;
			Z21 += m21;
			Z31 += m31;
			Z41 += m41;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
		p1 = ell[-1];
		Z21 = ex[-1] - Z21 - p1 * Z11;
		ex[-1] = Z21;
		p1 = ell[-2];
		p2 = ell[-2 + lskip1];
		Z31 = ex[-2] - Z31 - p1 * Z11 - p2 * Z21;
		ex[-2] = Z31;
		p1 = ell[-3];
		p2 = ell[-3 + lskip1];
		p3 = ell[-3 + lskip2];
		Z41 = ex[-3] - Z41 - p1 * Z11 - p2 * Z21 - p3 * Z31;
		ex[-3] = Z41;
		
	}
	
	for (; i < n; i++)
	{
		
		
		Z11 = 0;
		ell = L - i;
		ex = B;
		
		for (j = i - 4; j >= 0; j -= 4)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			
			m11 = p1 * q1;
			ell += lskip1;
			Z11 += m11;
			
			p1 = ell[0];
			q1 = ex[-1];
			
			m11 = p1 * q1;
			ell += lskip1;
			Z11 += m11;
			
			p1 = ell[0];
			q1 = ex[-2];
			
			m11 = p1 * q1;
			ell += lskip1;
			Z11 += m11;
			
			p1 = ell[0];
			q1 = ex[-3];
			
			m11 = p1 * q1;
			ell += lskip1;
			ex -= 4;
			Z11 += m11;
			
		}
		
		j += 4;
		for (; j > 0; j--)
		{
			
			p1 = ell[0];
			q1 = ex[0];
			
			m11 = p1 * q1;
			ell += lskip1;
			ex -= 1;
			Z11 += m11;
		}
		
		Z11 = ex[0] - Z11;
		ex[0] = Z11;
	}
}

void btVectorScale(btScalar *a, const btScalar *d, int n)
{
	btAssert(a && d && n >= 0);
	for (int i = 0; i < n; i++)
	{
		a[i] *= d[i];
	}
}

void btSolveLDLT(const btScalar *L, const btScalar *d, btScalar *b, int n, int nskip)
{
	btAssert(L && d && b && n > 0 && nskip >= n);
	btSolveL1(L, b, n, nskip);
	btVectorScale(b, d, n);
	btSolveL1T(L, b, n, nskip);
}









static void btSwapRowsAndCols(BTATYPE A, int n, int i1, int i2, int nskip,
							  int do_fast_row_swaps)
{
	btAssert(A && n > 0 && i1 >= 0 && i2 >= 0 && i1 < n && i2 < n &&
			 nskip >= n && i1 < i2);

#ifdef BTROWPTRS
	btScalar *A_i1 = A[i1];
	btScalar *A_i2 = A[i2];
	for (int i = i1 + 1; i < i2; ++i)
	{
		btScalar *A_i_i1 = A[i] + i1;
		A_i1[i] = *A_i_i1;
		*A_i_i1 = A_i2[i];
	}
	A_i1[i2] = A_i1[i1];
	A_i1[i1] = A_i2[i1];
	A_i2[i1] = A_i2[i2];
	
	if (do_fast_row_swaps)
	{
		A[i1] = A_i2;
		A[i2] = A_i1;
	}
	else
	{
		
		for (int k = 0; k <= i2; ++k)
		{
			btScalar tmp = A_i1[k];
			A_i1[k] = A_i2[k];
			A_i2[k] = tmp;
		}
	}
	
	for (int j = i2 + 1; j < n; ++j)
	{
		btScalar *A_j = A[j];
		btScalar tmp = A_j[i1];
		A_j[i1] = A_j[i2];
		A_j[i2] = tmp;
	}
#else
	btScalar *A_i1 = A + i1 * nskip;
	btScalar *A_i2 = A + i2 * nskip;
	for (int k = 0; k < i1; ++k)
	{
		btScalar tmp = A_i1[k];
		A_i1[k] = A_i2[k];
		A_i2[k] = tmp;
	}
	btScalar *A_i = A_i1 + nskip;
	for (int i = i1 + 1; i < i2; A_i += nskip, ++i)
	{
		btScalar tmp = A_i2[i];
		A_i2[i] = A_i[i1];
		A_i[i1] = tmp;
	}
	{
		btScalar tmp = A_i1[i1];
		A_i1[i1] = A_i2[i2];
		A_i2[i2] = tmp;
	}
	btScalar *A_j = A_i2 + nskip;
	for (int j = i2 + 1; j < n; A_j += nskip, ++j)
	{
		btScalar tmp = A_j[i1];
		A_j[i1] = A_j[i2];
		A_j[i2] = tmp;
	}
#endif
}



static void btSwapProblem(BTATYPE A, btScalar *x, btScalar *b, btScalar *w, btScalar *lo,
						  btScalar *hi, int *p, bool *state, int *findex,
						  int n, int i1, int i2, int nskip,
						  int do_fast_row_swaps)
{
	btScalar tmpr;
	int tmpi;
	bool tmpb;
	btAssert(n > 0 && i1 >= 0 && i2 >= 0 && i1 < n && i2 < n && nskip >= n && i1 <= i2);
	if (i1 == i2) return;

	btSwapRowsAndCols(A, n, i1, i2, nskip, do_fast_row_swaps);

	tmpr = x[i1];
	x[i1] = x[i2];
	x[i2] = tmpr;

	tmpr = b[i1];
	b[i1] = b[i2];
	b[i2] = tmpr;

	tmpr = w[i1];
	w[i1] = w[i2];
	w[i2] = tmpr;

	tmpr = lo[i1];
	lo[i1] = lo[i2];
	lo[i2] = tmpr;

	tmpr = hi[i1];
	hi[i1] = hi[i2];
	hi[i2] = tmpr;

	tmpi = p[i1];
	p[i1] = p[i2];
	p[i2] = tmpi;

	tmpb = state[i1];
	state[i1] = state[i2];
	state[i2] = tmpb;

	if (findex)
	{
		tmpi = findex[i1];
		findex[i1] = findex[i2];
		findex[i2] = tmpi;
	}
}

































#ifdef btLCP_FAST

struct btLCP
{
	const int m_n;
	const int m_nskip;
	int m_nub;
	int m_nC, m_nN;                                                         
	BTATYPE const m_A;                                                      
	btScalar *const m_x, *const m_b, *const m_w, *const m_lo, *const m_hi;  
	btScalar *const m_L, *const m_d;                                        
	btScalar *const m_Dell, *const m_ell, *const m_tmp;
	bool *const m_state;
	int *const m_findex, *const m_p, *const m_C;

	btLCP(int _n, int _nskip, int _nub, btScalar *_Adata, btScalar *_x, btScalar *_b, btScalar *_w,
		  btScalar *_lo, btScalar *_hi, btScalar *l, btScalar *_d,
		  btScalar *_Dell, btScalar *_ell, btScalar *_tmp,
		  bool *_state, int *_findex, int *p, int *c, btScalar **Arows);
	int getNub() const { return m_nub; }
	void transfer_i_to_C(int i);
	void transfer_i_to_N(int i) { m_nN++; }  
	void transfer_i_from_N_to_C(int i);
	void transfer_i_from_C_to_N(int i, btAlignedObjectArray<btScalar> &scratch);
	int numC() const { return m_nC; }
	int numN() const { return m_nN; }
	int indexC(int i) const { return i; }
	int indexN(int i) const { return i + m_nC; }
	btScalar Aii(int i) const { return BTAROW(i)[i]; }
	btScalar AiC_times_qC(int i, btScalar *q) const { return btLargeDot(BTAROW(i), q, m_nC); }
	btScalar AiN_times_qN(int i, btScalar *q) const { return btLargeDot(BTAROW(i) + m_nC, q + m_nC, m_nN); }
	void pN_equals_ANC_times_qC(btScalar *p, btScalar *q);
	void pN_plusequals_ANi(btScalar *p, int i, int sign = 1);
	void pC_plusequals_s_times_qC(btScalar *p, btScalar s, btScalar *q);
	void pN_plusequals_s_times_qN(btScalar *p, btScalar s, btScalar *q);
	void solve1(btScalar *a, int i, int dir = 1, int only_transfer = 0);
	void unpermute();
};

btLCP::btLCP(int _n, int _nskip, int _nub, btScalar *_Adata, btScalar *_x, btScalar *_b, btScalar *_w,
			 btScalar *_lo, btScalar *_hi, btScalar *l, btScalar *_d,
			 btScalar *_Dell, btScalar *_ell, btScalar *_tmp,
			 bool *_state, int *_findex, int *p, int *c, btScalar **Arows) : m_n(_n), m_nskip(_nskip), m_nub(_nub), m_nC(0), m_nN(0),
#ifdef BTROWPTRS
																			 m_A(Arows),
#else
																			 m_A(_Adata),
#endif
																			 m_x(_x),
																			 m_b(_b),
																			 m_w(_w),
																			 m_lo(_lo),
																			 m_hi(_hi),
																			 m_L(l),
																			 m_d(_d),
																			 m_Dell(_Dell),
																			 m_ell(_ell),
																			 m_tmp(_tmp),
																			 m_state(_state),
																			 m_findex(_findex),
																			 m_p(p),
																			 m_C(c)
{
	{
		btSetZero(m_x, m_n);
	}

	{
#ifdef BTROWPTRS
		
		btScalar *aptr = _Adata;
		BTATYPE A = m_A;
		const int n = m_n, nskip = m_nskip;
		for (int k = 0; k < n; aptr += nskip, ++k) A[k] = aptr;
#endif
	}

	{
		int *p = m_p;
		const int n = m_n;
		for (int k = 0; k < n; ++k) p[k] = k;  
	}

	

	
	
	
	
	
	
	
	

	{
		int *findex = m_findex;
		btScalar *lo = m_lo, *hi = m_hi;
		const int n = m_n;
		for (int k = m_nub; k < n; ++k)
		{
			if (findex && findex[k] >= 0) continue;
			if (lo[k] == -BT_INFINITY && hi[k] == BT_INFINITY)
			{
				btSwapProblem(m_A, m_x, m_b, m_w, lo, hi, m_p, m_state, findex, n, m_nub, k, m_nskip, 0);
				m_nub++;
			}
		}
	}

	
	
	if (m_nub > 0)
	{
		const int nub = m_nub;
		{
			btScalar *Lrow = m_L;
			const int nskip = m_nskip;
			for (int j = 0; j < nub; Lrow += nskip, ++j) memcpy(Lrow, BTAROW(j), (j + 1) * sizeof(btScalar));
		}
		btFactorLDLT(m_L, m_d, nub, m_nskip);
		memcpy(m_x, m_b, nub * sizeof(btScalar));
		btSolveLDLT(m_L, m_d, m_x, nub, m_nskip);
		btSetZero(m_w, nub);
		{
			int *C = m_C;
			for (int k = 0; k < nub; ++k) C[k] = k;
		}
		m_nC = nub;
	}

	
	if (m_findex)
	{
		const int nub = m_nub;
		int *findex = m_findex;
		int num_at_end = 0;
		for (int k = m_n - 1; k >= nub; k--)
		{
			if (findex[k] >= 0)
			{
				btSwapProblem(m_A, m_x, m_b, m_w, m_lo, m_hi, m_p, m_state, findex, m_n, k, m_n - 1 - num_at_end, m_nskip, 1);
				num_at_end++;
			}
		}
	}

	
	
}

void btLCP::transfer_i_to_C(int i)
{
	{
		if (m_nC > 0)
		{
			
			{
				const int nC = m_nC;
				btScalar *const Ltgt = m_L + nC * m_nskip, *ell = m_ell;
				for (int j = 0; j < nC; ++j) Ltgt[j] = ell[j];
			}
			const int nC = m_nC;
			m_d[nC] = btRecip(BTAROW(i)[i] - btLargeDot(m_ell, m_Dell, nC));
		}
		else
		{
			m_d[0] = btRecip(BTAROW(i)[i]);
		}

		btSwapProblem(m_A, m_x, m_b, m_w, m_lo, m_hi, m_p, m_state, m_findex, m_n, m_nC, i, m_nskip, 1);

		const int nC = m_nC;
		m_C[nC] = nC;
		m_nC = nC + 1;  
	}
}

void btLCP::transfer_i_from_N_to_C(int i)
{
	{
		if (m_nC > 0)
		{
			{
				btScalar *const aptr = BTAROW(i);
				btScalar *Dell = m_Dell;
				const int *C = m_C;
#ifdef BTNUB_OPTIMIZATIONS
				
				const int nub = m_nub;
				int j = 0;
				for (; j < nub; ++j) Dell[j] = aptr[j];
				const int nC = m_nC;
				for (; j < nC; ++j) Dell[j] = aptr[C[j]];
#else
				const int nC = m_nC;
				for (int j = 0; j < nC; ++j) Dell[j] = aptr[C[j]];
#endif
			}
			btSolveL1(m_L, m_Dell, m_nC, m_nskip);
			{
				const int nC = m_nC;
				btScalar *const Ltgt = m_L + nC * m_nskip;
				btScalar *ell = m_ell, *Dell = m_Dell, *d = m_d;
				for (int j = 0; j < nC; ++j) Ltgt[j] = ell[j] = Dell[j] * d[j];
			}
			const int nC = m_nC;
			m_d[nC] = btRecip(BTAROW(i)[i] - btLargeDot(m_ell, m_Dell, nC));
		}
		else
		{
			m_d[0] = btRecip(BTAROW(i)[i]);
		}

		btSwapProblem(m_A, m_x, m_b, m_w, m_lo, m_hi, m_p, m_state, m_findex, m_n, m_nC, i, m_nskip, 1);

		const int nC = m_nC;
		m_C[nC] = nC;
		m_nN--;
		m_nC = nC + 1;  
	}

	
	
	
	
	
}

void btRemoveRowCol(btScalar *A, int n, int nskip, int r)
{
	btAssert(A && n > 0 && nskip >= n && r >= 0 && r < n);
	if (r >= n - 1) return;
	if (r > 0)
	{
		{
			const size_t move_size = (n - r - 1) * sizeof(btScalar);
			btScalar *Adst = A + r;
			for (int i = 0; i < r; Adst += nskip, ++i)
			{
				btScalar *Asrc = Adst + 1;
				memmove(Adst, Asrc, move_size);
			}
		}
		{
			const size_t cpy_size = r * sizeof(btScalar);
			btScalar *Adst = A + r * nskip;
			for (int i = r; i < (n - 1); ++i)
			{
				btScalar *Asrc = Adst + nskip;
				memcpy(Adst, Asrc, cpy_size);
				Adst = Asrc;
			}
		}
	}
	{
		const size_t cpy_size = (n - r - 1) * sizeof(btScalar);
		btScalar *Adst = A + r * (nskip + 1);
		for (int i = r; i < (n - 1); ++i)
		{
			btScalar *Asrc = Adst + (nskip + 1);
			memcpy(Adst, Asrc, cpy_size);
			Adst = Asrc - 1;
		}
	}
}

void btLDLTAddTL(btScalar *L, btScalar *d, const btScalar *a, int n, int nskip, btAlignedObjectArray<btScalar> &scratch)
{
	btAssert(L && d && a && n > 0 && nskip >= n);

	if (n < 2) return;
	scratch.resize(2 * nskip);
	btScalar *W1 = &scratch[0];

	btScalar *W2 = W1 + nskip;

	W1[0] = btScalar(0.0);
	W2[0] = btScalar(0.0);
	for (int j = 1; j < n; ++j)
	{
		W1[j] = W2[j] = (btScalar)(a[j] * SIMDSQRT12);
	}
	btScalar W11 = (btScalar)((btScalar(0.5) * a[0] + 1) * SIMDSQRT12);
	btScalar W21 = (btScalar)((btScalar(0.5) * a[0] - 1) * SIMDSQRT12);

	btScalar alpha1 = btScalar(1.0);
	btScalar alpha2 = btScalar(1.0);

	{
		btScalar dee = d[0];
		btScalar alphanew = alpha1 + (W11 * W11) * dee;
		btAssert(alphanew != btScalar(0.0));
		dee /= alphanew;
		btScalar gamma1 = W11 * dee;
		dee *= alpha1;
		alpha1 = alphanew;
		alphanew = alpha2 - (W21 * W21) * dee;
		dee /= alphanew;
		
		alpha2 = alphanew;
		btScalar k1 = btScalar(1.0) - W21 * gamma1;
		btScalar k2 = W21 * gamma1 * W11 - W21;
		btScalar *ll = L + nskip;
		for (int p = 1; p < n; ll += nskip, ++p)
		{
			btScalar Wp = W1[p];
			btScalar ell = *ll;
			W1[p] = Wp - W11 * ell;
			W2[p] = k1 * Wp + k2 * ell;
		}
	}

	btScalar *ll = L + (nskip + 1);
	for (int j = 1; j < n; ll += nskip + 1, ++j)
	{
		btScalar k1 = W1[j];
		btScalar k2 = W2[j];

		btScalar dee = d[j];
		btScalar alphanew = alpha1 + (k1 * k1) * dee;
		btAssert(alphanew != btScalar(0.0));
		dee /= alphanew;
		btScalar gamma1 = k1 * dee;
		dee *= alpha1;
		alpha1 = alphanew;
		alphanew = alpha2 - (k2 * k2) * dee;
		dee /= alphanew;
		btScalar gamma2 = k2 * dee;
		dee *= alpha2;
		d[j] = dee;
		alpha2 = alphanew;

		btScalar *l = ll + nskip;
		for (int p = j + 1; p < n; l += nskip, ++p)
		{
			btScalar ell = *l;
			btScalar Wp = W1[p] - k1 * ell;
			ell += gamma1 * Wp;
			W1[p] = Wp;
			Wp = W2[p] - k2 * ell;
			ell -= gamma2 * Wp;
			W2[p] = Wp;
			*l = ell;
		}
	}
}

#define _BTGETA(i, j) (A[i][j])

#define BTGETA(i, j) ((i > j) ? _BTGETA(i, j) : _BTGETA(j, i))

inline size_t btEstimateLDLTAddTLTmpbufSize(int nskip)
{
	return nskip * 2 * sizeof(btScalar);
}

void btLDLTRemove(btScalar **A, const int *p, btScalar *L, btScalar *d,
				  int n1, int n2, int r, int nskip, btAlignedObjectArray<btScalar> &scratch)
{
	btAssert(A && p && L && d && n1 > 0 && n2 > 0 && r >= 0 && r < n2 &&
			 n1 >= n2 && nskip >= n1);
#ifdef BT_DEBUG
	for (int i = 0; i < n2; ++i)
		btAssert(p[i] >= 0 && p[i] < n1);
#endif

	if (r == n2 - 1)
	{
		return;  
	}
	else
	{
		size_t LDLTAddTL_size = btEstimateLDLTAddTLTmpbufSize(nskip);
		btAssert(LDLTAddTL_size % sizeof(btScalar) == 0);
		scratch.resize(nskip * 2 + n2);
		btScalar *tmp = &scratch[0];
		if (r == 0)
		{
			btScalar *a = (btScalar *)((char *)tmp + LDLTAddTL_size);
			const int p_0 = p[0];
			for (int i = 0; i < n2; ++i)
			{
				a[i] = -BTGETA(p[i], p_0);
			}
			a[0] += btScalar(1.0);
			btLDLTAddTL(L, d, a, n2, nskip, scratch);
		}
		else
		{
			btScalar *t = (btScalar *)((char *)tmp + LDLTAddTL_size);
			{
				btScalar *Lcurr = L + r * nskip;
				for (int i = 0; i < r; ++Lcurr, ++i)
				{
					btAssert(d[i] != btScalar(0.0));
					t[i] = *Lcurr / d[i];
				}
			}
			btScalar *a = t + r;
			{
				btScalar *Lcurr = L + r * nskip;
				const int *pp_r = p + r, p_r = *pp_r;
				const int n2_minus_r = n2 - r;
				for (int i = 0; i < n2_minus_r; Lcurr += nskip, ++i)
				{
					a[i] = btLargeDot(Lcurr, t, r) - BTGETA(pp_r[i], p_r);
				}
			}
			a[0] += btScalar(1.0);
			btLDLTAddTL(L + r * nskip + r, d + r, a, n2 - r, nskip, scratch);
		}
	}

	
	btRemoveRowCol(L, n2, nskip, r);
	if (r < (n2 - 1)) memmove(d + r, d + r + 1, (n2 - r - 1) * sizeof(btScalar));
}

void btLCP::transfer_i_from_C_to_N(int i, btAlignedObjectArray<btScalar> &scratch)
{
	{
		int *C = m_C;
		
		
		int last_idx = -1;
		const int nC = m_nC;
		int j = 0;
		for (; j < nC; ++j)
		{
			if (C[j] == nC - 1)
			{
				last_idx = j;
			}
			if (C[j] == i)
			{
				btLDLTRemove(m_A, C, m_L, m_d, m_n, nC, j, m_nskip, scratch);
				int k;
				if (last_idx == -1)
				{
					for (k = j + 1; k < nC; ++k)
					{
						if (C[k] == nC - 1)
						{
							break;
						}
					}
					btAssert(k < nC);
				}
				else
				{
					k = last_idx;
				}
				C[k] = C[j];
				if (j < (nC - 1)) memmove(C + j, C + j + 1, (nC - j - 1) * sizeof(int));
				break;
			}
		}
		btAssert(j < nC);

		btSwapProblem(m_A, m_x, m_b, m_w, m_lo, m_hi, m_p, m_state, m_findex, m_n, i, nC - 1, m_nskip, 1);

		m_nN++;
		m_nC = nC - 1;  
	}
}

void btLCP::pN_equals_ANC_times_qC(btScalar *p, btScalar *q)
{
	
	
	
	
	
	const int nC = m_nC;
	btScalar *ptgt = p + nC;
	const int nN = m_nN;
	for (int i = 0; i < nN; ++i)
	{
		ptgt[i] = btLargeDot(BTAROW(i + nC), q, nC);
	}
}

void btLCP::pN_plusequals_ANi(btScalar *p, int i, int sign)
{
	const int nC = m_nC;
	btScalar *aptr = BTAROW(i) + nC;
	btScalar *ptgt = p + nC;
	if (sign > 0)
	{
		const int nN = m_nN;
		for (int j = 0; j < nN; ++j) ptgt[j] += aptr[j];
	}
	else
	{
		const int nN = m_nN;
		for (int j = 0; j < nN; ++j) ptgt[j] -= aptr[j];
	}
}

void btLCP::pC_plusequals_s_times_qC(btScalar *p, btScalar s, btScalar *q)
{
	const int nC = m_nC;
	for (int i = 0; i < nC; ++i)
	{
		p[i] += s * q[i];
	}
}

void btLCP::pN_plusequals_s_times_qN(btScalar *p, btScalar s, btScalar *q)
{
	const int nC = m_nC;
	btScalar *ptgt = p + nC, *qsrc = q + nC;
	const int nN = m_nN;
	for (int i = 0; i < nN; ++i)
	{
		ptgt[i] += s * qsrc[i];
	}
}

void btLCP::solve1(btScalar *a, int i, int dir, int only_transfer)
{
	
	
	
	
	

	if (m_nC > 0)
	{
		{
			btScalar *Dell = m_Dell;
			int *C = m_C;
			btScalar *aptr = BTAROW(i);
#ifdef BTNUB_OPTIMIZATIONS
			
			const int nub = m_nub;
			int j = 0;
			for (; j < nub; ++j) Dell[j] = aptr[j];
			const int nC = m_nC;
			for (; j < nC; ++j) Dell[j] = aptr[C[j]];
#else
			const int nC = m_nC;
			for (int j = 0; j < nC; ++j) Dell[j] = aptr[C[j]];
#endif
		}
		btSolveL1(m_L, m_Dell, m_nC, m_nskip);
		{
			btScalar *ell = m_ell, *Dell = m_Dell, *d = m_d;
			const int nC = m_nC;
			for (int j = 0; j < nC; ++j) ell[j] = Dell[j] * d[j];
		}

		if (!only_transfer)
		{
			btScalar *tmp = m_tmp, *ell = m_ell;
			{
				const int nC = m_nC;
				for (int j = 0; j < nC; ++j) tmp[j] = ell[j];
			}
			btSolveL1T(m_L, tmp, m_nC, m_nskip);
			if (dir > 0)
			{
				int *C = m_C;
				btScalar *tmp = m_tmp;
				const int nC = m_nC;
				for (int j = 0; j < nC; ++j) a[C[j]] = -tmp[j];
			}
			else
			{
				int *C = m_C;
				btScalar *tmp = m_tmp;
				const int nC = m_nC;
				for (int j = 0; j < nC; ++j) a[C[j]] = tmp[j];
			}
		}
	}
}

void btLCP::unpermute()
{
	
	{
		memcpy(m_tmp, m_x, m_n * sizeof(btScalar));
		btScalar *x = m_x, *tmp = m_tmp;
		const int *p = m_p;
		const int n = m_n;
		for (int j = 0; j < n; ++j) x[p[j]] = tmp[j];
	}
	{
		memcpy(m_tmp, m_w, m_n * sizeof(btScalar));
		btScalar *w = m_w, *tmp = m_tmp;
		const int *p = m_p;
		const int n = m_n;
		for (int j = 0; j < n; ++j) w[p[j]] = tmp[j];
	}
}

#endif  




bool btSolveDantzigLCP(int n, btScalar *A, btScalar *x, btScalar *b,
					   btScalar *outer_w, int nub, btScalar *lo, btScalar *hi, int *findex, btDantzigScratchMemory &scratchMem)
{
	s_error = false;

	
	btAssert(n > 0 && A && x && b && lo && hi && nub >= 0 && nub <= n);
	btAssert(outer_w);

#ifdef BT_DEBUG
	{
		
		for (int k = 0; k < n; ++k)
			btAssert(lo[k] <= 0 && hi[k] >= 0);
	}
#endif

	
	
	if (nub >= n)
	{
		int nskip = (n);
		btFactorLDLT(A, outer_w, n, nskip);
		btSolveLDLT(A, outer_w, b, n, nskip);
		memcpy(x, b, n * sizeof(btScalar));

		return !s_error;
	}

	const int nskip = (n);
	scratchMem.L.resize(n * nskip);

	scratchMem.d.resize(n);

	btScalar *w = outer_w;
	scratchMem.delta_w.resize(n);
	scratchMem.delta_x.resize(n);
	scratchMem.Dell.resize(n);
	scratchMem.ell.resize(n);
	scratchMem.Arows.resize(n);
	scratchMem.p.resize(n);
	scratchMem.C.resize(n);

	
	scratchMem.state.resize(n);

	
	
	btLCP lcp(n, nskip, nub, A, x, b, w, lo, hi, &scratchMem.L[0], &scratchMem.d[0], &scratchMem.Dell[0], &scratchMem.ell[0], &scratchMem.delta_w[0], &scratchMem.state[0], findex, &scratchMem.p[0], &scratchMem.C[0], &scratchMem.Arows[0]);
	int adj_nub = lcp.getNub();

	
	
	
	
	
	
	
	

	bool hit_first_friction_index = false;
	for (int i = adj_nub; i < n; ++i)
	{
		s_error = false;
		
		
		
		

		
		
		
		
		
		
		

		if (!hit_first_friction_index && findex && findex[i] >= 0)
		{
			
			for (int j = 0; j < n; ++j) scratchMem.delta_w[scratchMem.p[j]] = x[j];

			
			for (int k = i; k < n; ++k)
			{
				btScalar wfk = scratchMem.delta_w[findex[k]];
				if (wfk == 0)
				{
					hi[k] = 0;
					lo[k] = 0;
				}
				else
				{
					hi[k] = btFabs(hi[k] * wfk);
					lo[k] = -hi[k];
				}
			}
			hit_first_friction_index = true;
		}

		
		
		w[i] = lcp.AiC_times_qC(i, x) + lcp.AiN_times_qN(i, x) - b[i];

		
		
		
		
		
		
		
		
		

		
		if (lo[i] == 0 && w[i] >= 0)
		{
			lcp.transfer_i_to_N(i);
			scratchMem.state[i] = false;
		}
		else if (hi[i] == 0 && w[i] <= 0)
		{
			lcp.transfer_i_to_N(i);
			scratchMem.state[i] = true;
		}
		else if (w[i] == 0)
		{
			
			
			
			
			
			lcp.solve1(&scratchMem.delta_x[0], i, 0, 1);

			lcp.transfer_i_to_C(i);
		}
		else
		{
			
			for (;;)
			{
				int dir;
				btScalar dirf;
				
				if (w[i] <= 0)
				{
					dir = 1;
					dirf = btScalar(1.0);
				}
				else
				{
					dir = -1;
					dirf = btScalar(-1.0);
				}

				
				lcp.solve1(&scratchMem.delta_x[0], i, dir);

				

				
				
				lcp.pN_equals_ANC_times_qC(&scratchMem.delta_w[0], &scratchMem.delta_x[0]);
				lcp.pN_plusequals_ANi(&scratchMem.delta_w[0], i, dir);
				scratchMem.delta_w[i] = lcp.AiC_times_qC(i, &scratchMem.delta_x[0]) + lcp.Aii(i) * dirf;

				
				
				

				int cmd = 1;  
				int si = 0;   
				btScalar s = -w[i] / scratchMem.delta_w[i];
				if (dir > 0)
				{
					if (hi[i] < BT_INFINITY)
					{
						btScalar s2 = (hi[i] - x[i]) * dirf;  
						if (s2 < s)
						{
							s = s2;
							cmd = 3;
						}
					}
				}
				else
				{
					if (lo[i] > -BT_INFINITY)
					{
						btScalar s2 = (lo[i] - x[i]) * dirf;  
						if (s2 < s)
						{
							s = s2;
							cmd = 2;
						}
					}
				}

				{
					const int numN = lcp.numN();
					for (int k = 0; k < numN; ++k)
					{
						const int indexN_k = lcp.indexN(k);
						if (!scratchMem.state[indexN_k] ? scratchMem.delta_w[indexN_k] < 0 : scratchMem.delta_w[indexN_k] > 0)
						{
							
							if (lo[indexN_k] == 0 && hi[indexN_k] == 0) continue;
							btScalar s2 = -w[indexN_k] / scratchMem.delta_w[indexN_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 4;
								si = indexN_k;
							}
						}
					}
				}

				{
					const int numC = lcp.numC();
					for (int k = adj_nub; k < numC; ++k)
					{
						const int indexC_k = lcp.indexC(k);
						if (scratchMem.delta_x[indexC_k] < 0 && lo[indexC_k] > -BT_INFINITY)
						{
							btScalar s2 = (lo[indexC_k] - x[indexC_k]) / scratchMem.delta_x[indexC_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 5;
								si = indexC_k;
							}
						}
						if (scratchMem.delta_x[indexC_k] > 0 && hi[indexC_k] < BT_INFINITY)
						{
							btScalar s2 = (hi[indexC_k] - x[indexC_k]) / scratchMem.delta_x[indexC_k];
							if (s2 < s)
							{
								s = s2;
								cmd = 6;
								si = indexC_k;
							}
						}
					}
				}

				
				
				

				
				
				
				if (s <= btScalar(0.0))
				{
					
					if (i < n)
					{
						btSetZero(x + i, n - i);
						btSetZero(w + i, n - i);
					}
					s_error = true;
					break;
				}

				
				lcp.pC_plusequals_s_times_qC(x, s, &scratchMem.delta_x[0]);
				x[i] += s * dirf;

				
				lcp.pN_plusequals_s_times_qN(w, s, &scratchMem.delta_w[0]);
				w[i] += s * scratchMem.delta_w[i];

				
				
				switch (cmd)
				{
					case 1:  
						w[i] = 0;
						lcp.transfer_i_to_C(i);
						break;
					case 2:  
						x[i] = lo[i];
						scratchMem.state[i] = false;
						lcp.transfer_i_to_N(i);
						break;
					case 3:  
						x[i] = hi[i];
						scratchMem.state[i] = true;
						lcp.transfer_i_to_N(i);
						break;
					case 4:  
						w[si] = 0;
						lcp.transfer_i_from_N_to_C(si);
						break;
					case 5:  
						x[si] = lo[si];
						scratchMem.state[si] = false;
						lcp.transfer_i_from_C_to_N(si, scratchMem.m_scratch);
						break;
					case 6:  
						x[si] = hi[si];
						scratchMem.state[si] = true;
						lcp.transfer_i_from_C_to_N(si, scratchMem.m_scratch);
						break;
				}

				if (cmd <= 3) break;
			}  
		}      

		if (s_error)
		{
			break;
		}
	}  

	lcp.unpermute();

	return !s_error;
}
