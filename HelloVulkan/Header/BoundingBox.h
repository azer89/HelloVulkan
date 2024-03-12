#ifndef BOUNDING_BOX
#define BOUNDING_BOX

#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct VertexData;

struct BoundingBox
{
public:
	glm::vec3 min_;
	glm::vec3 max_;
	
public:

	BoundingBox() = default;
	BoundingBox(const glm::vec3& min, const glm::vec3& max);
	BoundingBox(const VertexData* vertices, size_t vertextCount);

	glm::vec3 GetSize() const { return glm::vec3(max_[0] - min_[0], max_[1] - min_[1], max_[2] - min_[2]); }
	glm::vec3 GetCenter() const { return 0.5f * glm::vec3(max_[0] + min_[0], max_[1] + min_[1], max_[2] + min_[2]); }
	
	void Transform(const glm::mat4& t)
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

	BoundingBox GetTransformed(const glm::mat4& t) const
	{
		BoundingBox b = *this;
		b.Transform(t);
		return b;
	}
};

#endif