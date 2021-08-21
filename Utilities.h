#pragma once

#include <fstream>

const int MAX_FRAME_DRAWS = 2;

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

static std::vector<char> readFile(const std::string &filename) {
	// open stream from given file
	// std::ios::binary tells streamt o read file as binary
	// std::ios::ate tells stream to start reading from end of file
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	// check if file stream successfully opened
	if (!file.is_open()) {
		throw std::runtime_error("failed to open a file");
	}

	// get current read position and use to resize file buffer
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);

	// move read position (seek to) start of the file
	file.seekg(0);

	// read the file data into the buffer 
	file.read(fileBuffer.data(),fileSize);

	// close stream
	file.close();

	return fileBuffer;
}