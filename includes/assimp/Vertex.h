

#pragma once
#ifndef AI_VERTEX_H_INC
#define AI_VERTEX_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/vector3.h>
#include <assimp/mesh.h>
#include <assimp/ai_assert.h>

#include <functional>

namespace Assimp {

    
    
    
    
    
    namespace Intern {
        template <typename T0, typename T1, typename TRES = T0> struct plus {
            TRES operator() (const T0& t0, const T1& t1) const {
                return t0+t1;
            }
        };
        template <typename T0, typename T1, typename TRES = T0> struct minus {
            TRES operator() (const T0& t0, const T1& t1) const {
                return t0-t1;
            }
        };
        template <typename T0, typename T1, typename TRES = T0> struct multiplies {
            TRES operator() (const T0& t0, const T1& t1) const {
                return t0*t1;
            }
        };
        template <typename T0, typename T1, typename TRES = T0> struct divides {
            TRES operator() (const T0& t0, const T1& t1) const {
                return t0/t1;
            }
        };
    }




struct Vertex {
    friend Vertex operator + (const Vertex&,const Vertex&);
    friend Vertex operator - (const Vertex&,const Vertex&);
    friend Vertex operator * (const Vertex&,ai_real);
    friend Vertex operator / (const Vertex&,ai_real);
    friend Vertex operator * (ai_real, const Vertex&);

    aiVector3D position;
    aiVector3D normal;
    aiVector3D tangent, bitangent;

    aiVector3D texcoords[AI_MAX_NUMBER_OF_TEXTURECOORDS];
    aiColor4D colors[AI_MAX_NUMBER_OF_COLOR_SETS];

    Vertex() = default;
    ~Vertex() = default;

    
    
    explicit Vertex(const aiMesh* msh, unsigned int idx) {
        ai_assert(idx < msh->mNumVertices);
        position = msh->mVertices[idx];

        if (msh->HasNormals()) {
            normal = msh->mNormals[idx];
        }

        if (msh->HasTangentsAndBitangents()) {
            tangent = msh->mTangents[idx];
            bitangent = msh->mBitangents[idx];
        }

        for (unsigned int i = 0; msh->HasTextureCoords(i); ++i) {
            texcoords[i] = msh->mTextureCoords[i][idx];
        }

        for (unsigned int i = 0; msh->HasVertexColors(i); ++i) {
            colors[i] = msh->mColors[i][idx];
        }
    }

    
    
    explicit Vertex(const aiAnimMesh* msh, unsigned int idx) {
        ai_assert(idx < msh->mNumVertices);
        if (msh->HasPositions()) {
            position = msh->mVertices[idx];
        }

        if (msh->HasNormals()) {
            normal = msh->mNormals[idx];
        }

        if (msh->HasTangentsAndBitangents()) {
            tangent = msh->mTangents[idx];
            bitangent = msh->mBitangents[idx];
        }

        for (unsigned int i = 0; msh->HasTextureCoords(i); ++i) {
            texcoords[i] = msh->mTextureCoords[i][idx];
        }

        for (unsigned int i = 0; msh->HasVertexColors(i); ++i) {
           colors[i] = msh->mColors[i][idx];
        }
    }

    Vertex& operator += (const Vertex& v) {
        *this = *this+v;
        return *this;
    }

    Vertex& operator -= (const Vertex& v) {
        *this = *this-v;
        return *this;
    }

    Vertex& operator *= (ai_real v) {
        *this = *this*v;
        return *this;
    }

    Vertex& operator /= (ai_real v) {
        *this = *this/v;
        return *this;
    }

    bool operator < (const Vertex & o) const {
        if (position < o.position) return true;
        if (position != o.position) return false;

        if (normal < o.normal) return true;
        if (normal != o.normal) return false;

        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i ++) {
          if (texcoords[i] < o.texcoords[i]) return true;
          if (texcoords[i] != o.texcoords[i]) return false;
        }

        

        for (uint32_t i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; i ++) {
          if (colors[i] < o.colors[i]) return true;
          if (colors[i] != o.colors[i]) return false;
        }

        
        return false;
    }

    
    
