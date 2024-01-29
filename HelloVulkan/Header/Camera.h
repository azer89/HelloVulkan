#ifndef CAMERA
#define CAMERA

#include "glm/glm.hpp"

#include "Configs.h"
#include "UBO.h"

enum CameraMovement
{
	CameraForward,
	CameraBackward,
	CameraLeft,
	CameraRight,
};

class Camera
{
public:
	Camera(
		glm::vec3 position,
		glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
		float screenWidth = AppConfig::InitialScreenWidth,
		float screenHeight = AppConfig::InitialScreenHeight,
		float yaw = CameraConfig::Yaw,
		float pitch = CameraConfig::Pitch);

	void SetScreenSize(float width, float height);
	void ProcessKeyboard(CameraMovement direction, float deltaTime_);
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
	void ProcessMouseScroll(float yoffset);

	glm::mat4 GetProjectionMatrix() const;
	glm::mat4 GetViewMatrix() const;
	glm::vec3 Position() const;
	PerFrameUBO GetPerFrameUBO() const;

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

	// Screen size
	float screenWidth_;
	float screenHeight_;

private:
	void UpdateInternal();
};

#endif
