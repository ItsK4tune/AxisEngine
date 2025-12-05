#include "engine/entity.h"

#include <algorithm>
#include <limits>
#include <glm/gtc/type_ptr.hpp>

glm::mat4 Transform::getLocalModelMatrix()
{
    const glm::mat4 rotX = glm::rotate(glm::mat4(1.f), glm::radians(m_eulerRot.x), glm::vec3(1,0,0));
    const glm::mat4 rotY = glm::rotate(glm::mat4(1.f), glm::radians(m_eulerRot.y), glm::vec3(0,1,0));
    const glm::mat4 rotZ = glm::rotate(glm::mat4(1.f), glm::radians(m_eulerRot.z), glm::vec3(0,0,1));

    glm::mat4 rotation = rotY * rotX * rotZ;
    glm::mat4 trs = glm::translate(glm::mat4(1.f), m_pos) * rotation * glm::scale(glm::mat4(1.f), m_scale);
    return trs;
}

void Transform::computeModelMatrix()
{
    m_modelMatrix = getLocalModelMatrix();
    m_isDirty = false;
}

void Transform::computeModelMatrix(const glm::mat4& parentGlobalModelMatrix)
{
    m_modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
    m_isDirty = false;
}

void Transform::setLocalPosition(const glm::vec3& p)
{
    m_pos = p;
    m_isDirty = true;
}

void Transform::setLocalRotation(const glm::vec3& r)
{
    m_eulerRot = r;
    m_isDirty = true;
}

void Transform::setLocalScale(const glm::vec3& s)
{
    m_scale = s;
    m_isDirty = true;
}

const glm::vec3& Transform::getGlobalPosition() const
{
    return m_modelMatrix[3];
}

const glm::vec3& Transform::getLocalPosition() const { return m_pos; }
const glm::vec3& Transform::getLocalRotation() const { return m_eulerRot; }
const glm::vec3& Transform::getLocalScale() const { return m_scale; }
const glm::mat4& Transform::getModelMatrix() const { return m_modelMatrix; }

glm::vec3 Transform::getRight() const { return glm::vec3(m_modelMatrix[0]); }
glm::vec3 Transform::getUp() const { return glm::vec3(m_modelMatrix[1]); }
glm::vec3 Transform::getBackward() const { return glm::vec3(m_modelMatrix[2]); }
glm::vec3 Transform::getForward() const { return -glm::vec3(m_modelMatrix[2]); }

glm::vec3 Transform::getGlobalScale() const
{
    return glm::vec3(
        glm::length(getRight()),
        glm::length(getUp()),
        glm::length(getBackward())
    );
}

bool Transform::isDirty() const { return m_isDirty; }

Plane::Plane() {}

Plane::Plane(const glm::vec3& p1, const glm::vec3& norm)
{
    normal = glm::normalize(norm);
    distance = glm::dot(normal, p1);
}

float Plane::getSignedDistanceToPlane(const glm::vec3& point) const
{
    return glm::dot(normal, point) - distance;
}

Sphere::Sphere(const glm::vec3& c, float r)
    : center(c), radius(r)
{}

bool Sphere::isOnOrForwardPlane(const Plane& p) const
{
    return p.getSignedDistanceToPlane(center) > -radius;
}

bool Sphere::isOnFrustum(const Frustum& f, const Transform& tr) const
{
    glm::vec3 scale = tr.getGlobalScale();
    glm::vec3 globalCenter = glm::vec3(tr.getModelMatrix() * glm::vec4(center, 1));
    float maxScale = (std::max)({ scale.x, scale.y, scale.z });

    Sphere s(globalCenter, radius * (maxScale * 0.5f));

    return s.isOnOrForwardPlane(f.leftFace) &&
           s.isOnOrForwardPlane(f.rightFace) &&
           s.isOnOrForwardPlane(f.topFace) &&
           s.isOnOrForwardPlane(f.bottomFace) &&
           s.isOnOrForwardPlane(f.nearFace) &&
           s.isOnOrForwardPlane(f.farFace);
}

SquareAABB::SquareAABB(const glm::vec3& c, float e)
    : center(c), extent(e)
{}

bool SquareAABB::isOnOrForwardPlane(const Plane& plane) const
{
    float r = extent * (std::abs(plane.normal.x) + std::abs(plane.normal.y) + std::abs(plane.normal.z));
    return -r <= plane.getSignedDistanceToPlane(center);
}

