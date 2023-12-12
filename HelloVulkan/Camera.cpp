#include "Camera.h"
#include "AppSettings.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) :
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	MovementSpeed(CAMERA_SPEED),
	MouseSensitivity(CAMERA_SENSITIVITY),
	Zoom(CAMERA_ZOOM)
{
	Position = position;
	WorldUp = up;
	Yaw = yaw;
	Pitch = pitch;
	UpdateInternal();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
	Front(glm::vec3(0.0f, 0.0f, -1.0f)),
	MovementSpeed(CAMERA_SPEED),
	MouseSensitivity(CAMERA_SENSITIVITY),
	Zoom(CAMERA_ZOOM)
{
	Position = glm::vec3(posX, posY, posZ);
	WorldUp = glm::vec3(upX, upY, upZ);
	Yaw = yaw;
	Pitch = pitch;
	UpdateInternal();
}

glm::mat4 Camera::GetProjectionMatrix()
{
	return projectionMatrix;
}

glm::mat4 Camera::GetViewMatrix()
{
	return viewMatrix;
}

glm::mat4 Camera::GetInverseViewMatrix()
{
	return inverseViewMatrix;
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
	float velocity = MovementSpeed * deltaTime;
	if (direction == CameraForward)
	{
		Position += Front * velocity;
	}
	else if (direction == CameraBackward)
	{
		Position -= Front * velocity;
	}
	else if (direction == CameraLeft)
	{
		Position -= Right * velocity;
	}
	else if (direction == CameraRight)
	{
		Position += Right * velocity;
	}
	UpdateInternal();
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= MouseSensitivity;
	yoffset *= MouseSensitivity;

	Yaw += xoffset;
	Pitch += yoffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (Pitch > 89.0f)
		{
			Pitch = 89.0f;
		}
		else if (Pitch < -89.0f)
		{
			Pitch = -89.0f;
		}
	}

	// Update Front, Right and Up Vectors using the updated Euler angles
	UpdateInternal();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
	Zoom -= (float)yoffset;
	if (Zoom < 1.0f)
	{
		Zoom = 1.0f;
	}
	else if (Zoom > 45.0f)
	{
		Zoom = 45.0f;
	}
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::UpdateInternal()
{
	// Calculate the new Front vector
	glm::vec3 front;
	front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	front.y = sin(glm::radians(Pitch));
	front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
	Front = glm::normalize(front);

	// Also re-calculate the Right and Up vector
	Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	Up = glm::normalize(glm::cross(Right, Front));

	// Projection matrix
	float aspect = static_cast<float>(AppSettings::ScreenWidth) / static_cast<float>(AppSettings::ScreenHeight);
	assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

	float fovy = glm::radians(Zoom);
	float far = 100.0f;
	float near = 0.1f;
	const float tanHalfFovy = tan(fovy / 2.f);

	projectionMatrix = glm::mat4();
	projectionMatrix = glm::mat4{ 0.0f };
	projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
	projectionMatrix[1][1] = 1.f / (tanHalfFovy);
	projectionMatrix[2][2] = far / (far - near);
	projectionMatrix[2][3] = 1.f;
	projectionMatrix[3][2] = -(far * near) / (far - near);

	// View matrix
	const glm::vec3 w{ glm::normalize(Front) };
	const glm::vec3 u{ glm::normalize(glm::cross(w, Up)) };
	const glm::vec3 v{ glm::cross(w, u) };

	viewMatrix = glm::mat4{ 1.f };
	viewMatrix[0][0] = u.x;
	viewMatrix[1][0] = u.y;
	viewMatrix[2][0] = u.z;
	viewMatrix[0][1] = v.x;
	viewMatrix[1][1] = v.y;
	viewMatrix[2][1] = v.z;
	viewMatrix[0][2] = w.x;
	viewMatrix[1][2] = w.y;
	viewMatrix[2][2] = w.z;
	viewMatrix[3][0] = -glm::dot(u, Position);
	viewMatrix[3][1] = -glm::dot(v, Position);
	viewMatrix[3][2] = -glm::dot(w, Position);

	// Inverse view matrix
	inverseViewMatrix = glm::mat4{ 1.f };
	inverseViewMatrix[0][0] = u.x;
	inverseViewMatrix[0][1] = u.y;
	inverseViewMatrix[0][2] = u.z;
	inverseViewMatrix[1][0] = v.x;
	inverseViewMatrix[1][1] = v.y;
	inverseViewMatrix[1][2] = v.z;
	inverseViewMatrix[2][0] = w.x;
	inverseViewMatrix[2][1] = w.y;
	inverseViewMatrix[2][2] = w.z;
	inverseViewMatrix[3][0] = Position.x;
	inverseViewMatrix[3][1] = Position.y;
	inverseViewMatrix[3][2] = Position.z;
}