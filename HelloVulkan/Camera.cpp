#include "Camera.h"
#include "AppSettings.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

Camera::Camera(
	glm::vec3 position, 
	glm::vec3 up, 
	float yaw, 
	float pitch) :
	front_(glm::vec3(0.0f, 0.0f, -1.0f)),
	movementSpeed_(CameraSettings::Speed),
	mouseSensitivity_(CameraSettings::Sensitivity),
	zoom_(CameraSettings::Zoom)
{
	position_ = position;
	worldUp_ = up;
	yaw_ = yaw;
	pitch_ = pitch;
	UpdateInternal();
}

glm::mat4 Camera::GetProjectionMatrix()
{
	return projectionMatrix_;
}

glm::mat4 Camera::GetViewMatrix()
{
	return viewMatrix_;
}

glm::mat4 Camera::GetInverseViewMatrix()
{
	return inverseViewMatrix_;
}

glm::vec3 Camera::Position()
{
	return position_;
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = movementSpeed_ * deltaTime;
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
	UpdateInternal();
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= mouseSensitivity_;
	yoffset *= mouseSensitivity_;

	yaw_ += xoffset;
	pitch_ += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (pitch_ > 89.0f)
		{
			pitch_ = 89.0f;
		}
		else if (pitch_ < -89.0f)
		{
			pitch_ = -89.0f;
		}
	}

	// Update Front, Right and Up Vectors using the updated Euler angles
	UpdateInternal();
}

// Processes input received from a mouse scroll-wheel event. 
// Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
	zoom_ -= (float)yoffset;
	if (zoom_ < 1.0f)
	{
		zoom_ = 1.0f;
	}
	else if (zoom_ > 45.0f)
	{
		zoom_ = 45.0f;
	}
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::UpdateInternal()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
	front.y = sin(glm::radians(pitch_));
	front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
	front_ = glm::normalize(front);

	// Also re-calculate the Right and Up vector
	right_ = glm::normalize(glm::cross(front_, worldUp_));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	up_ = glm::normalize(glm::cross(right_, front_));

	// Projection matrix
	float aspect = static_cast<float>(AppSettings::ScreenWidth) / static_cast<float>(AppSettings::ScreenHeight);
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

	float fovy = glm::radians(zoom_);
	float far = 100.0f;
	float near = 0.1f;
	const float tanHalfFovy = tan(fovy / 2.f);

	projectionMatrix_ = glm::mat4();
	projectionMatrix_ = glm::mat4{ 0.0f };
	projectionMatrix_[0][0] = 1.f / (aspect * tanHalfFovy);
	projectionMatrix_[1][1] = 1.f / (tanHalfFovy);
	projectionMatrix_[2][2] = far / (far - near);
	projectionMatrix_[2][3] = 1.f;
	projectionMatrix_[3][2] = -(far * near) / (far - near);

	// View matrix
	const glm::vec3 w{ glm::normalize(front_) };
	const glm::vec3 u{ glm::normalize(glm::cross(w, up_)) };
	const glm::vec3 v{ glm::cross(w, u) };

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
	viewMatrix_[3][2] = -glm::dot(w, position_);

	// Inverse view matrix
	inverseViewMatrix_ = glm::mat4{ 1.f };
	inverseViewMatrix_[0][0] = u.x;
	inverseViewMatrix_[0][1] = u.y;
	inverseViewMatrix_[0][2] = u.z;
	inverseViewMatrix_[1][0] = v.x;
	inverseViewMatrix_[1][1] = v.y;
	inverseViewMatrix_[1][2] = v.z;
	inverseViewMatrix_[2][0] = w.x;
	inverseViewMatrix_[2][1] = w.y;
	inverseViewMatrix_[2][2] = w.z;
	inverseViewMatrix_[3][0] = position_.x;
	inverseViewMatrix_[3][1] = position_.y;
	inverseViewMatrix_[3][2] = position_.z;
}