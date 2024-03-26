#include "Animator.h"

Animator::Animator(Animation* animationPtr)
{
	currentTime_ = 0.0;
	currentAnimation_ = animationPtr;

	finalBoneMatrices_.reserve(100); // TODO

	for (int i = 0; i < 100; i++)
	{
		finalBoneMatrices_.push_back(glm::mat4(1.0f));
	}
}

void Animator::UpdateAnimation(float dt)
{
	deltaTime_ = dt;
	if (currentAnimation_)
	{
		currentTime_ += currentAnimation_->GetTicksPerSecond() * dt;
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

	Bone* Bone = currentAnimation_->FindBone(nodeName);

	if (Bone)
	{
		Bone->Update(currentTime_);
		nodeTransform = Bone->GetLocalTransform();
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	auto boneInfoMap = currentAnimation_->GetBoneIDMap();
	if (boneInfoMap.contains(nodeName))
	{
		int index = boneInfoMap[nodeName].id;
		glm::mat4 offset = boneInfoMap[nodeName].offsetMatrix;
		finalBoneMatrices_[index] = globalTransformation * offset;
	}

	for (uint32_t i = 0; i < node->childrenCount; ++i)
	{
		CalculateBoneTransform(&node->children[i], globalTransformation);
	}
}