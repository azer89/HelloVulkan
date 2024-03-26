#ifndef SCENE_ANIMATOR
#define SCENE_ANIMATOR

#include "Bone.h"
#include "Animation.h"

#include "glm/glm.hpp"

#include <vector>

class Animator
{
public:
	Animator(Animation* animationPtr);

	void UpdateAnimation(float dt);
	void PlayAnimation(Animation* animationPtr);
	void CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform);

	[[nodiscard]] std::vector<glm::mat4>& GetFinalBoneMatrices() { return finalBoneMatrices_; }

private:
	std::vector<glm::mat4> finalBoneMatrices_ = {};
	Animation* currentAnimation_ = nullptr;
	float currentTime_ = 0.0f;
	float deltaTime_ = 0.0f;
};

#endif