    void SortBack(aiMesh* out, unsigned int idx) const {
        ai_assert(idx<out->mNumVertices);
        out->mVertices[idx] = position;

        if (out->HasNormals()) {
            out->mNormals[idx] = normal;
        }

        if (out->HasTangentsAndBitangents()) {
            out->mTangents[idx] = tangent;
            out->mBitangents[idx] = bitangent;
        }

        for(unsigned int i = 0; out->HasTextureCoords(i); ++i) {
            out->mTextureCoords[i][idx] = texcoords[i];
        }

        for(unsigned int i = 0; out->HasVertexColors(i); ++i) {
            out->mColors[i][idx] = colors[i];
        }
    }

private:

    
    
    template <template <typename t> class op> static Vertex BinaryOp(const Vertex& v0, const Vertex& v1) {
        

        Vertex res;
        res.position  = op<aiVector3D>()(v0.position,v1.position);
        res.normal    = op<aiVector3D>()(v0.normal,v1.normal);
        res.tangent   = op<aiVector3D>()(v0.tangent,v1.tangent);
        res.bitangent = op<aiVector3D>()(v0.bitangent,v1.bitangent);

        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            res.texcoords[i] = op<aiVector3D>()(v0.texcoords[i],v1.texcoords[i]);
        }
        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
            res.colors[i] = op<aiColor4D>()(v0.colors[i],v1.colors[i]);
        }
        return res;
    }

    
    
    template <template <typename, typename, typename> class op>
    static Vertex BinaryOp(const Vertex& v0, ai_real f) {
        

        Vertex res;
        res.position  = op<aiVector3D,ai_real,aiVector3D>()(v0.position,f);
        res.normal    = op<aiVector3D,ai_real,aiVector3D>()(v0.normal,f);
        res.tangent   = op<aiVector3D,ai_real,aiVector3D>()(v0.tangent,f);
        res.bitangent = op<aiVector3D,ai_real,aiVector3D>()(v0.bitangent,f);

        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            res.texcoords[i] = op<aiVector3D,ai_real,aiVector3D>()(v0.texcoords[i],f);
        }
        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
            res.colors[i] = op<aiColor4D,float, aiColor4D>()(v0.colors[i],f);
        }
        return res;
    }

    
    
    template <template <typename, typename, typename> class op>
    static Vertex BinaryOp(ai_real f, const Vertex& v0) {
        

        Vertex res;
        res.position  = op<ai_real,aiVector3D,aiVector3D>()(f,v0.position);
        res.normal    = op<ai_real,aiVector3D,aiVector3D>()(f,v0.normal);
        res.tangent   = op<ai_real,aiVector3D,aiVector3D>()(f,v0.tangent);
        res.bitangent = op<ai_real,aiVector3D,aiVector3D>()(f,v0.bitangent);

        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            res.texcoords[i] = op<ai_real,aiVector3D,aiVector3D>()(f,v0.texcoords[i]);
        }
        for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
            res.colors[i] = op<float, aiColor4D,aiColor4D>()(f,v0.colors[i]);
        }
        return res;
    }
};


AI_FORCE_INLINE Vertex operator + (const Vertex& v0,const Vertex& v1) {
    return Vertex::BinaryOp<std::plus>(v0,v1);
}

AI_FORCE_INLINE Vertex operator - (const Vertex& v0,const Vertex& v1) {
    return Vertex::BinaryOp<std::minus>(v0,v1);
}

AI_FORCE_INLINE Vertex operator * (const Vertex& v0,ai_real f) {
    return Vertex::BinaryOp<Intern::multiplies>(v0,f);
}

AI_FORCE_INLINE Vertex operator / (const Vertex& v0,ai_real f) {
    return Vertex::BinaryOp<Intern::multiplies>(v0,1.f/f);
}

AI_FORCE_INLINE Vertex operator * (ai_real f,const Vertex& v0) {
    return Vertex::BinaryOp<Intern::multiplies>(f,v0);
}

}

#endif 