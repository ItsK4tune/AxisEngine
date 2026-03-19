


#pragma once
#ifndef AI_ANIM_H_INC
#define AI_ANIM_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#include <assimp/quaternion.h>
#include <assimp/types.h>

#ifdef __cplusplus
extern "C" {
#endif



enum aiAnimInterpolation {
    
    aiAnimInterpolation_Step,

    
    aiAnimInterpolation_Linear,

    
    aiAnimInterpolation_Spherical_Linear,

    
    aiAnimInterpolation_Cubic_Spline,


#ifndef SWIG
    _aiAnimInterpolation_Force32Bit = INT_MAX
#endif
};



struct aiVectorKey {
    
    double mTime;

    
    C_STRUCT aiVector3D mValue;

     
    C_ENUM aiAnimInterpolation mInterpolation;

#ifdef __cplusplus

    
    aiVectorKey() AI_NO_EXCEPT
            : mTime(0.0), mValue(), mInterpolation(aiAnimInterpolation_Linear) {}

    
    aiVectorKey(double time, const aiVector3D &value) :
            mTime(time), mValue(value), mInterpolation(aiAnimInterpolation_Linear){}

    typedef aiVector3D elem_type;

    
    bool operator==(const aiVectorKey &rhs) const {
        return rhs.mValue == this->mValue;
    }

    bool operator!=(const aiVectorKey &rhs) const {
        return rhs.mValue != this->mValue;
    }

    
    bool operator<(const aiVectorKey &rhs) const {
        return mTime < rhs.mTime;
    }

    bool operator>(const aiVectorKey &rhs) const {
        return mTime > rhs.mTime;
    }
#endif 
};



struct aiQuatKey {
    
    double mTime;

    
    C_STRUCT aiQuaternion mValue;

    
    C_ENUM aiAnimInterpolation mInterpolation;

#ifdef __cplusplus
    aiQuatKey() AI_NO_EXCEPT
            : mTime(0.0), mValue(), mInterpolation(aiAnimInterpolation_Linear) {}

    
    aiQuatKey(double time, const aiQuaternion &value) :
            mTime(time), mValue(value), mInterpolation(aiAnimInterpolation_Linear) {}

    typedef aiQuaternion elem_type;

    
    bool operator==(const aiQuatKey &rhs) const {
        return rhs.mValue == this->mValue;
    }

    bool operator!=(const aiQuatKey &rhs) const {
        return rhs.mValue != this->mValue;
    }

    
    bool operator<(const aiQuatKey &rhs) const {
        return mTime < rhs.mTime;
    }

    bool operator>(const aiQuatKey &rhs) const {
        return mTime > rhs.mTime;
    }
#endif
};



struct aiMeshKey {
    
    double mTime;

    
    unsigned int mValue;

#ifdef __cplusplus

    aiMeshKey() AI_NO_EXCEPT
            : mTime(0.0),
              mValue(0) {
    }

    
    aiMeshKey(double time, const unsigned int value) :
            mTime(time), mValue(value) {}

    typedef unsigned int elem_type;

    
    bool operator==(const aiMeshKey &o) const {
        return o.mValue == this->mValue;
    }
    bool operator!=(const aiMeshKey &o) const {
        return o.mValue != this->mValue;
    }

    
    bool operator<(const aiMeshKey &o) const {
        return mTime < o.mTime;
    }
    bool operator>(const aiMeshKey &o) const {
        return mTime > o.mTime;
    }

#endif
};



struct aiMeshMorphKey {
    
    double mTime;

    
    unsigned int *mValues;
    double *mWeights;

    
    unsigned int mNumValuesAndWeights;
#ifdef __cplusplus
    aiMeshMorphKey() AI_NO_EXCEPT
            : mTime(0.0),
              mValues(nullptr),
              mWeights(nullptr),
              mNumValuesAndWeights(0) {
    }

    ~aiMeshMorphKey() {
        if (mNumValuesAndWeights && mValues && mWeights) {
            delete[] mValues;
            delete[] mWeights;
        }
    }
#endif
};



enum aiAnimBehaviour {
    
    aiAnimBehaviour_DEFAULT = 0x0,

    
    aiAnimBehaviour_CONSTANT = 0x1,

    
    aiAnimBehaviour_LINEAR = 0x2,

    
    aiAnimBehaviour_REPEAT = 0x3,


#ifndef SWIG
    _aiAnimBehaviour_Force32Bit = INT_MAX
#endif
};



struct aiNodeAnim {
    
    C_STRUCT aiString mNodeName;

    
    unsigned int mNumPositionKeys;

    
    C_STRUCT aiVectorKey *mPositionKeys;

    
    unsigned int mNumRotationKeys;

    
    C_STRUCT aiQuatKey *mRotationKeys;

    
    unsigned int mNumScalingKeys;

    
    C_STRUCT aiVectorKey *mScalingKeys;

    
    C_ENUM aiAnimBehaviour mPreState;

    
    C_ENUM aiAnimBehaviour mPostState;

#ifdef __cplusplus
    aiNodeAnim() AI_NO_EXCEPT
            : mNumPositionKeys(0),
              mPositionKeys(nullptr),
              mNumRotationKeys(0),
              mRotationKeys(nullptr),
              mNumScalingKeys(0),
              mScalingKeys(nullptr),
              mPreState(aiAnimBehaviour_DEFAULT),
              mPostState(aiAnimBehaviour_DEFAULT) {
        
    }

    ~aiNodeAnim() {
        delete[] mPositionKeys;
        delete[] mRotationKeys;
        delete[] mScalingKeys;
    }
#endif 
};



struct aiMeshAnim {
    
    C_STRUCT aiString mName;

    
    unsigned int mNumKeys;

    
    C_STRUCT aiMeshKey *mKeys;

#ifdef __cplusplus

    aiMeshAnim() AI_NO_EXCEPT
            : mNumKeys(),
              mKeys() {}

    ~aiMeshAnim() {
        delete[] mKeys;
    }

#endif
};



struct aiMeshMorphAnim {
    
    C_STRUCT aiString mName;

    
    unsigned int mNumKeys;

    
    C_STRUCT aiMeshMorphKey *mKeys;

#ifdef __cplusplus

    aiMeshMorphAnim() AI_NO_EXCEPT
            : mNumKeys(),
              mKeys() {}

    ~aiMeshMorphAnim() {
        delete[] mKeys;
    }

#endif
};



struct aiAnimation {
    
    C_STRUCT aiString mName;

    
    double mDuration;

    
    double mTicksPerSecond;

    
    unsigned int mNumChannels;

    
    C_STRUCT aiNodeAnim **mChannels;

    
    unsigned int mNumMeshChannels;

    
    C_STRUCT aiMeshAnim **mMeshChannels;

    
    unsigned int mNumMorphMeshChannels;

    
    C_STRUCT aiMeshMorphAnim **mMorphMeshChannels;

#ifdef __cplusplus
    aiAnimation() AI_NO_EXCEPT
            : mDuration(-1.),
              mTicksPerSecond(0.),
              mNumChannels(0),
              mChannels(nullptr),
              mNumMeshChannels(0),
              mMeshChannels(nullptr),
              mNumMorphMeshChannels(0),
              mMorphMeshChannels(nullptr) {
        
    }

    ~aiAnimation() {
        
        if (mNumChannels && mChannels) {
            for (unsigned int a = 0; a < mNumChannels; a++) {
                delete mChannels[a];
            }

            delete[] mChannels;
        }
        if (mNumMeshChannels && mMeshChannels) {
            for (unsigned int a = 0; a < mNumMeshChannels; a++) {
                delete mMeshChannels[a];
            }

            delete[] mMeshChannels;
        }
        if (mNumMorphMeshChannels && mMorphMeshChannels) {
            for (unsigned int a = 0; a < mNumMorphMeshChannels; a++) {
                delete mMorphMeshChannels[a];
            }

            delete[] mMorphMeshChannels;
        }
    }
#endif 
};

#ifdef __cplusplus
}


