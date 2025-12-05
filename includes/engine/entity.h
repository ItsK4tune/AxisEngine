#pragma once

#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <list> 
#include <array> 
#include <memory> 

#include <engine/model.h>
#include <engine/camera.h>

class Transform
{
protected:
	glm::vec3 m_pos = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_eulerRot = { 0.0f, 0.0f, 0.0f }; 
	glm::vec3 m_scale = { 1.0f, 1.0f, 1.0f };

	glm::mat4 m_modelMatrix = glm::mat4(1.0f);

	bool m_isDirty = true;

	glm::mat4 getLocalModelMatrix();

public:

	void computeModelMatrix();
	void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix);

	void setLocalPosition(const glm::vec3& newPosition);
	void setLocalRotation(const glm::vec3& newRotation);
	void setLocalScale(const glm::vec3& newScale);

	const glm::vec3& getGlobalPosition() const;
	const glm::vec3& getLocalPosition() const;
	const glm::vec3& getLocalRotation() const;
	const glm::vec3& getLocalScale() const;
	const glm::mat4& getModelMatrix() const;
	glm::vec3 getRight() const;
	glm::vec3 getUp() const;
	glm::vec3 getBackward() const;
	glm::vec3 getForward() const;
	glm::vec3 getGlobalScale() const;

	bool isDirty() const;
};

struct Plane
{
	glm::vec3 normal = { 0.f, 1.f, 0.f };
	float     distance = 0.f;

	Plane();
	Plane(const glm::vec3& p1, const glm::vec3& norm);

	float getSignedDistanceToPlane(const glm::vec3& point) const;
};

struct Frustum
{
	Plane topFace;
	Plane bottomFace;

	Plane rightFace;
	Plane leftFace;

	Plane farFace;
	Plane nearFace;
};

struct BoundingVolume
{
	virtual bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const = 0;
	virtual bool isOnOrForwardPlane(const Plane& plane) const = 0;
	
	bool isOnFrustum(const Frustum& camFrustum) const;
};

struct Sphere : public BoundingVolume
{
	glm::vec3 center{ 0.f, 0.f, 0.f };
	float radius{ 0.f };

	Sphere(const glm::vec3& inCenter, float inRadius);

	bool isOnOrForwardPlane(const Plane& plane) const final;
	bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const final;
};

struct SquareAABB : public BoundingVolume
{
	glm::vec3 center{ 0.f, 0.f, 0.f };
	float extent{ 0.f };

	SquareAABB(const glm::vec3& inCenter, float inExtent);

	bool isOnOrForwardPlane(const Plane& plane) const final;
	bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const final;
};

struct AABB : public BoundingVolume
{
	glm::vec3 center{ 0.f, 0.f, 0.f };
	glm::vec3 extents{ 0.f, 0.f, 0.f };

	AABB(const glm::vec3& min, const glm::vec3& max);
	AABB(const glm::vec3& inCenter, float iI, float iJ, float iK);

	std::array<glm::vec3, 8> getVertice() const;

	bool isOnOrForwardPlane(const Plane& plane) const final;

	bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const final;
};

Frustum createFrustumFromCamera(const Camera& cam, float aspect, float fovY, float zNear, float zFar);
AABB generateAABB(const Model& model);
Sphere generateSphereBV(const Model& model);

class Entity
{
public:
	std::list<std::unique_ptr<Entity>> children;
	Entity* parent = nullptr;

	Transform transform;

	Model* pModel = nullptr;
	std::unique_ptr<AABB> boundingVolume;

	Entity(Model& model);

	AABB getGlobalAABB();

	template<typename... TArgs>
	void addChild(TArgs&... args)
	{
		children.emplace_back(std::make_unique<Entity>(args...));
		children.back()->parent = this;
	}

	void updateSelfAndChild();
	void forceUpdateSelfAndChild();
	void drawSelfAndChild(const Frustum& frustum, Shader& ourShader, unsigned int& display, unsigned int& total);
};