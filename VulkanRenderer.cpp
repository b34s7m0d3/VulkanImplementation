#include "VulkanRenderer.h"



VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow * newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}


	return 0;
}

void VulkanRenderer::cleanup()
{
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}


VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{

	// information about the application
	// most data here does not affect the program and it is for dev convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App x64";			// custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	// custom version of the application
	appInfo.pEngineName = "No Engine";						// custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);		// custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_2;				// the vulkan version


	// creation information for a VkInstance 
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	

	// create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// set up extensions instance will use
	uint32_t glfwExtensionCount = 0;						// GLFW may require multiple extensions
	const char** glfwExtensions;							// extesions passed as array of cstrings, so need pointer (array) to pointer (cstring)

	// get GLFW extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++) {

		instanceExtensions.push_back(glfwExtensions[i]);
	}

	// check instance extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions)) {
		
		throw std::runtime_error("VkInstance does not uspport required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO set up validation layers that instance will use
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		
		throw std::runtime_error("failed to create a vulkan instance");
	}

}

void VulkanRenderer::createLogicalDevice()
{
	// get the queue family indices for the choosen physical device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// Queueus the logica device needs to create and info to do so (only one for now, add more later)
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;			// the index of the family to create a queue from
	queueCreateInfo.queueCount = 1;										// number of queues to create
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;						// vulkan needs to know how to handle multiple queues, so decide prioriy (1 highest)

	// information to create logical device (somestimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;							// number of queue create infos
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;				// list of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = 0;							// number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;					// list of enabled logical device extensions
	
	// physical device features the logical device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;				// physical device features logical device will use

	// create the logical device for the given physical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device");
	 }

	// Queues are created at the same time as the device
	// so we want to hand the quees
	// given logical device of given queue family of given queue index (0 since olny one queue) place reference in given queue
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

void VulkanRenderer::getPhysicalDevice()
{
	// enumerate physical devices the ckinstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// if no devices available, then none support vulkan
	if (deviceCount == 0) {
		throw std::runtime_error("Can't find GPUs that support Vulkan instance");
	}

	// get list of physical devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto &device : deviceList) {

		if (checkDeviceSuitable(device)) {
			mainDevice.physicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// need to get number of extensions to create array of correct size to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// create a list of vkExtension properties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// check if given extensions are in list of available extensions
	for (const auto &checkExtension : *checkExtensions) {
		
		bool hasExtension = false;
		for (const auto &extension : extensions) {

			if (strcmp(checkExtension, extension.extensionName)) {
				
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) {
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// information about the device itself
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// information about what the device can do
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = getQueueFamilies(device);

	return indices.isVaild();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// get all queue family property info for the given devices
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// go through each queue family and check if it has at least one of the reuqired types of queue
	int i = 0;
	for (const auto &queueFamily : queueFamilyList) {
		// first check if queue family has at least 1 queue in that family
		// queue can have multiple types defined through bitfield, need to bitwise check if it has required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

			indices.graphicsFamily = i;		// if queue family is valid, get index
		}

		// check if queue family indices are in a valid state, stop searching if so
		if (indices.isVaild()) {
			break;
		}

		i++;
	}
	return indices;
}
