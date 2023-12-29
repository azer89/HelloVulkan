#ifndef CAMERA
#define CAMERA

#include <glm/glm.hpp>

enum CameraMovement
{
	CameraForward,
	CameraBackward,
	CameraLeft,
	CameraRight,
};

// Default camera values
namespace CameraSettings
{
	const float Yaw = -90.0f;
	const float Pitch = 0.0f;
	const float Speed = 2.5f;
	const float Sensitivity = 0.1f;
	const float Zoom = 45.0f;
}

class Camera
{
public:
	Camera(
		glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = CameraSettings::Yaw,
		float pitch = CameraSettings::Pitch);

	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();
	glm::vec3 Position();

	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
	void ProcessMouseScroll(float yoffset);

private:
	glm::mat4 projectionMatrix_;
	glm::mat4 viewMatrix_;

	glm::vec3 position_;
	glm::vec3 worldUp_;

	// Orthogonal axes
	glm::vec3 front_;
	glm::vec3 up_;
	glm::vec3 right_;

	// Euler Angles
	float yaw_;
	float pitch_;

	// Options
	float movementSpeed_;
	float mouseSensitivity_;
	float zoom_;

private:
	void UpdateInternal();
};

#endif
