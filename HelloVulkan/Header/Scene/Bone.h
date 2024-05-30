#ifndef SCENE_BONE
#define SCENE_BONE

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

#include "ScenePODs.h"

#include "assimp/scene.h"

class Bone
{
public:
	Bone() = default;
	Bone(const std::string& name, int id, const aiNodeAnim* channel);

	void Update(float animationTime);

	[[nodiscard]] glm::mat4 GetLocalTransform() const { return localTransform_; }
	[[nodiscard]] std::string GetBoneName() const { return name_; }

private:
	[[nodiscard]] int GetPositionIndex(float animationTime) const;
	[[nodiscard]] int GetRotationIndex(float animationTime) const;
	[[nodiscard]] int GetScaleIndex(float animationTime) const;
	[[nodiscard]] float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
	[[nodiscard]] glm::mat4 InterpolatePosition(float animationTime);
	[[nodiscard]] glm::mat4 InterpolateRotation(float animationTime);
	[[nodiscard]] glm::mat4 InterpolateScaling(float animationTime);

private:
	std::vector<KeyPosition> positions_{};
	std::vector<KeyRotation> rotations_{};
	std::vector<KeyScale> scales_{};
	std::string name_{};
	glm::mat4 localTransform_{};
	uint32_t positionCount_ = 0;
	uint32_t rotationCount_ = 0;
	uint32_t scalingCount_ = 0;
	int id_ = -1;
};

#endif
