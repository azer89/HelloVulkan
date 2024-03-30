#ifndef BOUNDING_BOX
#define BOUNDING_BOX

#include "Utility.h"
#include "VertexData.h"

#include "glm/glm.hpp"

#include <span>

struct BoundingBox
{
public:
	glm::vec4 min_;
	glm::vec4 max_;
	
public:

	BoundingBox() = default;
	BoundingBox(const std::span<glm::vec3> points);
	BoundingBox(const std::span<VertexData> vertexDataArray);

	[[nodiscard]] glm::vec3 GetSize() const { return glm::vec3(max_[0] - min_[0], max_[1] - min_[1], max_[2] - min_[2]); }
	[[nodiscard]] glm::vec3 GetCenter() const { return 0.5f * glm::vec3(max_[0] + min_[0], max_[1] + min_[1], max_[2] + min_[2]); }
	
	void Transform(const glm::mat4& t);
	[[nodiscard]] BoundingBox GetTransformed(const glm::mat4& t) const;
};

#endif