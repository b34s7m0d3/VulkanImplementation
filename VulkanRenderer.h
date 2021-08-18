#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>

#include "VulkanValidation.h"
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
	// - Main
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	// - Utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// vulkan functions
	// create functions
	void createInstance();
	void createDebugMessenger();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	std::vector<SwapChainImage> swapChainImages;

	// Get functions
	void getPhysicalDevice();

	// support functions
	// -- checker functions
	bool checkInstanceExtensionSupport(std::vector<const char*> * checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// -- getter functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	// -- Choose functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR> &presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

	// -- Create functions
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectflags);
};

