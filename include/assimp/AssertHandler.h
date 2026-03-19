



#ifndef INCLUDED_AI_ASSERTHANDLER_H
#define INCLUDED_AI_ASSERTHANDLER_H

#include <assimp/ai_assert.h>
#include <assimp/defs.h>

namespace Assimp {



using AiAssertHandler = void (*)(const char* failedExpression, const char* file, int line);



ASSIMP_API void setAiAssertHandler(AiAssertHandler handler);



AI_WONT_RETURN ASSIMP_API void defaultAiAssertHandler(const char* failedExpression, const char* file, int line) AI_WONT_RETURN_SUFFIX;



ASSIMP_API void aiAssertViolation(const char* failedExpression, const char* file, int line);

} 

#endif 
