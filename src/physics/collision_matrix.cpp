#include <iostream>
#include <physics/collision_matrix.h>

CollisionMatrix& CollisionMatrix::Instance()
{
    static CollisionMatrix instance;
    return instance;
}

void CollisionMatrix::IgnoreTagCollision(const std::string& tag1, const std::string& tag2)
{
    m_IgnoredTags.insert({tag1, tag2});
}

void CollisionMatrix::IgnoreNameCollision(const std::string& name1, const std::string& name2)
{
    m_IgnoredNames.insert({name1, name2});
}

void CollisionMatrix::Reset()
{
    m_IgnoredTags.clear();
    m_IgnoredNames.clear();
}

bool CollisionMatrix::CanCollide(const std::string& tag1, const std::string& tag2, const std::string& name1, const std::string& name2) const
{
    std::cout << "[DEBUG] CanCollide: tags(" << tag1 << ", " << tag2 << ") names(" << name1 << ", " << name2 << ")" << std::endl;
    if (m_IgnoredTags.find({tag1, tag2}) != m_IgnoredTags.end())
        return false;
        
    if (m_IgnoredNames.find({name1, name2}) != m_IgnoredNames.end())
        return false;

    return true;
}
