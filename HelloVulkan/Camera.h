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
const float CAMERA_YAW = -90.0f;
const float CAMERA_PITCH = 0.0f;
const float CAMERA_SPEED = 2.5f;
const float CAMERA_SENSITIVITY = 0.1f;
const float CAMERA_ZOOM = 45.0f;

class Camera
{
public:
	// Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	// Euler Angles
	float Yaw;
	float Pitch;

	// Options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
		float yaw = CAMERA_YAW,
		float pitch = CAMERA_PITCH);

	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetViewMatrix();
	glm::mat4 GetInverseViewMatrix();

	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
	void ProcessMouseScroll(float yoffset);

private:
	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 inverseViewMatrix;

private:
	void UpdateInternal();
};

#endif
