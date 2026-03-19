


#pragma once
#ifndef AI_PROGRESSHANDLER_H_INC
#define AI_PROGRESSHANDLER_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

namespace Assimp {



class ASSIMP_API ProgressHandler
#ifndef SWIG
    : public Intern::AllocateFromAssimpHeap
#endif
{
protected:
    
    ProgressHandler () AI_NO_EXCEPT = default;

public:
    
    virtual ~ProgressHandler () = default;

    
    
    virtual bool Update(float percentage = -1.f) = 0;

    
    
    virtual void UpdateFileRead(int currentStep , int numberOfSteps ) {
        float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
        Update( f * 0.5f );
    }

    
    
    virtual void UpdatePostProcess(int currentStep , int numberOfSteps ) {
        float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
        Update( f * 0.5f + 0.5f );
    }


    
    
    virtual void UpdateFileWrite(int currentStep , int numberOfSteps ) {
        float f = numberOfSteps ? currentStep / (float)numberOfSteps : 1.0f;
        Update(f * 0.5f);
    }
}; 



} 

#endif 
