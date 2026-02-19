

#ifndef btImplicitQRSVD_h
#define btImplicitQRSVD_h
#include <limits>
#include "btMatrix3x3.h"
class btMatrix2x2
{
public:
    btScalar m_00, m_01, m_10, m_11;
    btMatrix2x2(): m_00(0), m_10(0), m_01(0), m_11(0)
    {
    }
    btMatrix2x2(const btMatrix2x2& other): m_00(other.m_00),m_01(other.m_01),m_10(other.m_10),m_11(other.m_11)
    {}
    btScalar& operator()(int i, int j)
    {
        if (i == 0 && j == 0)
            return m_00;
        if (i == 1 && j == 0)
            return m_10;
        if (i == 0 && j == 1)
            return m_01;
        if (i == 1 && j == 1)
            return m_11;
        btAssert(false);
        return m_00;
    }
    const btScalar& operator()(int i, int j) const
    {
        if (i == 0 && j == 0)
            return m_00;
        if (i == 1 && j == 0)
            return m_10;
        if (i == 0 && j == 1)
            return m_01;
        if (i == 1 && j == 1)
            return m_11;
        btAssert(false);
        return m_00;
    }
    void setIdentity()
    {
        m_00 = 1;
        m_11 = 1;
        m_01 = 0;
        m_10 = 0;
    }
};

static inline btScalar copySign(btScalar x, btScalar y) {
    if ((x < 0 && y > 0) || (x > 0 && y < 0))
        return -x;
    return x;
}



class GivensRotation {
public:
    int rowi;
    int rowk;
    btScalar c;
    btScalar s;
    
    inline GivensRotation(int rowi_in, int rowk_in)
    : rowi(rowi_in)
    , rowk(rowk_in)
    , c(1)
    , s(0)
    {
    }
    
    inline GivensRotation(btScalar a, btScalar b, int rowi_in, int rowk_in)
    : rowi(rowi_in)
    , rowk(rowk_in)
    {
        compute(a, b);
    }
    
    ~GivensRotation() {}
    
    inline void transposeInPlace()
    {
        s = -s;
    }
    
    
    inline void compute(const btScalar a, const btScalar b)
    {
        btScalar d = a * a + b * b;
        c = 1;
        s = 0;
        if (d > SIMD_EPSILON) {
            btScalar sqrtd = btSqrt(d);
            if (sqrtd>SIMD_EPSILON)
            {
              btScalar t = btScalar(1.0)/sqrtd;
              c = a * t;
              s = -b * t;
            }
        }
    }
    
    
    inline void computeUnconventional(const btScalar a, const btScalar b)
    {
        btScalar d = a * a + b * b;
        c = 0;
        s = 1;
        if (d > SIMD_EPSILON) {
            btScalar t = btScalar(1.0)/btSqrt(d);
            s = a * t;
            c = b * t;
        }
    }
    
    inline void fill(const btMatrix3x3& R) const
    {
        btMatrix3x3& A = const_cast<btMatrix3x3&>(R);
        A.setIdentity();
        A[rowi][rowi] = c;
        A[rowk][rowi] = -s;
        A[rowi][rowk] = s;
        A[rowk][rowk] = c;
    }
    
    inline void fill(const btMatrix2x2& R) const
    {
        btMatrix2x2& A = const_cast<btMatrix2x2&>(R);
        A(rowi,rowi) = c;
        A(rowk,rowi) = -s;
        A(rowi,rowk) = s;
        A(rowk,rowk) = c;
    }
    
    
    inline void rowRotation(btMatrix3x3& A) const
    {
        for (int j = 0; j < 3; j++) {
            btScalar tau1 = A[rowi][j];
            btScalar tau2 = A[rowk][j];
            A[rowi][j] = c * tau1 - s * tau2;
            A[rowk][j] = s * tau1 + c * tau2;
        }
    }
    inline void rowRotation(btMatrix2x2& A) const
    {
        for (int j = 0; j < 2; j++) {
            btScalar tau1 = A(rowi,j);
            btScalar tau2 = A(rowk,j);
            A(rowi,j) = c * tau1 - s * tau2;
            A(rowk,j) = s * tau1 + c * tau2;
        }
    }
    
    
    inline void columnRotation(btMatrix3x3& A) const
    {
        for (int j = 0; j < 3; j++) {
            btScalar tau1 = A[j][rowi];
            btScalar tau2 = A[j][rowk];
            A[j][rowi] = c * tau1 - s * tau2;
            A[j][rowk] = s * tau1 + c * tau2;
        }
    }
    inline void columnRotation(btMatrix2x2& A) const
    {
        for (int j = 0; j < 2; j++) {
            btScalar tau1 = A(j,rowi);
            btScalar tau2 = A(j,rowk);
            A(j,rowi) = c * tau1 - s * tau2;
            A(j,rowk) = s * tau1 + c * tau2;
        }
    }
    
    
    inline void operator*=(const GivensRotation& A)
    {
        btScalar new_c = c * A.c - s * A.s;
        btScalar new_s = s * A.c + c * A.s;
        c = new_c;
        s = new_s;
    }
    
    
    inline GivensRotation operator*(const GivensRotation& A) const
    {
        GivensRotation r(*this);
        r *= A;
        return r;
    }
};


