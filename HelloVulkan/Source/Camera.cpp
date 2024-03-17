#include "Camera.h"
#include "Configs.h"

// WARNING Need to set GLM_FORCE_DEPTH_ZERO_TO_ONE in the vcxproj config
#include "glm/glm.hpp"
#include "glm/ext.hpp"

Camera::Camera(
	glm::vec3 position, 
	glm::vec3 worldUp, 
	float screenWidth,
	float screenHeight,
	float yaw, 
	float pitch) :
	position_(position),
	worldUp_(worldUp),
	yaw_(yaw),
	pitch_(pitch),
	front_(glm::vec3(0.f)),
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

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime_)
{
	float velocity = movementSpeed_ * deltaTime_;
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
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= mouseSensitivity_;
	yoffset *= mouseSensitivity_;

	yaw_ += xoffset;
	pitch_ += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		const float pitchLimit = 89.f;
		pitch_ = glm::clamp(pitch_, -pitchLimit, pitchLimit);
	}

	// Update orthogonal axes and matrices
	UpdateInternal();
}

// Processes input received from a mouse scroll-wheel event. 
// Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
	zoom_ -= (float)yoffset;
	zoom_ = glm::clamp(zoom_, 1.f, 45.f);

	// Update orthogonal axes and matrices
	UpdateInternal();
}

void Camera::UpdateInternal()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
	front.y = sin(glm::radians(pitch_));
	front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
	front_ = glm::normalize(front);

	// Calculate three orthogonal axes
	right_ = glm::normalize(glm::cross(front_, worldUp_));
	up_ = glm::normalize(glm::cross(right_, front_));

	// Projection matrix
	projectionMatrix_ = glm::perspective(glm::radians(zoom_),
		static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_),
		CameraConfig::Near,
		CameraConfig::Far);
	projectionMatrix_[1][1] *= -1;
	inverseProjectionMatrix_ = glm::inverse(projectionMatrix_);

	// View matrix
	viewMatrix_ = glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::GetProjectionMatrix() const
{
	return projectionMatrix_;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return viewMatrix_;
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
		.position = glm::vec4(position_, 1.f)
	};
}

RaytracingCameraUBO Camera::GetRaytracingCameraUBO() const
{
	return
	{
		.projectionInverse = glm::inverse(projectionMatrix_),
		.viewInverse = glm::inverse(viewMatrix_),
	};
}

ClusterForwardUBO Camera::GetClusterForwardUBO() const
{
	float zFloat = static_cast<float>(ClusterForwardConfig::SliceCountZ);
	float log2FarDivNear = glm::log2(CameraConfig::Far / CameraConfig::Near);
	float log2Near = glm::log2(CameraConfig::Near);

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
	glm::mat4 projView = projectionMatrix_ * viewMatrix_;
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

	glm::mat4 invProjView = glm::inverse(projView);

	const glm::vec4 corners[8] =
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