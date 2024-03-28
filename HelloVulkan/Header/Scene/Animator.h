#ifndef SCENE_ANIMATOR
#define SCENE_ANIMATOR

/*
Adapted from learnopengl.com/Guest-Articles/2020/Skeletal-Animation
*/

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

	//[[nodiscard]] std::vector<glm::mat4>& GetFinalBoneMatrices() { return finalBoneMatrices_; }

public:
	std::vector<glm::mat4> skinningMatrices_ = {};

private:
	
	Animation* currentAnimation_ = nullptr;
	float currentTime_ = 0.0f;
	float deltaTime_ = 0.0f;
};

#endif
