


#pragma once
#ifndef AI_SPATIALSORT_H_INC
#define AI_SPATIALSORT_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/types.h>
#include <vector>
#include <limits>

namespace Assimp {




class ASSIMP_API SpatialSort {
public:
    SpatialSort();

    
    
    SpatialSort(const aiVector3D *pPositions, unsigned int pNumPositions,
            unsigned int pElementOffset);

    
    ~SpatialSort() = default;

    
    
    void Fill(const aiVector3D *pPositions, unsigned int pNumPositions,
            unsigned int pElementOffset,
            bool pFinalize = true);

    
    
    void Append(const aiVector3D *pPositions, unsigned int pNumPositions,
            unsigned int pElementOffset,
            bool pFinalize = true);

    
    
    void Finalize();

    
    
    void FindPositions(const aiVector3D &pPosition, ai_real pRadius,
            std::vector<unsigned int> &poResults) const;

    
    
    void FindIdenticalPositions(const aiVector3D &pPosition,
            std::vector<unsigned int> &poResults) const;

    
    
    unsigned int GenerateMappingTable(std::vector<unsigned int> &fill,
            ai_real pRadius) const;

protected:
    
    ai_real CalculateDistance(const aiVector3D &pPosition) const;

protected:
    
    aiVector3D mPlaneNormal;

    
    aiVector3D mCentroid;

    
    struct Entry {
        unsigned int mIndex; 
        aiVector3D mPosition; 
        
        ai_real mDistance;

        Entry() AI_NO_EXCEPT
                : mIndex(std::numeric_limits<unsigned int>::max()),
                  mPosition(),
                  mDistance(std::numeric_limits<ai_real>::max()) {
            
        }
        Entry(unsigned int pIndex, const aiVector3D &pPosition) :
                mIndex(pIndex), mPosition(pPosition), mDistance(std::numeric_limits<ai_real>::max()) {
            
        }

        bool operator<(const Entry &e) const { return mDistance < e.mDistance; }
    };

    
    std::vector<Entry> mPositions;

    
    bool mFinalized;
};

} 

#endif 