inline void zeroChase(btMatrix3x3& H, btMatrix3x3& U, btMatrix3x3& V)
{
    
    
    GivensRotation r1(H[0][0], H[1][0], 0, 1);
    
    GivensRotation r2(1, 2);
    if (H[1][0] != 0)
        r2.compute(H[0][0] * H[0][1] + H[1][0] * H[1][1], H[0][0] * H[0][2] + H[1][0] * H[1][2]);
    else
        r2.compute(H[0][1], H[0][2]);
    
    r1.rowRotation(H);
    
    
    r2.columnRotation(H);
    r2.columnRotation(V);
    
    
    GivensRotation r3(H[1][1], H[2][1], 1, 2);
    r3.rowRotation(H);
    
    
    
    
    r1.columnRotation(U);
    r3.columnRotation(U);
}


inline void makeUpperBidiag(btMatrix3x3& H, btMatrix3x3& U, btMatrix3x3& V)
{
    U.setIdentity();
    V.setIdentity();
    
    
    
    GivensRotation r(H[1][0], H[2][0], 1, 2);
    r.rowRotation(H);
    
    r.columnRotation(U);
    
    zeroChase(H, U, V);
}


inline void makeLambdaShape(btMatrix3x3& H, btMatrix3x3& U, btMatrix3x3& V)
{
    U.setIdentity();
    V.setIdentity();
    
    
    
    GivensRotation r1(H[0][1], H[0][2], 1, 2);
    r1.columnRotation(H);
    r1.columnRotation(V);
    
    
    
    r1.computeUnconventional(H[1][2], H[2][2]);
    r1.rowRotation(H);
    r1.columnRotation(U);
    
    
    
    GivensRotation r2(H[2][0], H[2][1], 0, 1);
    r2.columnRotation(H);
    r2.columnRotation(V);
    
    
    r2.computeUnconventional(H[0][1], H[1][1]);
    r2.rowRotation(H);
    r2.columnRotation(U);
}


inline void polarDecomposition(const btMatrix2x2& A,
                   GivensRotation& R,
                   const btMatrix2x2& S_Sym)
{
    btScalar a = (A(0, 0) + A(1, 1)),  b = (A(1, 0) - A(0, 1));
    btScalar denominator = btSqrt(a*a+b*b);
    R.c = (btScalar)1;
    R.s = (btScalar)0;
    if (denominator > SIMD_EPSILON) { 
        
        R.c = a / denominator;
        R.s = -b / denominator;
    }
    btMatrix2x2& S = const_cast<btMatrix2x2&>(S_Sym);
    S = A;
    R.rowRotation(S);
}

inline void polarDecomposition(const btMatrix2x2& A,
                   const btMatrix2x2& R,
                   const btMatrix2x2& S_Sym)
{
    GivensRotation r(0, 1);
    polarDecomposition(A, r, S_Sym);
    r.fill(R);
}


inline void singularValueDecomposition(
                           const btMatrix2x2& A,
                           GivensRotation& U,
                           const btMatrix2x2& Sigma,
                           GivensRotation& V,
                           const btScalar tol = 64 * std::numeric_limits<btScalar>::epsilon())
{
    btMatrix2x2& sigma = const_cast<btMatrix2x2&>(Sigma);
    sigma.setIdentity();
    btMatrix2x2 S_Sym;
    polarDecomposition(A, U, S_Sym);
    btScalar cosine, sine;
    btScalar x = S_Sym(0, 0);
    btScalar y = S_Sym(0, 1);
    btScalar z = S_Sym(1, 1);
    if (y == 0) {
        
        cosine = 1;
        sine = 0;
        sigma(0,0) = x;
        sigma(1,1) = z;
    }
    else {
        btScalar tau = 0.5 * (x - z);
        btScalar val = tau * tau + y * y;
        if (val > SIMD_EPSILON)
        {
        btScalar w = btSqrt(val);
        
        btScalar t;
        if (tau > 0) {
            
            t = y / (tau + w);
        }
        else {
            
            t = y / (tau - w);
        }
        cosine = btScalar(1) / btSqrt(t * t + btScalar(1));
        sine = -t * cosine;
        
        btScalar c2 = cosine * cosine;
        btScalar csy = 2 * cosine * sine * y;
        btScalar s2 = sine * sine;
        sigma(0,0) = c2 * x - csy + s2 * z;
        sigma(1,1) = s2 * x + csy + c2 * z;
      } else
      	{
      		cosine = 1;
        sine = 0;
        sigma(0,0) = x;
        sigma(1,1) = z;
      	}
    }
    
    
    
    if (sigma(0,0) < sigma(1,1)) {
        std::swap(sigma(0,0), sigma(1,1));
        V.c = -sine;
        V.s = cosine;
    }
    else {
        V.c = cosine;
        V.s = sine;
    }
    U *= V;
}


