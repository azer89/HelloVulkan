#include "Camera.h"
#include "Configs.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
	if (direction == CameraForward)
	{
		position_ += front_ * velocity;
	}
	else if (direction == CameraBackward)
	{
		position_ -= front_ * velocity;
	}
	else if (direction == CameraLeft)
	{
		position_ -= right_ * velocity;
	}
	else if (direction == CameraRight)
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
	/*float aspect = screenWidth_ / screenHeight_;
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

	float fovy = glm::radians(zoom_);
	float far = AppConfig::far;
	float near = AppConfig::near;
	const float tanHalfFovy = tan(fovy / 2.f);

	projectionMatrix_ = glm::mat4();
	projectionMatrix_ = glm::mat4{ 0.0f };
	projectionMatrix_[0][0] = 1.f / (aspect * tanHalfFovy);
	projectionMatrix_[1][1] = 1.f / (tanHalfFovy);
	projectionMatrix_[2][2] = far / (far - near);
	projectionMatrix_[2][3] = 1.f;
	projectionMatrix_[3][2] = -(far * near) / (far - near);*/

	projectionMatrix_ = glm::perspective(glm::radians(zoom_),
		static_cast<float>(screenWidth_) / static_cast<float>(screenHeight_),
		CameraConfig::Near,
		CameraConfig::Far);
	projectionMatrix_[1][1] *= -1;

	// View matrix
	/*const glm::vec3 w = front_;
	const glm::vec3 u = right_;
	const glm::vec3 v{ glm::cross(w, u) }; // Flip Y axis

	viewMatrix_ = glm::mat4{ 1.f };
	viewMatrix_[0][0] = u.x;
	viewMatrix_[1][0] = u.y;
	viewMatrix_[2][0] = u.z;
	viewMatrix_[0][1] = v.x;
	viewMatrix_[1][1] = v.y;
	viewMatrix_[2][1] = v.z;
	viewMatrix_[0][2] = w.x;
	viewMatrix_[1][2] = w.y;
	viewMatrix_[2][2] = w.z;
	viewMatrix_[3][0] = -glm::dot(u, position_);
	viewMatrix_[3][1] = -glm::dot(v, position_);
	viewMatrix_[3][2] = -glm::dot(w, position_);*/

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
		.position = glm::vec4(position_, 1.f),
		.near = CameraConfig::Near,
		.far = CameraConfig::Far
	};
}