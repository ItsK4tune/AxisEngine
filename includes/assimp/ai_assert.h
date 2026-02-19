



#pragma once
#ifndef AI_ASSERT_H_INC
#define AI_ASSERT_H_INC

#include <assimp/defs.h>

#if defined(ASSIMP_BUILD_DEBUG)

namespace Assimp {





ASSIMP_API void aiAssertViolation(const char* failedExpression, const char* file, int line);

}
#endif


#if defined(ASSIMP_BUILD_DEBUG)
#   define ai_assert(expression) (void)((!!(expression)) || (Assimp::aiAssertViolation(#expression, __FILE__, __LINE__), 0))
#   define ai_assert_entry() ai_assert(false)
#else
#   define  ai_assert(expression)
#   define  ai_assert_entry()
#endif 

#endif 