bool SquareAABB::isOnFrustum(const Frustum& f, const Transform& t) const
{
    glm::vec3 globalCenter = glm::vec3(t.getModelMatrix() * glm::vec4(center, 1));

    glm::vec3 r = t.getRight() * extent;
    glm::vec3 u = t.getUp() * extent;
    glm::vec3 fwd = t.getForward() * extent;

    float i = std::abs(glm::dot(glm::vec3(1,0,0), r))
        + std::abs(glm::dot(glm::vec3(1,0,0), u))
        + std::abs(glm::dot(glm::vec3(1,0,0), fwd));

    float j = std::abs(glm::dot(glm::vec3(0,1,0), r))
        + std::abs(glm::dot(glm::vec3(0,1,0), u))
        + std::abs(glm::dot(glm::vec3(0,1,0), fwd));

    float k = std::abs(glm::dot(glm::vec3(0,0,1), r))
        + std::abs(glm::dot(glm::vec3(0,0,1), u))
        + std::abs(glm::dot(glm::vec3(0,0,1), fwd));

    SquareAABB g(globalCenter, (std::max)({ i, j, k }));

    return g.isOnOrForwardPlane(f.leftFace)
        && g.isOnOrForwardPlane(f.rightFace)
        && g.isOnOrForwardPlane(f.topFace)
        && g.isOnOrForwardPlane(f.bottomFace)
        && g.isOnOrForwardPlane(f.nearFace)
        && g.isOnOrForwardPlane(f.farFace);
}

AABB::AABB(const glm::vec3& min, const glm::vec3& max)
{
    center = (min + max) * 0.5f;
    extents = glm::vec3(max.x - center.x, max.y - center.y, max.z - center.z);
}

AABB::AABB(const glm::vec3& c, float i, float j, float k)
{
    center = c;
    extents = glm::vec3(i, j, k);
}

bool AABB::isOnOrForwardPlane(const Plane& p) const
{
    float r =
        extents.x * std::abs(p.normal.x) +
        extents.y * std::abs(p.normal.y) +
        extents.z * std::abs(p.normal.z);

    return -r <= p.getSignedDistanceToPlane(center);
}

std::array<glm::vec3, 8> AABB::getVertice() const
{
    std::array<glm::vec3, 8> v;
    v[0] = { center.x - extents.x, center.y - extents.y, center.z - extents.z };
    v[1] = { center.x + extents.x, center.y - extents.y, center.z - extents.z };
    v[2] = { center.x - extents.x, center.y + extents.y, center.z - extents.z };
    v[3] = { center.x + extents.x, center.y + extents.y, center.z - extents.z };
    v[4] = { center.x - extents.x, center.y - extents.y, center.z + extents.z };
    v[5] = { center.x + extents.x, center.y - extents.y, center.z + extents.z };
    v[6] = { center.x - extents.x, center.y + extents.y, center.z + extents.z };
    v[7] = { center.x + extents.x, center.y + extents.y, center.z + extents.z };
    return v;
}

bool AABB::isOnFrustum(const Frustum& fr, const Transform& t) const
{
    glm::vec3 gc = glm::vec3(t.getModelMatrix() * glm::vec4(center, 1));

    glm::vec3 r = t.getRight() * extents.x;
    glm::vec3 u = t.getUp() * extents.y;
    glm::vec3 fwd = t.getForward() * extents.z;

    float i = std::abs(glm::dot(glm::vec3(1,0,0), r))
        + std::abs(glm::dot(glm::vec3(1,0,0), u))
        + std::abs(glm::dot(glm::vec3(1,0,0), fwd));
    float j = std::abs(glm::dot(glm::vec3(0,1,0), r))
        + std::abs(glm::dot(glm::vec3(0,1,0), u))
        + std::abs(glm::dot(glm::vec3(0,1,0), fwd));
    float k = std::abs(glm::dot(glm::vec3(0,0,1), r))
        + std::abs(glm::dot(glm::vec3(0,0,1), u))
        + std::abs(glm::dot(glm::vec3(0,0,1), fwd));

    AABB g(gc, i, j, k);

    return g.isOnOrForwardPlane(fr.leftFace)
        && g.isOnOrForwardPlane(fr.rightFace)
        && g.isOnOrForwardPlane(fr.topFace)
        && g.isOnOrForwardPlane(fr.bottomFace)
        && g.isOnOrForwardPlane(fr.nearFace)
        && g.isOnOrForwardPlane(fr.farFace);
}

Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar)
{
    Frustum fr;

    float halfV = zFar * tanf(fovY * 0.5f);
    float halfH = halfV * aspect;
    glm::vec3 frontFar = cam.Front * zFar;

    fr.nearFace = Plane(cam.Position + cam.Front * zNear, cam.Front);
    fr.farFace  = Plane(cam.Position + frontFar, -cam.Front);

    fr.rightFace = Plane(cam.Position, glm::cross(frontFar - cam.Right * halfH, cam.Up));
    fr.leftFace  = Plane(cam.Position, glm::cross(cam.Up, frontFar + cam.Right * halfH));

    fr.topFace    = Plane(cam.Position, glm::cross(cam.Right, frontFar - cam.Up * halfV));
    fr.bottomFace = Plane(cam.Position, glm::cross(frontFar + cam.Up * halfV, cam.Right));

    return fr;
}