inline void singularValueDecomposition(
                           const btMatrix2x2& A,
                           const btMatrix2x2& U,
                           const btMatrix2x2& Sigma,
                           const btMatrix2x2& V,
                           const btScalar tol = 64 * std::numeric_limits<btScalar>::epsilon())
{
    GivensRotation gv(0, 1);
    GivensRotation gu(0, 1);
    singularValueDecomposition(A, gu, Sigma, gv);
    
    gu.fill(U);
    gv.fill(V);
}


inline btScalar wilkinsonShift(const btScalar a1, const btScalar b1, const btScalar a2)
{
	btScalar d = (btScalar)0.5 * (a1 - a2);
	btScalar bs = b1 * b1;
	btScalar val = d * d + bs;
	if (val>SIMD_EPSILON)
	{
		btScalar denom = btFabs(d) + btSqrt(val);

		btScalar mu = a2 - copySign(bs / (denom), d);
		
		return mu;
	}
	return a2;
}


template <int t>
inline void process(btMatrix3x3& B, btMatrix3x3& U, btVector3& sigma, btMatrix3x3& V)
{
    int other = (t == 1) ? 0 : 2;
    GivensRotation u(0, 1);
    GivensRotation v(0, 1);
    sigma[other] = B[other][other];
    
    btMatrix2x2 B_sub, sigma_sub;
    if (t == 0)
    {
        B_sub.m_00 = B[0][0];
        B_sub.m_10 = B[1][0];
        B_sub.m_01 = B[0][1];
        B_sub.m_11 = B[1][1];
        sigma_sub.m_00 = sigma[0];
        sigma_sub.m_11 = sigma[1];

        singularValueDecomposition(B_sub, u, sigma_sub, v);
        B[0][0] = B_sub.m_00;
        B[1][0] = B_sub.m_10;
        B[0][1] = B_sub.m_01;
        B[1][1] = B_sub.m_11;
        sigma[0] = sigma_sub.m_00;
        sigma[1] = sigma_sub.m_11;
    }
    else
    {
        B_sub.m_00 = B[1][1];
        B_sub.m_10 = B[2][1];
        B_sub.m_01 = B[1][2];
        B_sub.m_11 = B[2][2];
        sigma_sub.m_00 = sigma[1];
        sigma_sub.m_11 = sigma[2];
        
        singularValueDecomposition(B_sub, u, sigma_sub, v);
        B[1][1] = B_sub.m_00;
        B[2][1] = B_sub.m_10;
        B[1][2] = B_sub.m_01;
        B[2][2] = B_sub.m_11;
        sigma[1] = sigma_sub.m_00;
        sigma[2] = sigma_sub.m_11;
    }
    u.rowi += t;
    u.rowk += t;
    v.rowi += t;
    v.rowk += t;
    u.columnRotation(U);
    v.columnRotation(V);
}


inline void flipSign(int i, btMatrix3x3& U, btVector3& sigma)
{
    sigma[i] = -sigma[i];
    U[0][i] = -U[0][i];
    U[1][i] = -U[1][i];
    U[2][i] = -U[2][i];
}

inline void flipSign(int i, btMatrix3x3& U)
{
    U[0][i] = -U[0][i];
    U[1][i] = -U[1][i];
    U[2][i] = -U[2][i];
}

inline void swapCol(btMatrix3x3& A, int i, int j)
{
    for (int d = 0; d < 3; ++d)
        std::swap(A[d][i], A[d][j]);
}

