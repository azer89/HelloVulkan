#include "Animator.h"

#include <iostream>

constexpr uint32_t MAX_BONE_MATRICES = 100; // TODO

Animator::Animator(Animation* animationPtr)
{
	currentTime_ = 0.0;
	currentAnimation_ = animationPtr;

	skinningMatrices_.reserve(MAX_BONE_MATRICES);

	for (int i = 0; i < MAX_BONE_MATRICES; i++)
	{
		skinningMatrices_.push_back(glm::mat4(1.0f));
	}
}

void Animator::UpdateAnimation(float dt)
{
	deltaTime_ = dt;
	if (currentAnimation_)
	{
		currentTime_ += currentAnimation_->GetTicksPerSecond() * deltaTime_;
		currentTime_ = fmod(currentTime_, currentAnimation_->GetDuration());
		CalculateBoneTransform(&currentAnimation_->GetRootNode(), glm::mat4(1.0f));
	}
}

void Animator::PlayAnimation(Animation* animationPtr)
{
	currentAnimation_ = animationPtr;
	currentTime_ = 0.0f;
}

void Animator::CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform)
{
	std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;

	Bone* bone = currentAnimation_->FindBone(nodeName);

	if (bone)
	{
		bone->Update(currentTime_);
		nodeTransform = bone->GetLocalTransform();
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	//auto boneInfoMap = currentAnimation_->GetBoneIDMap();
	int index = -1;
	glm::mat4 offsetMatrix = glm::mat4(1.0f);
	if (currentAnimation_->GetIndexAndOffsetMatrix(nodeName, index, offsetMatrix))
	{
		if (index >= skinningMatrices_.size())
		{
			std::cerr << "finalBoneMatrices_ is not long enough, index = " << index << '\n';
		}
		else
		{
			skinningMatrices_[index] = globalTransformation * offsetMatrix;
		}
	}

	for (uint32_t i = 0; i < node->childrenCount; ++i)
	{
		CalculateBoneTransform(&node->children[i], globalTransformation);
	}
}