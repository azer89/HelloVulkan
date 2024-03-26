#include "Animation.h"

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

Animation::Animation(const std::string& animationPath, Model* model)
{
	Assimp::Importer importer;
	const aiScene* scene = model->GetAssimpScene();
	assert(scene && scene->mRootNode);
	auto animation = scene->mAnimations[0];
	duration_ = animation->mDuration;
	ticksPerSecond_ = animation->mTicksPerSecond;
	aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
	globalTransformation = globalTransformation.Inverse();
	ReadHierarchyData(rootNode_, scene->mRootNode);
	ReadMissingBones(animation, *model);
}

Bone* Animation::FindBone(const std::string& name)
{
	auto iter = std::find_if(bones_.begin(), bones_.end(),
		[&](const Bone& Bone)
		{
			return Bone.GetBoneName() == name;
		}
	);
	if (iter == bones_.end())
	{
		return nullptr;
	}

	return &(*iter);
}

void Animation::ReadMissingBones(const aiAnimation* animation, Model& model)
{
	int size = animation->mNumChannels;

	auto& boneInfoMap = model.GetBoneInfoMap();
	int boneCount = model.GetBoneCount();

	// Reading channels(bones engaged in an animation and their keyframes)
	for (int i = 0; i < size; i++)
	{
		auto channel = animation->mChannels[i];
		std::string boneName = channel->mNodeName.data;

		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			boneInfoMap_[boneName].id = boneCount;
			boneCount++;
		}
		bones_.emplace_back(channel->mNodeName.data,
			boneInfoMap_[channel->mNodeName.data].id, channel);
	}

	// Copy a set
	boneInfoMap_ = boneInfoMap;
}

void Animation::ReadHierarchyData(AnimationNode& dest, const aiNode* src)
{
	assert(src);

	dest.name = src->mName.data;
	dest.transformation = CastToGLMMat4(src->mTransformation);
	dest.childrenCount = src->mNumChildren;

	for (int i = 0; i < src->mNumChildren; i++)
	{
		AnimationNode newData;
		ReadHierarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}