AABB generateAABB(const Model& m)
{
    glm::vec3 minV( (std::numeric_limits<float>::max)() );
    glm::vec3 maxV( std::numeric_limits<float>::lowest() );

    for (auto& mesh : m.meshes)
    {
        for (auto& v : mesh.vertices)
        {
            minV.x = (std::min)(minV.x, v.Position.x);
            minV.y = (std::min)(minV.y, v.Position.y);
            minV.z = (std::min)(minV.z, v.Position.z);

            maxV.x = (std::max)(maxV.x, v.Position.x);
            maxV.y = (std::max)(maxV.y, v.Position.y);
            maxV.z = (std::max)(maxV.z, v.Position.z);
        }
    }
    return AABB(minV, maxV);
}

Sphere generateSphereBV(const Model& model)
{
    glm::vec3 minV( (std::numeric_limits<float>::max)() );
    glm::vec3 maxV( std::numeric_limits<float>::lowest() );

    for (auto& mesh : model.meshes)
    {
        for (auto& v : mesh.vertices)
        {
            minV = (glm::min)(minV, v.Position);
            maxV = (glm::max)(maxV, v.Position);
        }
    }

    glm::vec3 center = (minV + maxV) * 0.5f;
    float radius = glm::length(maxV - center);
    return Sphere(center, radius);
}

Entity::Entity(Model& model)
    : pModel(&model)
{
    boundingVolume = std::make_unique<AABB>(generateAABB(model));
}

AABB Entity::getGlobalAABB()
{
    glm::vec3 gc = glm::vec3(transform.getModelMatrix() * glm::vec4(boundingVolume->center, 1));

    glm::vec3 r = transform.getRight()   * boundingVolume->extents.x;
    glm::vec3 u = transform.getUp()      * boundingVolume->extents.y;
    glm::vec3 f = transform.getForward() * boundingVolume->extents.z;

    float i = std::abs(glm::dot(glm::vec3(1,0,0), r))
        + std::abs(glm::dot(glm::vec3(1,0,0), u))
        + std::abs(glm::dot(glm::vec3(1,0,0), f));

    float j = std::abs(glm::dot(glm::vec3(0,1,0), r))
        + std::abs(glm::dot(glm::vec3(0,1,0), u))
        + std::abs(glm::dot(glm::vec3(0,1,0), f));

    float k = std::abs(glm::dot(glm::vec3(0,0,1), r))
        + std::abs(glm::dot(glm::vec3(0,0,1), u))
        + std::abs(glm::dot(glm::vec3(0,0,1), f));

    return AABB(gc, i, j, k);
}

void Entity::updateSelfAndChild()
{
    if (parent == nullptr) transform.computeModelMatrix();
    else transform.computeModelMatrix(parent->transform.getModelMatrix());

    for (auto& child : children)
        child->updateSelfAndChild();
}

void Entity::forceUpdateSelfAndChild()
{
    transform.computeModelMatrix();
    for (auto& child : children)
        child->forceUpdateSelfAndChild();
}

void Entity::drawSelfAndChild(const Frustum& fr, Shader& shader, unsigned int& display, unsigned int& total)
{
    total++;

    bool visible = true;
    if (boundingVolume)
        visible = boundingVolume->isOnFrustum(fr, transform);

    if (visible)
    {
        display++;
        shader.use();
        shader.setMat4("model", transform.getModelMatrix());
        pModel->Draw(shader);
    }

    for (auto& child : children)
        child->drawSelfAndChild(fr, shader, display, total);
}
