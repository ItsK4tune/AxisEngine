


#pragma once
#ifndef AI_METADATA_H_INC
#define AI_METADATA_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1500)
#include "Compiler/pstdint.h"
#else
#include <stdint.h>
#endif

#include <assimp/quaternion.h>




typedef enum aiMetadataType {
    AI_BOOL = 0,
    AI_INT32 = 1,
    AI_UINT64 = 2,
    AI_FLOAT = 3,
    AI_DOUBLE = 4,
    AI_AISTRING = 5,
    AI_AIVECTOR3D = 6,
    AI_AIMETADATA = 7,
    AI_INT64 = 8,
    AI_UINT32 = 9,
    AI_META_MAX = 10,

#ifndef SWIG
    FORCE_32BIT = INT_MAX
#endif
} aiMetadataType;




struct aiMetadataEntry {
    aiMetadataType mType;
    void *mData;

#ifdef __cplusplus
    aiMetadataEntry() :
            mType(AI_META_MAX),
            mData( nullptr ) {
        
    }
#endif
};

#ifdef __cplusplus

#include <string>

struct aiMetadata;





inline aiMetadataType GetAiType(const bool &) {
    return AI_BOOL;
}
inline aiMetadataType GetAiType(int32_t) {
    return AI_INT32;
}
inline aiMetadataType GetAiType(const uint64_t &) {
    return AI_UINT64;
}
inline aiMetadataType GetAiType(const float &) {
    return AI_FLOAT;
}
inline aiMetadataType GetAiType(const double &) {
    return AI_DOUBLE;
}
inline aiMetadataType GetAiType(const aiString &) {
    return AI_AISTRING;
}
inline aiMetadataType GetAiType(const aiVector3D &) {
    return AI_AIVECTOR3D;
}
inline aiMetadataType GetAiType(const aiMetadata &) {
    return AI_AIMETADATA;
}
inline aiMetadataType GetAiType(const int64_t &) {
    return AI_INT64;
}
inline aiMetadataType GetAiType(const uint32_t &) {
    return AI_UINT32;
}

#endif 




struct aiMetadata {
    
    unsigned int mNumProperties;

    
    C_STRUCT aiString *mKeys;

    
    C_STRUCT aiMetadataEntry *mValues;

#ifdef __cplusplus

    
    aiMetadata() AI_NO_EXCEPT
            : mNumProperties(0),
              mKeys(nullptr),
              mValues(nullptr) {
        
    }

    aiMetadata(const aiMetadata &rhs) :
            mNumProperties(rhs.mNumProperties), mKeys(nullptr), mValues(nullptr) {
        mKeys = new aiString[mNumProperties];
        for (size_t i = 0; i < static_cast<size_t>(mNumProperties); ++i) {
            mKeys[i] = rhs.mKeys[i];
        }
        mValues = new aiMetadataEntry[mNumProperties];
        for (size_t i = 0; i < static_cast<size_t>(mNumProperties); ++i) {
            mValues[i].mType = rhs.mValues[i].mType;
            switch (rhs.mValues[i].mType) {
            case AI_BOOL:
                mValues[i].mData = new bool;
                ::memcpy(mValues[i].mData, rhs.mValues[i].mData, sizeof(bool));
                break;
            case AI_INT32: {
                int32_t v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(int32_t));
                mValues[i].mData = new int32_t(v);
            } break;
            case AI_UINT64: {
                uint64_t v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(uint64_t));
                mValues[i].mData = new uint64_t(v);
            } break;
            case AI_FLOAT: {
                float v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(float));
                mValues[i].mData = new float(v);
            } break;
            case AI_DOUBLE: {
                double v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(double));
                mValues[i].mData = new double(v);
            } break;
            case AI_AISTRING: {
                aiString v;
                rhs.Get<aiString>(static_cast<unsigned int>(i), v);
                mValues[i].mData = new aiString(v);
            } break;
            case AI_AIVECTOR3D: {
                aiVector3D v;
                rhs.Get<aiVector3D>(static_cast<unsigned int>(i), v);
                mValues[i].mData = new aiVector3D(v);
            } break;
            case AI_AIMETADATA: {
                aiMetadata v;
                rhs.Get<aiMetadata>(static_cast<unsigned int>(i), v);
                mValues[i].mData = new aiMetadata(v);
            } break;
            case AI_INT64: {
                int64_t v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(int64_t));
                mValues[i].mData = new int64_t(v);
            } break;
            case AI_UINT32: {
                uint32_t v;
                ::memcpy(&v, rhs.mValues[i].mData, sizeof(uint32_t));
                mValues[i].mData = new uint32_t(v);
            } break;
