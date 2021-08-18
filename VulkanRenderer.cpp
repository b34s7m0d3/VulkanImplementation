#include "VulkanRenderer.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;

#else
const bool enableValidationLayers = true;
#endif // NDEBUG


//std::vector<const char*> getRequiredExtensions() {
//	uint32_t glfwExtensionCount = 0;
//	const char** glfwExtensions;
//	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//
//	if (enableValidationLayers) {
//		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//	}
//
//	return extensions;
//}


VulkanRenderer::VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow * newWindow)
{
	window = newWindow;

	try
	{
		createInstance();
		createDebugMessenger();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
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
	for (auto image : swapChainImages) {
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}

	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}


VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{

	if (enableValidationLayers && !checkVlaidationLayerSupport()) {
		throw std::runtime_error("validation layers requested but not available!");
	}

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

	if (enableValidationLayers) {
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	// check instance extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions)) {
		
		throw std::runtime_error("VkInstance does not uspport required extensions!");
	}

	
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();
	
	//VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		//populateDebugMEssengerCreateInfo(debugCreateInfo);
		//createInfo.pNext = (VkDebugUtilsMessengerCreateFlagsEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	// create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (vkCreateInstance (&createInfo, nullptr, &instance) != VK_SUCCESS) {
		
		throw std::runtime_error("failed to create a vulkan instance");
	}

}


void VulkanRenderer::createDebugMessenger() {
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	//populateDebugMEssengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// get the queue family indices for the choosen physical device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// vector for queue creation information and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logica device needs to create and info to do so 
	for (int queueFamilyIndex : queueFamilyIndices) {

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;				// the index of the family to create a queue from
		queueCreateInfo.queueCount = 1;										// number of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;						// vulkan needs to know how to handle multiple queues, so decide prioriy (1 highest)
	
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// information to create logical device (somestimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		// number of queue create infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								// list of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());	// number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							// list of enabled logical device extensions
	
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
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface() {
	// create Surface (creates a surface create info struct, runs the create surface function, returns VK result)
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a surface");
	}

}

void VulkanRenderer::createSwapChain(){
	// get swapchain details so we can pick best settings
	SwapChanDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

	// find optimal surface values for our swap chain
	VkSurfaceFormatKHR surfaceformat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentmode = chooseBestPresentationMode(swapChainDetails.presentationMode);
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	// how many images are in the swap chain? get 1 more than the minimum to allow triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// if adding one goes over maximum, make max our imageCount
	// if 0 then limitless
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount) {
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	// creation information for swapchain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;														// swap chain surface
	swapChainCreateInfo.imageFormat = surfaceformat.format;										// swap chain format
	swapChainCreateInfo.imageColorSpace = surfaceformat.colorSpace;								// swap chain color space
	swapChainCreateInfo.presentMode = presentmode;												// swap chain presentation mode
	swapChainCreateInfo.imageExtent = extent;													// swap chain image extent
	swapChainCreateInfo.minImageCount = imageCount;												// minimum images in swapchain
	swapChainCreateInfo.imageArrayLayers = 1;													// number of layers for each image in chain
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;						// what attachment images will be used as
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;	// transform to perform on swap chain
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;						// how to handle blending images with external graphics
	swapChainCreateInfo.clipped = VK_TRUE;														// whether to clip parts of image

	// get queue family indices
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// if graphics and presentation families are different, then swapchain must let images be shared between families

	if (indices.graphicsFamily != indices.presentationFamily) {

		// queues to share between
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};

		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;		// image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2;							// number of ques to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;			// array of queues to share between
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// if old swap chain been destroyed and this one replaces it, then link old one to quicly hand over responsibilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// create swapchain

	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Swapchain!");
	}

	// store for later reference
	swapChainImageFormat = surfaceformat.format;
	swapChainExtent = extent;

	// get swapchain images (first count, then values)
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);

	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data());

	for (VkImage image : images) {

		// store image handle
		SwapChainImage swapChainImage = {};
		swapChainImage.image = image;
		swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		// add to swapchain image list
		swapChainImages.push_back(swapChainImage);
	}
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

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// If no extensions found, return failure
	if (extensionCount == 0)
	{
		return false;
	}

	// Populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto &deviceExtension : deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto &extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}


bool VulkanRenderer::checkVlaidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
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

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;

	if (extensionsSupported) {
		SwapChanDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationMode.empty() && !swapChainDetails.formats.empty();
	}
	return indices.isVaild() && extensionsSupported && swapChainValid;
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


		// check if queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

		// check if queue is presentation type (can be both graphics and presentation type)
		if (queueFamily.queueCount > 0 && presentationSupport) {
			indices.presentationFamily = i;
		}

		// check if queue family indices are in a valid state, stop searching if so
		if (indices.isVaild()) {
			break;
		}

		i++;
	}
	return indices;
}

SwapChanDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device){

	SwapChanDetails swapChainDetails;
	// -- CAPABILITIES--
	// get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	// -- FORMATS--
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	// if formats returned, get list of formats
	if (formatCount != 0) {
		swapChainDetails.formats.reserve(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	// --PRESENTATION MODES--
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	// if presentation modes returned, get list of presentation modes
	if (presentationCount != 0) {
		swapChainDetails.presentationMode.reserve(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationMode.data());
	}

	return swapChainDetails;
}

// best format is subjective, but this will be
// format		:	VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
// colorspace	:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {

	// if only 1 format is available and is undefined it means all are available and no restrictions
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// if restricted, find optimal
	for (const auto &format : formats) {
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	// if optimal not found, use first
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes){

	// look for mailbox presentation mode
	for (const auto &presentationMode : presentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return presentationMode;
		}
	}

	// if can't find look for FIFO as vulkan spec says it must be present
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & surfaceCapabilities){

	// if current extent is at numeric limits, the extent can vary, otherwise it's the size of the window
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		// if value can vary, need to set manually

		// get window size
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// surface also defines max and min, so make sure within boundaries by clamping value
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectflags) {

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;									// image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;				// type of image 
	viewCreateInfo.format = format;									// format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;	// allows remapping of rgba components to other rgba values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// subresources allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectflags;		// which aspect of image to view (example COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;				// start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;					// number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;				// start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;					// number of array levels to view

	// create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);

	if (result != VK_SUCCESS) {
		std::runtime_error("Failed to create an image view!");
	}

	return imageView;
}
