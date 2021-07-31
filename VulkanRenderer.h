#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	int init(GLFWwindow * newWindow);
	void cleanup();


	~VulkanRenderer();

private: 
	GLFWwindow * window;

	// vulkan components
	VkInstance instance;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;

	// vulkan functions
	// create functions
	void createInstance();
	void createLogicalDevice();

	// Get functions
	void getPhysicalDevice();

	// support functions
	// -- checker functions
	bool checkInstanceExtensionSupport(std::vector<const char*> * checkExtensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// -- getter functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
};

