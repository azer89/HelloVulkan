#include "Animation.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"

inline glm::mat4 CastToGLMMat4(const aiMatrix4x4& m)
{
	return glm::transpose(glm::make_mat4(&m.a1));
}

void Animation::Init(std::string const& path, Model* model) 
{
	model_ = model;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
	assert(scene && scene->mRootNode);

	const aiAnimation* animation = scene->mAnimations[0]; // TODO Currently can only support one animation
	duration_ = static_cast<float>(animation->mDuration);
	ticksPerSecond_ = static_cast<float>(animation->mTicksPerSecond);
	aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
	globalTransformation = globalTransformation.Inverse();
	CreateHierarchy(rootNode_, scene->mRootNode);
	AddBones(animation);
}

Bone* Animation::GetBone(const std::string& name)
{
	if (boneMap_.contains(name))
	{
		return &boneMap_[name];
	}
	return nullptr;
}

void Animation::AddBones(const aiAnimation* animation)
{
	int boneCounter = model_->GetBoneCounter();

	// Reading channels (bones engaged in an animation and their keyframes)
	const uint32_t size = animation->mNumChannels;
	for (uint32_t i = 0; i < size; ++i)
	{
		auto channel = animation->mChannels[i];
		const std::string boneName = channel->mNodeName.data;

		// Add additional bones if not found
		if (!model_->boneInfoMap_.contains(boneName))
		{
			model_->boneInfoMap_[boneName].id_ = boneCounter;
			boneCounter++;
		}

		// Add the bone to map
		boneMap_[boneName] = Bone(boneName, model_->boneInfoMap_[boneName].id_, channel);
	}
}

void Animation::CreateHierarchy(AnimationNode& dest, const aiNode* src)
{
	assert(src);

	dest.name_ = src->mName.data;
	dest.transformation_ = CastToGLMMat4(src->mTransformation);
	dest.childrenCount_ = src->mNumChildren;

	for (uint32_t i = 0; i < src->mNumChildren; i++)
	{
		AnimationNode newData;
		CreateHierarchy(newData, src->mChildren[i]);
		dest.children_.push_back(newData);
	}
}

bool Animation::GetIndexAndOffsetMatrix(const std::string& name, int& index, glm::mat4& offsetMatrix) const
{
	if (model_->boneInfoMap_.contains(name))
	{
		index = model_->boneInfoMap_[name].id_;
		offsetMatrix = model_->boneInfoMap_[name].offsetMatrix_;
		return true;
	}
	return false;
}