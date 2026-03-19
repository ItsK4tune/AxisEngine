


#pragma once
#ifndef AI_SUBDISIVION_H_INC
#define AI_SUBDISIVION_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>

struct aiMesh;

namespace Assimp {




class ASSIMP_API Subdivider {
public:

    
    enum Algorithm  {
        CATMULL_CLARKE = 0x1
    };

    virtual ~Subdivider();

    
    
    static Subdivider* Create (Algorithm algo);

    
    
    virtual void Subdivide ( aiMesh* mesh,
        aiMesh*& out, unsigned int num,
        bool discard_input = false) = 0;

    
    
    virtual void Subdivide (
        aiMesh** smesh,
        size_t nmesh,
        aiMesh** out,
        unsigned int num,
        bool discard_input = false) = 0;

};

inline Subdivider::~Subdivider() = default;

} 


#endif 

