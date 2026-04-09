#pragma once

#include <string>

enum class ShapeType
{
    Box,
    Sphere,
    Capsule,
    Cylinder,
    Mesh,
    Heightfield,
    Compound
};

inline ShapeType ShapeTypeFromString(const std::string& str)
{
    if (str == "BOX" || str == "Box")              return ShapeType::Box;
    if (str == "SPHERE" || str == "Sphere")        return ShapeType::Sphere;
    if (str == "CAPSULE" || str == "Capsule")      return ShapeType::Capsule;
    if (str == "CYLINDER" || str == "Cylinder")    return ShapeType::Cylinder;
    if (str == "MESH" || str == "Mesh")            return ShapeType::Mesh;
    if (str == "HEIGHTFIELD" || str == "Heightfield") return ShapeType::Heightfield;
    if (str == "COMPOUND" || str == "Compound")    return ShapeType::Compound;
    return ShapeType::Box;
}

inline const char* ShapeTypeToString(ShapeType type)
{
    switch (type)
    {
        case ShapeType::Box:         return "BOX";
        case ShapeType::Sphere:      return "SPHERE";
        case ShapeType::Capsule:     return "CAPSULE";
        case ShapeType::Cylinder:    return "CYLINDER";
        case ShapeType::Mesh:        return "MESH";
        case ShapeType::Heightfield: return "HEIGHTFIELD";
        case ShapeType::Compound:    return "COMPOUND";
    }
    return "BOX";
}
