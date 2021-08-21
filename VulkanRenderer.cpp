#include "VulkanRenderer.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;

#else
const bool enableValidationLayers = true;
#endif // NDEBUG


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
		createRenderPass();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();
		createCommandBuffers();
		recordCommands();
		createSynchronization();
	}
	catch (const std::runtime_error &e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}


	return 0;
}

void VulkanRenderer::draw(){

	// -- GET NEXT IMAGE --
	// wait for given fence to signal (open) from last drawing call 
	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// manually reset (close) fences
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);

	// get index of next image to draw to and signal semaphore when read to draw to
	uint32_t imageIndex;
	vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// -- SUBMIT COMMAND BUFFER TO RENDER --
	// queue submission infomration
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;								// number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];		// list of semaphores to wait on
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages;						// stages to check semaphores at
	submitInfo.commandBufferCount = 1;								// number of command buffers to submit
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];		// command buffer to submit
	submitInfo.signalSemaphoreCount = 1;							// number of semaphores to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame];	// semaphores to signal when command buffer finishes
	
	// submit command buffer to queue
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to submit command buffer to Queue");
	}

	// -- PRESENT RENDERED IMAGE TO SCREEN --
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;								// number of semaphores to wait on
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame];	// semaphore to wait on
	presentInfo.swapchainCount = 1;									// number of swapchains to present to
	presentInfo.pSwapchains = &swapchain;							// swapcahins to present images to
	presentInfo.pImageIndices = &imageIndex;						// index of images in swapchains to present

	// present image
	result = vkQueuePresentKHR(presentationQueue, &presentInfo);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present rendered image");
	}
	
	// get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
	// wait until no actions being run on the device before destroying it
	vkDeviceWaitIdle(mainDevice.logicalDevice);

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
		vkDestroyFence(mainDevice.logicalDevice,drawFences[i],nullptr);
	}
	
	vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);
	for (auto framebuffer : swapChainFrameBuffers) {
		vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);
	
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

	if (enableValidationLayers && !checkValidationLayerSupport()) {
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

		createInfo.ppEnabledLayerNames = nullptr;
	}

	// create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		
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

	VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);

	if (result != VK_SUCCESS) {
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
	SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

	// find optimal surface values for our swap chain
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
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
	swapChainCreateInfo.imageFormat = surfaceFormat.format;										// swap chain format
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;								// swap chain color space
	swapChainCreateInfo.presentMode = presentMode;												// swap chain presentation mode
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
	swapChainImageFormat = surfaceFormat.format;
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

void VulkanRenderer::createRenderPass(){

	// color attachment of render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;						// format to use for attachment
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// number of samples to write for multisampling 
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// describes what to do with attachment before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// describes what to do with attachment after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// describes what to do with stencil after rendering

	// framebuffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operation
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// image data layout before render pass starts
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// image data layout after render pass (to change to)

	// attachment reference uses an attachment index that refers to index in the attachment list passed to renderpasscreateinfo
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;							// index of the attachment in the render pass
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// information about a partiucular subpass the render pass is using
	VkSubpassDescription subPass = {};
	subPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// pipeline type subpass is to be bound to
	subPass.colorAttachmentCount = 1;
	subPass.pColorAttachments = &colorAttachmentReference;

	// need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;

	// conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// transition must happen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// subpass index (VK_SUBPASS_EXTERNAL = special value, meaning outside of render pass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// stage access mask

	// transition must happen before
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// transition must happen after...
	subpassDependencies[1].srcSubpass = 0;							
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;		
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// but must happen before
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// create info for render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subPass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create a render pass!");
	}

}

