


#pragma once
#ifndef INCLUDED_AI_CREATE_ANIM_MESH_H
#define INCLUDED_AI_CREATE_ANIM_MESH_H

#ifdef __GNUC__
#   pragma GCC system_header
#endif

#include <assimp/mesh.h>

namespace Assimp {


ASSIMP_API aiAnimMesh *aiCreateAnimMesh(const aiMesh *mesh,
                                        bool needPositions = true,
                                        bool needNormals = true,
                                        bool needTangents = true,
                                        bool needColors = true,
                                        bool needTexCoords = true);

} 

#endif 

