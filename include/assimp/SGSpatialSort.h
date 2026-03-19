


#pragma once
#ifndef AI_D3DSSPATIALSORT_H_INC
#define AI_D3DSSPATIALSORT_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/types.h>
#include <vector>
#include <stdint.h>

namespace Assimp {




class ASSIMP_API SGSpatialSort {
public:
    SGSpatialSort();

    
    
    explicit SGSpatialSort(const std::vector<aiVector3D>& vPositions);

    
    
    void Add(const aiVector3D& vPosition, unsigned int index,
        unsigned int smoothingGroup);

    
    
    void Prepare();

    
    ~SGSpatialSort() = default;

    
    
    
    void FindPositions( const aiVector3D& pPosition, uint32_t pSG,
        float pRadius, std::vector<unsigned int>& poResults,
        bool exactMatch = false) const;

protected:
    
    aiVector3D mPlaneNormal;

    
    
    
    struct Entry {
        unsigned int mIndex;    
        aiVector3D mPosition;   
        uint32_t mSmoothGroups;
        float mDistance;        

        Entry() AI_NO_EXCEPT
        : mIndex(0)
        , mPosition()
        , mSmoothGroups(0)
        , mDistance(0.0f) {
            
        }

        Entry( unsigned int pIndex, const aiVector3D& pPosition, float pDistance,uint32_t pSG)
        : mIndex( pIndex)
        , mPosition( pPosition)
        , mSmoothGroups(pSG)
        , mDistance( pDistance) {
            
        }

        bool operator < (const Entry& e) const {
            return mDistance < e.mDistance;
        }
    };

    
    std::vector<Entry> mPositions;
};

} 

#endif 
