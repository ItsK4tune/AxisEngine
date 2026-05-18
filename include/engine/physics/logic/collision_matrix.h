#pragma once

#include <string>
#include <unordered_set>
#include <utility>

struct StringPairHash
{
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const
    {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ hash2;
    }
};

struct StringPairEqual
{
    template <class T1, class T2>
    bool operator()(const std::pair<T1, T2>& lhs, const std::pair<T1, T2>& rhs) const
    {
        return (lhs.first == rhs.first && lhs.second == rhs.second) ||
               (lhs.first == rhs.second && lhs.second == rhs.first);
    }
};

class CollisionMatrix
{
public:
    CollisionMatrix() = default;

    void IgnoreTagCollision(const std::string& tag1, const std::string& tag2);
    void IgnoreNameCollision(const std::string& name1, const std::string& name2);

    void Reset();

    bool CanCollide(const std::string& tag1, const std::string& tag2, const std::string& name1,
                    const std::string& name2) const;

private:
    std::unordered_set<std::pair<std::string, std::string>, StringPairHash, StringPairEqual> m_IgnoredTags;
    std::unordered_set<std::pair<std::string, std::string>, StringPairHash, StringPairEqual> m_IgnoredNames;
};