inline void sort(btMatrix3x3& U, btVector3& sigma, btMatrix3x3& V, int t)
{
    if (t == 0)
    {
        
        if (btFabs(sigma[1]) >= btFabs(sigma[2])) {
            if (sigma[1] < 0) {
                flipSign(1, U, sigma);
                flipSign(2, U, sigma);
            }
            return;
        }
        
        
        if (sigma[2] < 0) {
            flipSign(1, U, sigma);
            flipSign(2, U, sigma);
        }
        
        
        std::swap(sigma[1], sigma[2]);
        
        swapCol(U,1,2);
        swapCol(V,1,2);
        
        
        if (sigma[1] > sigma[0]) {
            std::swap(sigma[0], sigma[1]);
            swapCol(U,0,1);
            swapCol(V,0,1);
        }
        
        
        else {
            flipSign(2, U);
            flipSign(2, V);
        }
    }
    else if (t == 1)
    {
        
        if (btFabs(sigma[0]) >= sigma[1]) {
            if (sigma[0] < 0) {
                flipSign(0, U, sigma);
                flipSign(2, U, sigma);
            }
            return;
        }
        
        
        std::swap(sigma[0], sigma[1]);
        swapCol(U, 0, 1);
        swapCol(V, 0, 1);
        
        
        if (btFabs(sigma[1]) < btFabs(sigma[2])) {
            std::swap(sigma[1], sigma[2]);
            swapCol(U, 1, 2);
            swapCol(V, 1, 2);
        }
        
        
        else {
            flipSign(1, U);
            flipSign(1, V);
        }
        
        
        if (sigma[1] < 0) {
            flipSign(1, U, sigma);
            flipSign(2, U, sigma);
        }
    }
}


inline int singularValueDecomposition(const btMatrix3x3& A,
                                     btMatrix3x3& U,
                                     btVector3& sigma,
                                     btMatrix3x3& V,
                                     btScalar tol = 128*std::numeric_limits<btScalar>::epsilon())
{

    btMatrix3x3 B = A;
    U.setIdentity();
    V.setIdentity();
    
    makeUpperBidiag(B, U, V);
    
    int count = 0;
    btScalar mu = (btScalar)0;
    GivensRotation r(0, 1);
    
    btScalar alpha_1 = B[0][0];
    btScalar beta_1 = B[0][1];
    btScalar alpha_2 = B[1][1];
    btScalar alpha_3 = B[2][2];
    btScalar beta_2 = B[1][2];
    btScalar gamma_1 = alpha_1 * beta_1;
    btScalar gamma_2 = alpha_2 * beta_2;
    btScalar val = alpha_1 * alpha_1 + alpha_2 * alpha_2 + alpha_3 * alpha_3 + beta_1 * beta_1 + beta_2 * beta_2;
    if (val > SIMD_EPSILON)
    {
	    tol *= btMax((btScalar)0.5 * btSqrt(val), (btScalar)1);
		}    
    
    int max_count = 100;
    
    while (btFabs(beta_2) > tol && btFabs(beta_1) > tol
           && btFabs(alpha_1) > tol && btFabs(alpha_2) > tol
           && btFabs(alpha_3) > tol
           && count < max_count) {
        mu = wilkinsonShift(alpha_2 * alpha_2 + beta_1 * beta_1, gamma_2, alpha_3 * alpha_3 + beta_2 * beta_2);
        
        r.compute(alpha_1 * alpha_1 - mu, gamma_1);
        r.columnRotation(B);
        
        r.columnRotation(V);
        zeroChase(B, U, V);
        
        alpha_1 = B[0][0];
        beta_1 = B[0][1];
        alpha_2 = B[1][1];
        alpha_3 = B[2][2];
        beta_2 = B[1][2];
        gamma_1 = alpha_1 * beta_1;
        gamma_2 = alpha_2 * beta_2;
        count++;
    }
    
    if (btFabs(beta_2) <= tol) {
        process<0>(B, U, sigma, V);
        sort(U, sigma, V,0);
    }
    
    else if (btFabs(beta_1) <= tol) {
        process<1>(B, U, sigma, V);
        sort(U, sigma, V,1);
    }
    
    else if (btFabs(alpha_2) <= tol) {
        
        GivensRotation r1(1, 2);
        r1.computeUnconventional(B[1][2], B[2][2]);
        r1.rowRotation(B);
        r1.columnRotation(U);
        
        process<0>(B, U, sigma, V);
        sort(U, sigma, V, 0);
    }
    
    else if (btFabs(alpha_3) <= tol) {
        
        GivensRotation r1(1, 2);
        r1.compute(B[1][1], B[1][2]);
        r1.columnRotation(B);
        r1.columnRotation(V);
        
        GivensRotation r2(0, 2);
        r2.compute(B[0][0], B[0][2]);
        r2.columnRotation(B);
        r2.columnRotation(V);
        
        process<0>(B, U, sigma, V);
        sort(U, sigma, V, 0);
    }
    
    else if (btFabs(alpha_1) <= tol) {
        
        GivensRotation r1(0, 1);
        r1.computeUnconventional(B[0][1], B[1][1]);
        r1.rowRotation(B);
        r1.columnRotation(U);
        
        
        GivensRotation r2(0, 2);
        r2.computeUnconventional(B[0][2], B[2][2]);
        r2.rowRotation(B);
        r2.columnRotation(U);
        
        process<1>(B, U, sigma, V);
        sort(U, sigma, V, 1);
    }
    
    return count;
}
#endif 
