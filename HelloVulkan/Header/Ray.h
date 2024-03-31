#ifndef HELLO_VULKAN_RAY
#define HELLO_VULKAN_RAY

#include "glm/glm.hpp"

#include <iostream>

struct Ray
{
public:
	glm::vec3 origin_;
	glm::vec3 direction_;
	glm::vec3 dirFrac_;

public:
	Ray(glm::vec3 origin, glm::vec3 direction) :
		origin_(origin),
		direction_(direction)
	{
		dirFrac_.x = 1.f / direction_.x;
		dirFrac_.y = 1.f / direction_.y;
		dirFrac_.z = 1.f / direction_.z;
	}

	void Print()
	{
		std::cout << "origin: " << origin_.x << ", " << origin_.y << ", " << origin_.z << "\n";
		std::cout << "direction: " << direction_.x << ", " << direction_.y << ", " << direction_.z << "\n";
	}
};

#endif