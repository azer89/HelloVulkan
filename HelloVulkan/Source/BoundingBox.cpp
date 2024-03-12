#include "BoundingBox.h"
#include "Mesh.h"

BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max) : min_(glm::min(min, max)), max_(glm::max(min, max))
{
}

BoundingBox::BoundingBox(const VertexData* vertices, size_t vertextCount)
{
	glm::vec3 vmin(std::numeric_limits<float>::max());
	glm::vec3 vmax(std::numeric_limits<float>::lowest());

	for (size_t i = 0; i != vertextCount; i++)
	{
		// TODO Remove casting from v4 to vec3
		vmin = glm::min(vmin, glm::vec3(vertices[i].position_));
		vmax = glm::max(vmax, glm::vec3(vertices[i].position_));
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