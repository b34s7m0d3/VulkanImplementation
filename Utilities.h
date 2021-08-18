#pragma once

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1;			// location of graphics queue family
	int presentationFamily = -1;		// location of presentation queue family

	// check if queue families are valid
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};


struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;		// Surface properties, example image size/extent
	std::vector<VkSurfaceFormatKHR> formats;			// Surface image formats it can support, example RGBA and size of each color
	std::vector<VkPresentModeKHR> presentationModes;		// how images should be presented to screen

};

struct SwapChainImage {
	VkImage image;
	VkImageView imageView;
};