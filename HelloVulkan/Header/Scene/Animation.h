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
#include <unordered_map>

class Animation
{
public:
	Animation(std::string const& path, Model* model);
	~Animation() = default;

	[[nodiscard]] Bone* FindBone(const std::string& name);
	[[nodiscard]] float GetTicksPerSecond() const { return ticksPerSecond_; }
	[[nodiscard]] float GetDuration() const { return duration_; }
	[[nodiscard]] const AnimationNode& GetRootNode() const { return rootNode_; }
	//[[nodiscard]] const std::unordered_map<std::string, BoneInfo>& GetBoneIDMap() const { return boneInfoMap_; }
	[[nodiscard]] bool GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix);

private:
	void ReadMissingBones(const aiAnimation* animation, Model& model);
	void ReadHierarchyData(AnimationNode& dest, const aiNode* src);

private:
	float duration_;
	float ticksPerSecond_;
	std::vector<Bone> bones_;
	AnimationNode rootNode_;
	std::unordered_map<std::string, BoneInfo> boneInfoMap_;
};

#endif