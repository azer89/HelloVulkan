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