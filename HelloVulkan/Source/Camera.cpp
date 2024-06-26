#include "Camera.h"
#include "Configs.h"

// WARNING Need to set GLM_FORCE_DEPTH_ZERO_TO_ONE in the vcxproj config
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <iostream>

Camera::Camera(
	glm::vec3 position, 
	glm::vec3 worldUp, 
	float screenWidth,
	float screenHeight,
	float yaw, 
	float pitch) :
	position_(position),
	worldUp_(worldUp),
	front_(glm::vec3(0.f)),
	yaw_(yaw),
	pitch_(pitch),
	movementSpeed_(CameraConfig::Speed),
	mouseSensitivity_(CameraConfig::Sensitivity),
	zoom_(CameraConfig::Zoom),
	screenWidth_(screenWidth),
	screenHeight_(screenHeight)
{
	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::SetScreenSize(float width, float height)
{
	screenWidth_ = width;
	screenHeight_ = height;

	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	const float velocity = movementSpeed_ * deltaTime;
	if (direction == CameraMovement::Forward)
	{
		position_ += front_ * velocity;
	}
	else if (direction == CameraMovement::Backward)
	{
		position_ -= front_ * velocity;
	}
	else if (direction == CameraMovement::Left)
	{
		position_ -= right_ * velocity;
	}
	else if (direction == CameraMovement::Right)
	{
		position_ += right_ * velocity;
	}
	
	// Update orthogonal axes and matrices
	UpdateInternal();
}

// Processes input received from a mouse input system. 
// Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xOffset, float yOffset)
{
	xOffset *= mouseSensitivity_;
	yOffset *= mouseSensitivity_;

	yaw_ += glm::radians(xOffset);
	pitch_ += glm::radians(yOffset);
	
	ConstrainPitch();

	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::SetPositionAndTarget(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
{
	position_ = cameraPosition;

	const glm::vec3 direction = glm::normalize(cameraTarget - position_);
	pitch_ = asin(direction.y);
	yaw_ = atan2(direction.z, direction.x);

	ConstrainPitch();

	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::ConstrainPitch()
{
	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	constexpr float pitchLimit = 1.55334f; // 89 degree
	pitch_ = glm::clamp(pitch_, -pitchLimit, pitchLimit);
}

// Processes input received from a mouse scroll-wheel event. 
// Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yOffset)
{
	zoom_ -= yOffset;
	zoom_ = glm::clamp(zoom_, 1.f, CameraConfig::Zoom);

	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::UpdateInternal()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(yaw_) * cos(pitch_);
	front.y = sin(pitch_);
	front.z = sin(yaw_) * cos(pitch_);
	front_ = glm::normalize(front);

	// Calculate three orthogonal axes
	right_ = glm::normalize(glm::cross(front_, worldUp_));
	up_ = glm::normalize(glm::cross(right_, front_));

	// Projection matrix
	projectionMatrix_ = glm::perspective(zoom_,
		static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_),
		CameraConfig::Near,
		CameraConfig::Far);
	projectionMatrix_[1][1] *= -1;
	inverseProjectionMatrix_ = glm::inverse(projectionMatrix_);

	// View matrix
	viewMatrix_ = glm::lookAt(position_, position_ + front_, up_);

	// Tell the listeners the camera has changed
	ChangedEvent_.Invoke();
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	return projectionMatrix_;
}

glm::mat4 Camera::GetInverseProjectionMatrix() const
{
	return inverseProjectionMatrix_;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return viewMatrix_;
}

glm::mat4 Camera::GetInverseViewMatrix() const
{
	return glm::inverse(viewMatrix_);
}

glm::vec3 Camera::Position() const
{
	return position_;
}

CameraUBO Camera::GetCameraUBO() const
{
	return
	{
		.projection = projectionMatrix_,
		.view = viewMatrix_,
		.position = glm::vec4(position_, 1.f),
		.cameraNear = CameraConfig::Near,
		.cameraFar = CameraConfig::Far,
	};
}

ClusterForwardUBO Camera::GetClusterForwardUBO() const
{
	const float zFloat = static_cast<float>(ClusterForwardConfig::SliceCountZ);
	const float log2FarDivNear = glm::log2(CameraConfig::Far / CameraConfig::Near);
	const float log2Near = glm::log2(CameraConfig::Near);

	return
	{
		.cameraInverseProjection = inverseProjectionMatrix_,
		.cameraView = viewMatrix_,
		.screenSize = glm::vec2(screenWidth_, screenHeight_),
		.sliceScaling = zFloat / log2FarDivNear,
		.sliceBias = -(zFloat * log2Near / log2FarDivNear),
		.cameraNear = CameraConfig::Near,
		.cameraFar = CameraConfig::Far,
		.sliceCountX = ClusterForwardConfig::SliceCountX,
		.sliceCountY = ClusterForwardConfig::SliceCountY,
		.sliceCountZ = ClusterForwardConfig::SliceCountZ
	};
}

FrustumUBO Camera::GetFrustumUBO() const
{
	const glm::mat4 projView = projectionMatrix_ * viewMatrix_;
	glm::mat4 t = glm::transpose(projView);

	FrustumUBO ubo =
	{
		.planes =
		{
			glm::vec4(t[3] + t[0]), // left
			glm::vec4(t[3] - t[0]), // right
			glm::vec4(t[3] + t[1]), // bottom
			glm::vec4(t[3] - t[1]), // top
			glm::vec4(t[3] + t[2]), // near
			glm::vec4(t[3] - t[2])  // far
		}
	};

	const glm::mat4 invProjView = glm::inverse(projView);

	static const glm::vec4 corners[8] =
	{
		glm::vec4(-1, -1, -1, 1), glm::vec4( 1, -1, -1, 1),
		glm::vec4( 1,  1, -1, 1), glm::vec4(-1,  1, -1, 1),
		glm::vec4(-1, -1,  1, 1), glm::vec4( 1, -1,  1, 1),
		glm::vec4( 1,  1,  1, 1), glm::vec4(-1,  1,  1, 1)
	};

	for (int i = 0; i < 8; i++)
	{
		const glm::vec4 q = invProjView * corners[i];
		ubo.corners[i] = q / q.w;
	}

	return ubo;
}

Ray Camera::GetRayFromScreenToWorld(float screenPosX, float screenPosY) const
{
	const glm::mat4 inverseViewMatrix_ = glm::inverse(viewMatrix_);
	const glm::vec4 rayClipSpace =
		glm::vec4(
			(static_cast<float>(screenPosX) / screenWidth_) * 2.f - 1.f,
			(static_cast<float>(screenPosY) / screenHeight_) * 2.f - 1.f,
			-1.f,
			1.f);
	glm::vec4 rayViewSpace = inverseProjectionMatrix_ * rayClipSpace;
	rayViewSpace.z = -1.f;
	rayViewSpace.w = 0.f;

	glm::vec4 rayWorldSpace = inverseViewMatrix_ * rayViewSpace;
	rayWorldSpace = glm::normalize(rayWorldSpace);

	return Ray(position_, glm::vec3(rayWorldSpace));
}