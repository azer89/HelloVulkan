#include "Animation.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

Animation::Animation(std::string const& path, Model* model)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

	assert(scene && scene->mRootNode);
	auto animation = scene->mAnimations[0]; // TODO Can only support one animation
	duration_ = static_cast<float>(animation->mDuration);
	ticksPerSecond_ = static_cast<float>(animation->mTicksPerSecond);
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
	uint32_t size = animation->mNumChannels;

	auto& boneInfoMap = model.GetBoneInfoMap();
	int boneCount = model.GetBoneCount();

	// Reading channels(bones engaged in an animation and their keyframes)
	for (uint32_t i = 0; i < size; ++i)
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

	for (uint32_t i = 0; i < src->mNumChildren; i++)
	{
		AnimationNode newData;
		ReadHierarchyData(newData, src->mChildren[i]);
		dest.children.push_back(newData);
	}
}

bool Animation::GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix)
{
	if (boneInfoMap_.contains(name))
	{
		index = boneInfoMap_[name].id;
		offsetMatrix = boneInfoMap_[name].offsetMatrix;
		return true;
	}
	return false;
}