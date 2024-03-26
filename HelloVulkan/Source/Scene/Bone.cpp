#include "Bone.h"

inline glm::vec3 CastToGLMVec3(const aiVector3D& vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::quat CastToGLMQuat(const aiQuaternion& pOrientation)
{
	return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

Bone::Bone(const std::string& name, int id, const aiNodeAnim* channel) :
	name_(name),
	id_(id),
	localTransform_(1.0f)
{
	positionCount_ = channel->mNumPositionKeys;

	for (int positionIndex = 0; positionIndex < positionCount_; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
		float timeStamp = channel->mPositionKeys[positionIndex].mTime;
		KeyPosition data =
		{
			.position = CastToGLMVec3(aiPosition),
			.timeStamp = timeStamp
		};
		positions_.push_back(data);
	}

	rotationCount_ = channel->mNumRotationKeys;
	for (int rotationIndex = 0; rotationIndex < rotationCount_; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
		float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
		KeyRotation data =
		{
			.orientation = CastToGLMQuat(aiOrientation),
			.timeStamp = timeStamp
		};
		rotations_.push_back(data);
	}

	scalingCount_ = channel->mNumScalingKeys;
	for (int keyIndex = 0; keyIndex < scalingCount_; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
		float timeStamp = channel->mScalingKeys[keyIndex].mTime;
		KeyScale data =
		{
			.scale = CastToGLMVec3(scale),
			.timeStamp = timeStamp
		};
		scales_.push_back(data);
	}
}

void Bone::Update(float animationTime)
{
	glm::mat4 translation = InterpolatePosition(animationTime);
	glm::mat4 rotation = InterpolateRotation(animationTime);
	glm::mat4 scale = InterpolateScaling(animationTime);
	localTransform_ = translation * rotation * scale;
}

uint32_t Bone::GetPositionIndex(float animationTime)
{
	for (uint32_t index = 0; index < positionCount_ - 1; ++index)
	{
		if (animationTime < positions_[index + 1].timeStamp)
		{
			return index;
		}
	}
	assert(0);
}

uint32_t Bone::GetRotationIndex(float animationTime)
{
	for (uint32_t index = 0; index < rotationCount_ - 1; ++index)
	{
		if (animationTime < rotations_[index + 1].timeStamp)
		{
			return index;
		}
	}
	assert(0);
}

uint32_t Bone::GetScaleIndex(float animationTime)
{
	for (uint32_t index = 0; index < scalingCount_ - 1; ++index)
	{
		if (animationTime < scales_[index + 1].timeStamp)
		{
			return index;
		}
	}
	assert(0);
}


float Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float scaleFactor = 0.0f;
	float midWayLength = animationTime - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	scaleFactor = midWayLength / framesDiff;
	return scaleFactor;
}

glm::mat4 Bone::InterpolatePosition(float animationTime)
{
	if (positionCount_ == 1)
	{
		return glm::translate(glm::mat4(1.0f), positions_[0].position);
	}

	int p0Index = GetPositionIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(positions_[p0Index].timeStamp,
		positions_[p1Index].timeStamp, animationTime);
	glm::vec3 finalPosition =
		glm::mix(positions_[p0Index].position,
			positions_[p1Index].position,
			scaleFactor);
	return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::InterpolateRotation(float animationTime)
{
	if (rotationCount_ == 1)
	{
		auto rotation = glm::normalize(rotations_[0].orientation);
		return glm::toMat4(rotation);
	}

	int p0Index = GetRotationIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(rotations_[p0Index].timeStamp,
		rotations_[p1Index].timeStamp, animationTime);
	glm::quat finalRotation =
		glm::slerp(rotations_[p0Index].orientation,
			rotations_[p1Index].orientation,
			scaleFactor);
	finalRotation = glm::normalize(finalRotation);
	return glm::toMat4(finalRotation);

}

glm::mat4 Bone::InterpolateScaling(float animationTime)
{
	if (scalingCount_ = 1)
	{
		return glm::scale(glm::mat4(1.0f), scales_[0].scale);
	}

	int p0Index = GetScaleIndex(animationTime);
	int p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(scales_[p0Index].timeStamp,
		scales_[p1Index].timeStamp, animationTime);
	glm::vec3 finalScale =
		glm::mix(scales_[p0Index].scale,
			scales_[p1Index].scale,
			scaleFactor);
	return glm::scale(glm::mat4(1.0f), finalScale);
}