#ifndef SWIG
            case FORCE_32BIT:
#endif
            default:
                break;
            }
        }
    }

    aiMetadata &operator=(aiMetadata rhs) {
        using std::swap;
        swap(mNumProperties, rhs.mNumProperties);
        swap(mKeys, rhs.mKeys);
        swap(mValues, rhs.mValues);
        return *this;
    }

    
    ~aiMetadata() {
        delete[] mKeys;
        mKeys = nullptr;
        if (mValues) {
            
            for (unsigned i = 0; i < mNumProperties; ++i) {
                void *data = mValues[i].mData;
                switch (mValues[i].mType) {
                case AI_BOOL:
                    delete static_cast<bool *>(data);
                    break;
                case AI_INT32:
                    delete static_cast<int32_t *>(data);
                    break;
                case AI_UINT64:
                    delete static_cast<uint64_t *>(data);
                    break;
                case AI_FLOAT:
                    delete static_cast<float *>(data);
                    break;
                case AI_DOUBLE:
                    delete static_cast<double *>(data);
                    break;
                case AI_AISTRING:
                    delete static_cast<aiString *>(data);
                    break;
                case AI_AIVECTOR3D:
                    delete static_cast<aiVector3D *>(data);
                    break;
                case AI_AIMETADATA:
                    delete static_cast<aiMetadata *>(data);
                    break;
                case AI_INT64:
                    delete static_cast<int64_t *>(data);
                    break;
                case AI_UINT32:
                    delete static_cast<uint32_t *>(data);
                    break;
#ifndef SWIG
                case FORCE_32BIT:
#endif
                default:
                    break;
                }
            }

            
            delete[] mValues;
            mValues = nullptr;
        }
    }

    
    static inline aiMetadata *Alloc(unsigned int numProperties) {
        if (0 == numProperties) {
            return nullptr;
        }

        aiMetadata *data = new aiMetadata;
        data->mNumProperties = numProperties;
        data->mKeys = new aiString[data->mNumProperties]();
        data->mValues = new aiMetadataEntry[data->mNumProperties]();

        return data;
    }

    
    static inline void Dealloc(aiMetadata *metadata) {
        delete metadata;
    }

    template <typename T>
    inline void Add(const std::string &key, const T &value) {
        aiString *new_keys = new aiString[mNumProperties + 1];
        aiMetadataEntry *new_values = new aiMetadataEntry[mNumProperties + 1];

        for (unsigned int i = 0; i < mNumProperties; ++i) {
            new_keys[i] = mKeys[i];
            new_values[i] = mValues[i];
        }

        delete[] mKeys;
        delete[] mValues;

        mKeys = new_keys;
        mValues = new_values;

        mNumProperties++;

        Set(mNumProperties - 1, key, value);
    }

    template <typename T>
    inline bool Set(unsigned index, const std::string &key, const T &value) {
        
        if (index >= mNumProperties) {
            return false;
        }

        
        if (key.empty()) {
            return false;
        }

        
        mKeys[index] = key;

        
        mValues[index].mType = GetAiType(value);

        
        if (nullptr != mValues[index].mData && AI_AIMETADATA != mValues[index].mType) {
            ::memcpy(mValues[index].mData, &value, sizeof(T));
        } else if (nullptr != mValues[index].mData && AI_AIMETADATA == mValues[index].mType) {
            *static_cast<T *>(mValues[index].mData) = value;
        } else {
            if (nullptr != mValues[index].mData) {
                delete static_cast<T *>(mValues[index].mData);
                mValues[index].mData = nullptr;
            }
            mValues[index].mData = new T(value);
        }

        return true;
    }

    template <typename T>
    inline bool Set(const std::string &key, const T &value) {
        if (key.empty()) {
            return false;
        }

        bool result = false;
        for (unsigned int i = 0; i < mNumProperties; ++i) {
            if (key == mKeys[i].C_Str()) {
                Set(i, key, value);
                result = true;
                break;
            }
        }

        return result;
    }

    template <typename T>
    inline bool Get(unsigned index, T &value) const {
        
        if (index >= mNumProperties) {
            return false;
        }

        
        
        if (GetAiType(value) != mValues[index].mType) {
            return false;
        }

        
        
        value = *static_cast<T *>(mValues[index].mData);

        return true;
    }

    template <typename T>
    inline bool Get(const aiString &key, T &value) const {
        
        for (unsigned int i = 0; i < mNumProperties; ++i) {
            if (mKeys[i] == key) {
                return Get(i, value);
            }
        }
        return false;
    }

    template <typename T>
    inline bool Get(const std::string &key, T &value) const {
        return Get(aiString(key), value);
    }

    
    
    
    
    
    inline bool Get(size_t index, const aiString *&key, const aiMetadataEntry *&entry) const {
        if (index >= mNumProperties) {
            return false;
        }

        key = &mKeys[index];
        entry = &mValues[index];

        return true;
    }

    
    
    inline bool HasKey(const char *key) const {
        if (nullptr == key) {
            return false;
        }

        
        for (unsigned int i = 0; i < mNumProperties; ++i) {
            if (0 == strncmp(mKeys[i].C_Str(), key, mKeys[i].length)) {
                return true;
            }
        }
        return false;
    }

    friend bool CompareKeys(const aiMetadata &lhs, const aiMetadata &rhs) {
        if (lhs.mNumProperties != rhs.mNumProperties) {
            return false;
        }

        for (unsigned int i = 0; i < lhs.mNumProperties; ++i) {
            if (lhs.mKeys[i] != rhs.mKeys[i]) {
                return false;
            }
        }
        return true;
    }

    friend bool CompareValues(const aiMetadata &lhs, const aiMetadata &rhs) {
        if (lhs.mNumProperties != rhs.mNumProperties) {
            return false;
        }

        for (unsigned int i = 0; i < lhs.mNumProperties; ++i) {
            if (lhs.mValues[i].mType != rhs.mValues[i].mType) {
                return false;
            }

            switch (lhs.mValues[i].mType) {
            case AI_BOOL: {
                if (*static_cast<bool *>(lhs.mValues[i].mData) != *static_cast<bool *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_INT32: {
                if (*static_cast<int32_t *>(lhs.mValues[i].mData) != *static_cast<int32_t *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_UINT64: {
                if (*static_cast<uint64_t *>(lhs.mValues[i].mData) != *static_cast<uint64_t *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_FLOAT: {
                if (*static_cast<float *>(lhs.mValues[i].mData) != *static_cast<float *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_DOUBLE: {
                if (*static_cast<double *>(lhs.mValues[i].mData) != *static_cast<double *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_AISTRING: {
                if (*static_cast<aiString *>(lhs.mValues[i].mData) != *static_cast<aiString *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_AIVECTOR3D: {
                if (*static_cast<aiVector3D *>(lhs.mValues[i].mData) != *static_cast<aiVector3D *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_AIMETADATA: {
                if (*static_cast<aiMetadata *>(lhs.mValues[i].mData) != *static_cast<aiMetadata *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_INT64: {
                if (*static_cast<int64_t *>(lhs.mValues[i].mData) != *static_cast<int64_t *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
            case AI_UINT32: {
                if (*static_cast<uint32_t *>(lhs.mValues[i].mData) != *static_cast<uint32_t *>(rhs.mValues[i].mData)) {
                    return false;
                }
            } break;
#ifndef SWIG
            case FORCE_32BIT:
#endif
            default:
                break;
            }
        }

        return true;
    }

    friend bool operator==(const aiMetadata &lhs, const aiMetadata &rhs) {
        return CompareKeys(lhs, rhs) && CompareValues(lhs, rhs);
    }

    friend bool operator!=(const aiMetadata &lhs, const aiMetadata &rhs) {
        return !(lhs == rhs);
    }

#endif 
};

#endif 
