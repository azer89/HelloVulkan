#include "Animator.h"

#include <iostream>

Animator::Animator()
{
	Reset();
}

void Animator::UpdateAnimation(
	Animation* animation,
	std::vector<glm::mat4>& skinningMatrices,
	float dt)
{
	deltaTime_ = dt;
	currentTime_ += animation->GetTicksPerSecond() * deltaTime_;
	currentTime_ = fmod(currentTime_, animation->GetDuration());
	glm::mat4 initMat = glm::mat4(1.0f);
	CalculateBoneTransform(
		animation,
		&(animation->rootNode_),
		initMat,
		skinningMatrices);
}

void Animator::Reset()
{
	currentTime_ = 0.0f;
}

void Animator::CalculateBoneTransform(
	Animation* animation,
	const AnimationNode* node,
	const glm::mat4& parentTransform,
	std::vector<glm::mat4>& skinningMatrices)
{
	const std::string nodeName = node->name;
	glm::mat4 nodeTransform = node->transformation;
	Bone* bone = animation->GetBone(nodeName);

	if (bone)
	{
		bone->Update(currentTime_);
		nodeTransform = bone->GetLocalTransform();
	}

	glm::mat4 globalTransformation = parentTransform * nodeTransform;

	int index = -1;
	glm::mat4 offsetMatrix = glm::mat4(1.0f);
	if (animation->GetIndexAndOffsetMatrix(nodeName, index, offsetMatrix))
	{
		if (index >= skinningMatrices.size())
		{
			std::cerr << "finalBoneMatrices_ is not long enough, index = " << index << 
				", skinningMatrices size = " << skinningMatrices.size() << '\n';
		}
		else
		{
			skinningMatrices[index] = globalTransformation * offsetMatrix;
		}
	}

	for (uint32_t i = 0; i < node->childrenCount; ++i)
	{
		CalculateBoneTransform(
			animation,
			&node->children[i],
			globalTransformation,
			skinningMatrices);
	}
}