#ifndef BOUNDING_BOX
#define BOUNDING_BOX

#include "glm/glm.hpp"

struct VertexData;

struct BoundingBox
{
public:
	glm::vec3 min_;
	glm::vec3 max_;
	
public:

	BoundingBox() = default;
	BoundingBox(const glm::vec3* points, size_t pointCount);

	[[nodiscard]] glm::vec3 GetSize() const { return glm::vec3(max_[0] - min_[0], max_[1] - min_[1], max_[2] - min_[2]); }
	[[nodiscard]] glm::vec3 GetCenter() const { return 0.5f * glm::vec3(max_[0] + min_[0], max_[1] + min_[1], max_[2] + min_[2]); }
	
	void Transform(const glm::mat4& t);
	[[nodiscard]] BoundingBox GetTransformed(const glm::mat4& t) const;
};

#endif