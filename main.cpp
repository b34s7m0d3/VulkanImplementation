#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>

#include "VulkanRenderer.h"


GLFWwindow * window;
VulkanRenderer vulkanRenderer;


void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600) {

	// initialize GLFW
	glfwInit();

	// set GLFW to not work with OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() {

	// create window
	initWindow("Test Window", 800, 600);

	// create vulkan renderer instance
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		
		return EXIT_FAILURE;
	}

	// loop until closed
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	vulkanRenderer.cleanup();

	// destroy window and terminate glfw
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}