


#pragma once
#ifndef AI_STANDARD_SHAPES_H_INC
#define AI_STANDARD_SHAPES_H_INC

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/vector3.h>
#include <stddef.h>
#include <vector>

struct aiMesh;

namespace Assimp {



class ASSIMP_API StandardShapes
{
    
    StandardShapes() {}

public:


    
    
    static aiMesh* MakeMesh(const std::vector<aiVector3D>& positions,
        unsigned int numIndices);


    static aiMesh* MakeMesh ( unsigned int (*GenerateFunc)
        (std::vector<aiVector3D>&));

    static aiMesh* MakeMesh ( unsigned int (*GenerateFunc)
        (std::vector<aiVector3D>&, bool));

    static aiMesh* MakeMesh ( unsigned int n,  void (*GenerateFunc)
        (unsigned int,std::vector<aiVector3D>&));

    
    
    static unsigned int MakeHexahedron(
        std::vector<aiVector3D>& positions,
        bool polygons = false);

    
    
    static unsigned int MakeIcosahedron(
        std::vector<aiVector3D>& positions);


    
    
    static unsigned int MakeDodecahedron(
        std::vector<aiVector3D>& positions,
        bool polygons = false);


    
    
    static unsigned int MakeOctahedron(
        std::vector<aiVector3D>& positions);


    
    
    static unsigned int MakeTetrahedron(
        std::vector<aiVector3D>& positions);



    
    
    static void MakeSphere(unsigned int tess,
        std::vector<aiVector3D>& positions);


    
    
    static void MakeCone(ai_real height,ai_real radius1,
        ai_real radius2,unsigned int tess,
        std::vector<aiVector3D>& positions,bool bOpen= false);


    
    
    static void MakeCircle(ai_real radius, unsigned int tess,
        std::vector<aiVector3D>& positions);

};
} 

#endif 