namespace Assimp {



template <typename T>
struct Interpolator {
    
    
    void operator()(T &anim_out, const T &a, const T &b, ai_real d) const {
        anim_out = a + (b - a) * d;
    }
}; 



template <>
struct Interpolator<aiQuaternion> {
    void operator()(aiQuaternion &out, const aiQuaternion &a,
            const aiQuaternion &b, ai_real d) const {
        aiQuaternion::Interpolate(out, a, b, d);
    }
}; 

template <>
struct Interpolator<unsigned int> {
    void operator()(unsigned int &out, unsigned int a,
            unsigned int b, ai_real d) const {
        out = d > 0.5f ? b : a;
    }
}; 

template <>
struct Interpolator<aiVectorKey> {
    void operator()(aiVector3D &out, const aiVectorKey &a,
            const aiVectorKey &b, ai_real d) const {
        Interpolator<aiVector3D> ipl;
        ipl(out, a.mValue, b.mValue, d);
    }
}; 

template <>
struct Interpolator<aiQuatKey> {
    void operator()(aiQuaternion &out, const aiQuatKey &a,
            const aiQuatKey &b, ai_real d) const {
        Interpolator<aiQuaternion> ipl;
        ipl(out, a.mValue, b.mValue, d);
    }
}; 

template <>
struct Interpolator<aiMeshKey> {
    void operator()(unsigned int &out, const aiMeshKey &a,
            const aiMeshKey &b, ai_real d) const {
        Interpolator<unsigned int> ipl;
        ipl(out, a.mValue, b.mValue, d);
    }
}; 



} 

#endif 

#endif 
