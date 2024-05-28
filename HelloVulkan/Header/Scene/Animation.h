#ifndef SCENE_ANIMATION
#define SCENE_ANIMATION

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

#include "Bone.h"
#include "Model.h"
#include "ScenePODs.h"

#include <string>
#include <vector>

class Animation
{
public:
	Animation() = default;
	~Animation() = default;

	void Init(std::string const& path, Model* model);

	[[nodiscard]] Bone* GetBone(const std::string& name);
	[[nodiscard]] float GetTicksPerSecond() const { return ticksPerSecond_; }
	[[nodiscard]] float GetDuration() const { return duration_; }
	[[nodiscard]] bool GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix) const;

private:
	void AddBones(const aiAnimation* animation);
	void CreateHierarchy(AnimationNode& dest, const aiNode* src);

public:
	AnimationNode rootNode_{};

private:
	float duration_ = 0.0f;
	float ticksPerSecond_ = 0.0f;
	Model* model_{};
	std::unordered_map<std::string, Bone> boneMap_{};
};

#endif