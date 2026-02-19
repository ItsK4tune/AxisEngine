


#pragma once
#ifndef AI_TYPES_H_INC
#define AI_TYPES_H_INC

#ifdef __GNUC__
#pragma GCC system_header
#endif


#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>


#include <assimp/defs.h>


#include <assimp/vector2.h>
#include <assimp/vector3.h>
#include <assimp/color4.h>
#include <assimp/matrix3x3.h>
#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>

typedef int32_t ai_int32;
typedef uint32_t ai_uint32;

#ifdef __cplusplus

#include <cstring>
#include <new>    
#include <string> 

namespace Assimp {

namespace Intern {



#ifndef SWIG
struct ASSIMP_API AllocateFromAssimpHeap {
    

    
    void *operator new(size_t num_bytes) ;
    void *operator new(size_t num_bytes, const std::nothrow_t &) throw();
    void operator delete(void *data);

    
    void *operator new[](size_t num_bytes) ;
    void *operator new[](size_t num_bytes, const std::nothrow_t &) throw();
    void operator delete[](void *data);

}; 
#endif
} 

} 

extern "C" {
#endif


#ifdef __cplusplus
static const size_t AI_MAXLEN = 1024;
#else
#define AI_MAXLEN 1024
#endif



struct aiPlane {
#ifdef __cplusplus
    aiPlane() AI_NO_EXCEPT : a(0.f), b(0.f), c(0.f), d(0.f) {}
    aiPlane(ai_real _a, ai_real _b, ai_real _c, ai_real _d) :
            a(_a), b(_b), c(_c), d(_d) {}

    aiPlane(const aiPlane &o) :
            a(o.a), b(o.b), c(o.c), d(o.d) {}

#endif 

    
    ai_real a, b, c, d;
}; 



struct aiRay {
#ifdef __cplusplus
    aiRay() AI_NO_EXCEPT {}
    aiRay(const aiVector3D &_pos, const aiVector3D &_dir) :
            pos(_pos), dir(_dir) {}

    aiRay(const aiRay &o) :
            pos(o.pos), dir(o.dir) {}

#endif 

    
    C_STRUCT aiVector3D pos, dir;
}; 



struct aiColor3D {
#ifdef __cplusplus
    aiColor3D() AI_NO_EXCEPT : r(0.0f), g(0.0f), b(0.0f) {}
    aiColor3D(float _r, float _g, float _b) :
            r(_r), g(_g), b(_b) {}
    explicit aiColor3D(float _r) :
            r(_r), g(_r), b(_r) {}
    aiColor3D(const aiColor3D &o) :
            r(o.r), g(o.g), b(o.b) {}

    aiColor3D &operator=(const aiColor3D &o) {
        r = o.r;
        g = o.g;
        b = o.b;
        return *this;
    }

    
    
    bool operator==(const aiColor3D &other) const { return r == other.r && g == other.g && b == other.b; }

    
    
    bool operator!=(const aiColor3D &other) const { return r != other.r || g != other.g || b != other.b; }

    
    
    bool operator<(const aiColor3D &other) const {
        return r < other.r || (r == other.r && (g < other.g || (g == other.g && b < other.b)));
    }

    
    aiColor3D operator+(const aiColor3D &c) const {
        return aiColor3D(r + c.r, g + c.g, b + c.b);
    }

    
    aiColor3D operator-(const aiColor3D &c) const {
        return aiColor3D(r - c.r, g - c.g, b - c.b);
    }

    
    aiColor3D operator*(const aiColor3D &c) const {
        return aiColor3D(r * c.r, g * c.g, b * c.b);
    }

    
    aiColor3D operator*(ai_real f) const {
        return aiColor3D(r * f, g * f, b * f);
    }

    
    float operator[](unsigned int i) const {
        return *(&r + i);
    }

    
    float &operator[](unsigned int i) {
        if (0 == i) {
            return r;
        } else if (1 == i) {
            return g;
        } else if (2 == i) {
            return b;
        }
        return r;
    }

    
    bool IsBlack() const {
        static const float epsilon = float(10e-3);
        return std::fabs(r) < epsilon && std::fabs(g) < epsilon && std::fabs(b) < epsilon;
    }

#endif 

    
    float r, g, b;
}; 



struct aiString {
#ifdef __cplusplus
    
    aiString() AI_NO_EXCEPT :
            length(0), data{'\0'} {
#ifdef ASSIMP_BUILD_DEBUG
        
        memset(data + 1, 27, AI_MAXLEN - 1);
#endif
    }

    
    aiString(const aiString &rOther) :
            length(rOther.length), data{'\0'} {
        
        length = length >= AI_MAXLEN ? AI_MAXLEN - 1 : length;
        memcpy(data, rOther.data, length);
        data[length] = '\0';
    }

    
    explicit aiString(const std::string &pString) :
            length((ai_uint32)pString.length()), data{'\0'} {
        length = length >= AI_MAXLEN ? AI_MAXLEN - 1 : length;
        memcpy(data, pString.c_str(), length);
        data[length] = '\0';
    }

    
    void Set(const std::string &pString) {
        if (pString.length() > AI_MAXLEN - 1) {
            return;
        }
        length = (ai_uint32)pString.length();
        memcpy(data, pString.c_str(), length);
        data[length] = 0;
    }

    
    void Set(const char *sz, size_t maxlen) {
        if (sz == nullptr) {
            return;
        }
        size_t len = 0;
        for (size_t i=0; i<maxlen; ++i) {
            if (sz[i] == '\0') {
                break;
            }
            ++len;
        }
        if (len > AI_MAXLEN - 1) {
            len = AI_MAXLEN - 1;
        }
        length = static_cast<uint32_t>(len);
        memcpy(data, sz, len);
        data[len] = 0;
    }

    
    aiString &operator=(const aiString &rOther) {
        if (this == &rOther) {
            return *this;
        }

        length = rOther.length;
        if (length > (AI_MAXLEN - 1)) {
            length = static_cast<ai_int32>(AI_MAXLEN - 1);
        }

        memcpy(data, rOther.data, length);
        data[length] = '\0';
        return *this;
    }

    
    aiString &operator=(const char *sz) {
        Set(sz);
        return *this;
    }

    
    aiString &operator=(const std::string &pString) {
        Set(pString);
        return *this;
    }

    
    bool operator==(const aiString &other) const {
        if (length == other.length) {
            return memcmp(data, other.data, length) == 0;
        }
        return false;
    }

    
    bool operator!=(const aiString &other) const {
        return !(*this == other);
    }

    
    void Append(const char *app) {
        const ai_uint32 len = static_cast<ai_uint32>(::strlen(app));
        if (!len) {
            return;
        }
        if (length + len >= AI_MAXLEN) {
            return;
        }

        memcpy(&data[length], app, len + 1);
        length += len;
    }

    
    void Clear() {
        length = 0;
        data[0] = '\0';

#ifdef ASSIMP_BUILD_DEBUG
        
        memset(data + 1, 27, AI_MAXLEN - 1);
#endif
    }

    
    const char *C_Str() const {
        return data;
    }

    
    bool Empty() const {
        return length == 0;
    }

#endif 

    
    ai_uint32 length;

    
    char data[AI_MAXLEN];
}; 



typedef enum aiReturn {
    
    aiReturn_SUCCESS = 0x0,

    
    aiReturn_FAILURE = -0x1,

    
    aiReturn_OUTOFMEMORY = -0x3,

    
    _AI_ENFORCE_ENUM_SIZE = 0x7fffffff

    
} aiReturn; 


#define AI_SUCCESS aiReturn_SUCCESS
#define AI_FAILURE aiReturn_FAILURE
#define AI_OUTOFMEMORY aiReturn_OUTOFMEMORY



enum aiOrigin {
    
    aiOrigin_SET = 0x0,

    
    aiOrigin_CUR = 0x1,

    
    aiOrigin_END = 0x2,

    
    _AI_ORIGIN_ENFORCE_ENUM_SIZE = 0x7fffffff

    
}; 



enum aiDefaultLogStream {
    
    aiDefaultLogStream_FILE = 0x1,

    
    aiDefaultLogStream_STDOUT = 0x2,

    
    aiDefaultLogStream_STDERR = 0x4,

    
    aiDefaultLogStream_DEBUGGER = 0x8,

    
    _AI_DLS_ENFORCE_ENUM_SIZE = 0x7fffffff
    
}; 


#define DLS_FILE aiDefaultLogStream_FILE
#define DLS_STDOUT aiDefaultLogStream_STDOUT
#define DLS_STDERR aiDefaultLogStream_STDERR
#define DLS_DEBUGGER aiDefaultLogStream_DEBUGGER



struct aiMemoryInfo {
#ifdef __cplusplus

    
    aiMemoryInfo() AI_NO_EXCEPT
            : textures(0),
              materials(0),
              meshes(0),
              nodes(0),
              animations(0),
              cameras(0),
              lights(0),
              total(0) {}

#endif

    
    unsigned int textures;

    
    unsigned int materials;

    
    unsigned int meshes;

    
    unsigned int nodes;

    
    unsigned int animations;

    
    unsigned int cameras;

    
    unsigned int lights;

    
    unsigned int total;
}; 


struct aiBuffer {
    const char *data; 
    const char *end;  

#ifdef __cplusplus
    
    aiBuffer() :
            data(nullptr), end(nullptr) {}

    
    ~aiBuffer() = default;
#endif 
};

#ifdef __cplusplus
}
#endif 


#include "vector2.inl"
#include "vector3.inl"
#include "color4.inl"
#include "matrix3x3.inl"
#include "matrix4x4.inl"
#include "quaternion.inl"

#endif 
