#include "BoundingBox.h"
#include "VertexData.h"

#include "glm/ext.hpp"

BoundingBox::BoundingBox(const std::span<glm::vec3> points)
{
	glm::vec3 vmin(std::numeric_limits<float>::max());
	glm::vec3 vmax(std::numeric_limits<float>::lowest());

	for (glm::vec3& p : points)
	{
		vmin = glm::min(vmin, p);
		vmax = glm::max(vmax, p);
	}
	min_ = glm::vec4(vmin, 1.0);
	max_ = glm::vec4(vmax, 1.0);
}

BoundingBox::BoundingBox(const std::span<VertexData> vertexDataArray)
{
	glm::vec3 vMin(std::numeric_limits<float>::max());
	glm::vec3 vMax(std::numeric_limits<float>::lowest());
	for (auto& vData : vertexDataArray)
	{
		const glm::vec3& v = vData.position;
		vMin = glm::min(vMin, v);
		vMax = glm::max(vMax, v);
	}
	min_ = glm::vec4(vMin, 1.0);
	max_ = glm::vec4(vMax, 1.0);
}

void BoundingBox::Transform(const glm::mat4& t)
{
	glm::vec3 corners[] = {
		glm::vec3(min_.x, min_.y, min_.z),
		glm::vec3(min_.x, max_.y, min_.z),
		glm::vec3(min_.x, min_.y, max_.z),
		glm::vec3(min_.x, max_.y, max_.z),
		glm::vec3(max_.x, min_.y, min_.z),
		glm::vec3(max_.x, max_.y, min_.z),
		glm::vec3(max_.x, min_.y, max_.z),
		glm::vec3(max_.x, max_.y, max_.z),
	};
	for (auto& v : corners)
	{
		v = glm::vec3(t * glm::vec4(v, 1.0f));
	}

	*this = BoundingBox(corners);
}

BoundingBox BoundingBox::GetTransformed(const glm::mat4& t) const
{
	BoundingBox b = *this;
	b.Transform(t);
	return b;
}

// gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms
bool BoundingBox::Hit(const Ray& r, float& t)
{
	float t1 = (min_.x - r.origin_.x) * r.dirFrac_.x;
	float t2 = (max_.x - r.origin_.x) * r.dirFrac_.x;
	float t3 = (min_.y - r.origin_.y) * r.dirFrac_.y;
	float t4 = (max_.y - r.origin_.y) * r.dirFrac_.y;
	float t5 = (min_.z - r.origin_.z) * r.dirFrac_.z;
	float t6 = (max_.z - r.origin_.z) * r.dirFrac_.z;

	float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
	if (tmax < 0)
	{
		t = tmax;
		return false;
	}

	// if tmin > tmax, ray doesn't intersect AABB
	if (tmin > tmax)
	{
		t = tmax;
		return false;
	}

	t = tmin;
	return true;
}