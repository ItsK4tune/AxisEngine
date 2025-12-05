#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <btBulletDynamicsCommon.h>

class BulletGLMHelpers {
public:
    static btVector3 convert(const glm::vec3& v) { return btVector3(v.x, v.y, v.z); }
    static glm::vec3 convert(const btVector3& v) { return glm::vec3(v.x(), v.y(), v.z()); }
    
    static btQuaternion convert(const glm::quat& q) { return btQuaternion(q.x, q.y, q.z, q.w); }
    static glm::quat convert(const btQuaternion& q) { return glm::quat(q.w(), q.x(), q.y(), q.z()); }

    static glm::mat4 convert(const btTransform& t) {
        glm::mat4 m;
        t.getOpenGLMatrix(&m[0][0]);
        return m;
    }
};