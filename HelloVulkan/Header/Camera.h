#ifndef CAMERA
#define CAMERA

// NOTE Need to set GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include "Configs.h"
#include "Event.h"
#include "UBOs.h"
#include "Ray.h"

enum class CameraMovement : uint8_t
{
	Forward = 0u,
	Backward = 1u,
	Left = 2u,
	Right = 3u,
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

	void SetPositionAndTarget(glm::vec3 cameraPosition, glm::vec3 cameraTarget);
	void SetScreenSize(float width, float height);
	void ProcessKeyboard(CameraMovement direction, float deltaTime);
	void ProcessMouseMovement(float xOffset, float yOffset);
	void ProcessMouseScroll(float yOffset);

	[[nodiscard]] glm::mat4 GetProjectionMatrix() const;
	[[nodiscard]] glm::mat4 GetInverseProjectionMatrix() const;
	[[nodiscard]] glm::mat4 GetViewMatrix() const;
	[[nodiscard]] glm::mat4 GetInverseViewMatrix() const;
	[[nodiscard]] glm::vec3 Position() const;
	[[nodiscard]] CameraUBO GetCameraUBO() const;
	[[nodiscard]] ClusterForwardUBO GetClusterForwardUBO() const;
	[[nodiscard]] FrustumUBO GetFrustumUBO() const;
	[[nodiscard]] Ray GetRayFromScreenToWorld(float screenPosX, float screenPosY) const;

public:
	Event<> ChangedEvent_;

private:
	glm::mat4 projectionMatrix_{};
	glm::mat4 inverseProjectionMatrix_{};
	glm::mat4 viewMatrix_{};

	glm::vec3 position_{};
	glm::vec3 worldUp_{};

	// Orthogonal axes
	glm::vec3 front_{};
	glm::vec3 up_{};
	glm::vec3 right_{};

	// Euler Angles
	float yaw_ = 0;
	float pitch_ = 0;

	// Options
	float movementSpeed_ = 0;
	float mouseSensitivity_ = 0;
	float zoom_ = 0;

	// Screen size
	float screenWidth_ = 0;
	float screenHeight_ = 0;

private:
	void UpdateInternal();
	void ConstrainPitch();
};

#endif
