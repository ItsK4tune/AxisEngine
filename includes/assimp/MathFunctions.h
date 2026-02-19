

#pragma once

#ifdef __GNUC__
#   pragma GCC system_header
#endif



#include <limits>

namespace Assimp {
namespace Math {





template <typename IntegerType>
inline IntegerType gcd( IntegerType a, IntegerType b ) {
	const IntegerType zero = (IntegerType)0;
	while ( true ) {
		if ( a == zero ) {
			return b;
        }
		b %= a;

		if ( b == zero ) {
			return a;
        }
		a %= b;
	}
}





template < typename IntegerType >
inline IntegerType lcm( IntegerType a, IntegerType b ) {
	const IntegerType t = gcd (a,b);
	if (!t) {
        return t;
    }
	return a / t * b;
}


template<class T>
inline T getEpsilon() {
    return std::numeric_limits<T>::epsilon();
}



template<class T>
inline T aiPi() {
    return static_cast<T>(3.14159265358979323846);
}

} 
} 