void VulkanRenderer::createGraphicsPipeline() {

	// read SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	// create shader modules
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// --SHADER STAGE CREATION INFORMATION --
	// Vertex stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;				// shader stage name
	vertexShaderCreateInfo.module = vertexShaderModule;						// shader module to be used by stage
	vertexShaderCreateInfo.pName = "main";									// entry point in to shader

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	// put shader stage creation info in to array
	// graphics pipeline creation info requires an array of shader stage creates
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// -- VERTEX INPUT --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;			// list of vertex binding descriptions (data spacing, stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;		// list of vertex attribute descriptions (data format and where to bind to or from)

	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;		// primitive type to assemble
	inputAssembly.primitiveRestartEnable = VK_FALSE;					// allow overriding of strip topology to start new primitives

	// -- VIEWPORT & SCISSOR --
	// create a viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f;									// x start coordinate
	viewport.y = 0.0f;									// y start coordinate
	viewport.width = (float)swapChainExtent.width;		// width of viewport
	viewport.height = (float)swapChainExtent.height;	// height of viewport
	viewport.minDepth = 0.0f;							// min framebuffer depth
	viewport.maxDepth = 1.0f;							// max framebuffer depth

	// create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };							// offset to use region from
	scissor.extent = swapChainExtent;					// extent to describe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- DYNAMIC STATES --
	// dynamic states to enable
	//std::vector<VkDynamicState> dynamicStateEnables;
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);		// dynamic viewport - can resize in command buffer
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);		// dynamic scissor - can resixe in command buffer

	// dynamic state crteation info
	//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	//dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- RASTERIZER -- 
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;				// change if fragments beyond near or far plane are clipped (default) or clamped to plane (needs gpu feature if other setting)
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;		// wether to discard data and sckip rasterizer, never creates fragments, only for pipeline without framebuffer
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;		// how to handle filling points between vertices (needs gpu feature if other setting)
	rasterizerCreateInfo.lineWidth = 1.0f;								// how thic lines should be when drawn (needs gpu feature if other setting)
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;			// which face of a tri to cull
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;		// winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;				// whether to add depth bias to fragments (good for stopping "shadow_acne" in shadow mapping


	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// enable multisample shading or not
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// number of samples to use per fragment


	// -- BLENDING --
	// blending decides how to blend a new color being written to a fragment with the old value

	// blend attachment state (how blending is handled)
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;	// color to apply blending to
	colorState.blendEnable = VK_TRUE;																										// enable blending

	// blending use equation (srcColorBlendFactor * new color) colorBlendOp (destColorBlendFactor * old color)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;

	// summarized: (VK_BLEND_FACTOR_SRC_ALPHA * new color) * (VKL_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
	//			   (new color alpha * new color) * ((1- new color alpha) * new color)

	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;

	// summmarized: (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;				// alternative to calculations is to use logical operations
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;

	// -- PIPELINE LAYOUT --
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// create pipeline layout
	VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	// -- DEPTH STENCIL TESTING --
	// TODO set up depth stencil testing


	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;									// number of shader stages
	pipelineCreateInfo.pStages = shaderStages;							// list of shader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// all the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;							// pipeline layout pipeline should use
	pipelineCreateInfo.renderPass = renderPass;							// render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0;										// subpass of render pass to use with pipeline

	// pipeline derivatives, can create multiple pipelines that derive from one another for optimization
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;				// existing pipeline to derive from
	pipelineCreateInfo.basePipelineIndex = -1;							// or index of pipeline being created to derive from

	// create graphics pipeline
	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	// destroy shader modules, no longer needed after pipeline created
	vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createFrameBuffers(){

	// resize frame buffer count to swapchain image count
	swapChainFrameBuffers.resize(swapChainImages.size());

	// create a framebuffer for each swapchain image
	for (size_t i = 0; i < swapChainFrameBuffers.size(); i++){
		
		std::array<VkImageView, 1> attachments = {
			swapChainImages[i].imageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;											// render pass layout the frame buffer will be using
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		
		framebufferCreateInfo.pAttachments = attachments.data();								// list of attachments (1:1 awith render pass)
		framebufferCreateInfo.width = swapChainExtent.width;									// frame buffer widht
		framebufferCreateInfo.height = swapChainExtent.height;									// frame buffer height
		framebufferCreateInfo.layers = 1;														// frame buffer layers

		VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &swapChainFrameBuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("falied to create framebuffer");
		}
	}
}

void VulkanRenderer::createCommandPool(){

	// get indices of queue families from device
	QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	// queue family type that buffers from this command pool will use

	// createa a graphics queue family command pool
	VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
	
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create command buffer pool");
	}
}

void VulkanRenderer::createCommandBuffers(){

	// resize command buffer count to have one for each framebuffer
	commandBuffers.resize(swapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = graphicsCommandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY buffer will submit directly to the pool, can't be called from other command buffers
															// VK_COMMAND_BUFFER_LEVEL_SECONDARY can't be called directrly, but can be colled from other buffers via vkCommandExecuteBuffers
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// allocate command buffers and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers");
	}

}

void VulkanRenderer::createSynchronization() {

	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);
	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {

		if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS || 
			vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS || 
			vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create a semaphore and/or fence");
		}
	}
}

void VulkanRenderer::recordCommands() {

	// information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// information about how to begin a  render pass (only needed for graphical application)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;								// render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };							// start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = swapChainExtent;					// size of region to run render pass on (starting at offset)
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	renderPassBeginInfo.pClearValues = clearValues;								// list of clear values (TODO depth attachment clear value)
	renderPassBeginInfo.clearValueCount = 1;	

	for (size_t i = 0; i < commandBuffers.size(); i++){

		renderPassBeginInfo.framebuffer = swapChainFrameBuffers[i];

		// start recording commands to command buffer
		VkResult result = vkBeginCommandBuffer(commandBuffers[i], &bufferBeginInfo);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to start recording a command buffer");
		}

		// begin render pass
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind pipeline to be used in render pass
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		// execute our pipeline
		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

		// end render pass
		vkCmdEndRenderPass(commandBuffers[i]);

		// stop recording to command buffer
		result = vkEndCommandBuffer(commandBuffers[i]);

		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to stop recording a command buffer");
		}
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

			if (strcmp(checkExtension, extension.extensionName)==0) {
				
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


bool VulkanRenderer::checkValidationLayerSupport() {
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
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}
	return indices.isValid() && extensionsSupported && swapChainValid;
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
		if (indices.isValid()) {
			break;
		}

		i++;
	}
	return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device){

	SwapChainDetails swapChainDetails;
	// -- CAPABILITIES--
	// get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	// -- FORMATS--
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	// if formats returned, get list of formats
	if (formatCount != 0) {
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	// --PRESENTATION MODES--
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	// if presentation modes returned, get list of presentation modes
	if (presentationCount != 0) {
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
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

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes){

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
		throw std::runtime_error("Failed to create an image view!");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code){
	
	// shader module creation information
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();										// size of code
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());		// pointer to code (uint32_t type)

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create the shader module");
	}

	return shaderModule;
}
