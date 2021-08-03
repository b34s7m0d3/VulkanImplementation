#pragma once

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of queue families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1;			// location of graphics queue family

	// check if queue families are valid
	bool isVaild() {
		return graphicsFamily >= 0;
	}
};
