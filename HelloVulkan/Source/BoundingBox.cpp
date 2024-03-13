#include "BoundingBox.h"
#include "VertexData.h"

#include "glm/ext.hpp"

BoundingBox::BoundingBox(const glm::vec3* points, size_t pointCount)
{
	glm::vec3 vmin(std::numeric_limits<float>::max());
	glm::vec3 vmax(std::numeric_limits<float>::lowest());

	for (size_t i = 0; i != pointCount; i++)
	{
		vmin = glm::min(vmin, points[i]);
		vmax = glm::max(vmax, points[i]);
	}
	min_ = vmin;
	max_ = vmax;
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

	*this = BoundingBox(corners, 8);
}

BoundingBox BoundingBox::GetTransformed(const glm::mat4& t) const
{
	BoundingBox b = *this;
	b.Transform(t);
	return b;
}