/*
		Copyright 2024 Etay Meiri

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>
#include <assert.h>

#include "vulkan_core.h"

namespace OgldevVK {

VulkanCore::VulkanCore(GLFWwindow* pWindow)
{
	m_pWindow = pWindow;
}


VulkanCore::~VulkanCore()
{

}


void VulkanCore::Init(const char* pAppName, int NumUniformBuffers, size_t UniformDataSize)
{
	m_numUniformBuffers = NumUniformBuffers;
	m_uniformDataSize = UniformDataSize;
	CreateInstance(pAppName);
	InitDebugCallbacks();
	CreateSurface();
	m_physDevices.Init(m_instance, m_surface);
	m_devAndQueue = m_physDevices.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);
	CreateDevice();
	CreateSwapChain();
	CreateRenderPass();
	CreateFramebuffer();
	CreateCommandBufferPool();
	CreateCommandBuffers(1, &m_copyCmdBuf);
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSet();
}

void VulkanCore::CreateInstance(const char* pAppName)
{
	std::vector<const char*> ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> Extensions = {		
		"VK_KHR_surface",
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined (_WIN32)
		"VK_KHR_win32_surface",
#endif
#if defined (__APPLE__)
		"VK_MVK_macos_surface",
#endif
#if defined (__linux__)
		"VK_KHR_xcb_surface",
#endif
	    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};

	const VkApplicationInfo AppInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = pAppName,
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Ogldev Vulkan Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

	VkInstanceCreateInfo CreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &AppInfo,
		.enabledLayerCount = (uint32_t)(ValidationLayers.size()),
		.ppEnabledLayerNames = ValidationLayers.data(),
		.enabledExtensionCount = (uint32_t)(Extensions.size()),
		.ppEnabledExtensionNames = Extensions.data()
	};

	VkResult res = vkCreateInstance(&CreateInfo, NULL, &m_instance);
	CHECK_VK_RESULT(res, "Create instance");
	printf("Vulkan instance created\n");
}


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	printf("Validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL ReportCallback(VkDebugReportFlagsEXT      flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t                   object,
	size_t                     location,
	int32_t                    messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* PUserData)
{
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		return VK_FALSE;
	}

	printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);
	return VK_FALSE;
}


void VulkanCore::InitDebugCallbacks()
{
	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = nullptr
	};

	PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessenger = VK_NULL_HANDLE;
	CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");

	VkResult res = CreateDebugUtilsMessenger(m_instance, &MessengerCreateInfo, NULL, &m_messenger);
	CHECK_VK_RESULT(res, "Create debug utils messenger");

	const VkDebugReportCallbackCreateInfoEXT CallbackCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		.pNext = NULL,
		.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
					VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
					VK_DEBUG_REPORT_ERROR_BIT_EXT |
					VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		.pfnCallback = &ReportCallback,
		.pUserData = NULL
	};

	PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = VK_NULL_HANDLE;
	CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");

	res = CreateDebugReportCallback(m_instance, &CallbackCreateInfo, NULL, &m_reportCallback);
	CHECK_VK_RESULT(res, "Create debug report callback");

	printf("Debug callbacks initialized\n");
}


void VulkanCore::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_pWindow, NULL, &m_surface)) {
		fprintf(stderr, "Error creating GLFW window surface\n");
		exit(1);
	}

	printf("GLFW window surface created\n");
}


void VulkanCore::CreateDevice()
{
	VkDeviceQueueCreateInfo qInfo = {};
	qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	float qPriorities = 1.0f;
	qInfo.queueCount = 1;
	qInfo.pQueuePriorities = &qPriorities;
	qInfo.queueFamilyIndex = m_devAndQueue.Queue;

	std::vector<const char*> DevExts = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	VkDeviceCreateInfo devInfo = {};
	devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devInfo.enabledExtensionCount = (uint32_t)DevExts.size();
	devInfo.ppEnabledExtensionNames = DevExts.data();
	devInfo.queueCreateInfoCount = 1;
	devInfo.pQueueCreateInfos = &qInfo;

	VkResult res = vkCreateDevice(m_physDevices.m_devices[m_devAndQueue.Device], &devInfo, NULL, &m_device);
	CHECK_VK_RESULT(res, "Create device\n");

	printf("Device created\n");

	vkGetDeviceQueue(m_device, m_devAndQueue.Queue, 0, &m_queue);

	printf("Queue acquired\n");
}


static VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& PresentModes)
{
	for (int i = 0; i < PresentModes.size(); i++) {
		if (PresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return PresentModes[i];
		}
	}

	// Fallback to FIFO which is always supported
	return VK_PRESENT_MODE_FIFO_KHR;
}


static uint32_t ChooseNumImages(const VkSurfaceCapabilitiesKHR& Capabilities)
{
	uint32_t RequestedNumImages = Capabilities.minImageCount + 1;

	int FinalNumImages = 0;

	if ((Capabilities.maxImageCount > 0) && (RequestedNumImages > Capabilities.maxImageCount)) {
		FinalNumImages = Capabilities.maxImageCount;
	}
	else {
		FinalNumImages = RequestedNumImages;
	}

	return FinalNumImages;
}


void CreateImageView(VkDevice device,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkImageView* imageView,
	VkImageViewType viewType,
	uint32_t layerCount,
	uint32_t mipLevels)
{
	VkImageViewCreateInfo ViewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VkResult res = vkCreateImageView(device, &ViewInfo, NULL, imageView);
	CHECK_VK_RESULT(res, "vkCreateImageView");
}


void VulkanCore::CreateSwapChain()
{
	const VkSurfaceCapabilitiesKHR& SurfaceCaps = m_physDevices.m_surfaceCaps[m_devAndQueue.Device];

	assert(SurfaceCaps.currentExtent.width != -1);

	uint NumImages = ChooseNumImages(SurfaceCaps);

	const std::vector<VkPresentModeKHR>& PresentModes = m_physDevices.m_presentModes[m_devAndQueue.Device];
	VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

	VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};

	SwapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	SwapChainCreateInfo.surface = m_surface;
	SwapChainCreateInfo.minImageCount = NumImages;
	SwapChainCreateInfo.imageFormat = m_physDevices.m_surfaceFormats[m_devAndQueue.Device][0].format;
	SwapChainCreateInfo.imageColorSpace = m_physDevices.m_surfaceFormats[m_devAndQueue.Device][0].colorSpace;
	SwapChainCreateInfo.imageExtent = SurfaceCaps.currentExtent;
	SwapChainCreateInfo.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	SwapChainCreateInfo.preTransform = SurfaceCaps.currentTransform;
	SwapChainCreateInfo.imageArrayLayers = 1;
	SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	SwapChainCreateInfo.presentMode = PresentMode;
	SwapChainCreateInfo.clipped = VK_TRUE;
	SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	SwapChainCreateInfo.queueFamilyIndexCount = 1;
	SwapChainCreateInfo.pQueueFamilyIndices = &m_devAndQueue.Queue;

	VkResult res = vkCreateSwapchainKHR(m_device, &SwapChainCreateInfo, NULL, &m_swapChain);
	CHECK_VK_RESULT(res, "vkCreateSwapchainKHR\n");

	printf("Swap chain created\n");

	uint NumSwapChainImages = 0;
	res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, NULL);
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");
	assert(NumImages == NumSwapChainImages);

	printf("Number of images %d\n", NumSwapChainImages);

	m_images.resize(NumSwapChainImages);
	m_imageViews.resize(NumSwapChainImages);

	res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, &(m_images[0]));
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");

	int LayerCount = 1;
	int MipLevels = 1;
	for (uint i = 0; i < NumSwapChainImages; i++) {
		CreateImageView(m_device, m_images[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &m_imageViews[i], VK_IMAGE_VIEW_TYPE_2D, LayerCount, MipLevels);
	}
}


uint32_t VulkanCore::AcquireNextImage(VkSemaphore Semaphore)
{
	uint32_t ImageIndex = 0;
	VkResult res = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, Semaphore, NULL, &ImageIndex);
	CHECK_VK_RESULT(res, "vkAcquireNextImageKHR\n");
	return ImageIndex;
}


void VulkanCore::Submit(const VkCommandBuffer* pCmbBuf, VkSemaphore PresentCompleteSem, VkSemaphore RenderCompleteSem)
{
	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = pCmbBuf;
	if (PresentCompleteSem) {
		submitInfo.pWaitSemaphores = &PresentCompleteSem;
		submitInfo.waitSemaphoreCount = 1;
		VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.pWaitDstStageMask = &waitFlags;
	}

	if (RenderCompleteSem) {
		submitInfo.pSignalSemaphores = &RenderCompleteSem;
		submitInfo.signalSemaphoreCount = 1;
	}

	VkResult res = vkQueueSubmit(m_queue, 1, &submitInfo, NULL);
	CHECK_VK_RESULT(res, "vkQueueSubmit\n");
}

void VulkanCore::QueuePresent(uint32_t ImageIndex, VkSemaphore RenderCompleteSem)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapChain;
	presentInfo.pImageIndices = &ImageIndex;
	presentInfo.pWaitSemaphores = &RenderCompleteSem;
	presentInfo.waitSemaphoreCount = 1;

	VkResult res = vkQueuePresentKHR(m_queue, &presentInfo);
	CHECK_VK_RESULT(res, "vkQueuePresentKHR\n");
}

VkSemaphore VulkanCore::CreateSemaphore()
{
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	VkResult res = vkCreateSemaphore(m_device, &createInfo, NULL, &semaphore);
	CHECK_VK_RESULT(res, "vkCreateSemaphore\n");
	return semaphore;
}


const VkSurfaceFormatKHR& VulkanCore::GetSurfaceFormat() const
{
	return m_physDevices.m_surfaceFormats[m_devAndQueue.Device][0];
}


void VulkanCore::CreateRenderPass()
{
	VkAttachmentReference attachRef = {};
	attachRef.attachment = 0;
	attachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &attachRef;

	VkAttachmentDescription attachDesc = {};
	attachDesc.format = GetSurfaceFormat().format;
	attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;

	VkResult res = vkCreateRenderPass(m_device, &renderPassCreateInfo, NULL, &m_renderPass);
	CHECK_VK_RESULT(res, "vkCreateRenderPass\n");

	printf("Created a render pass\n");
}


void VulkanCore::CreateFramebuffer()
{
	m_fbs.resize(m_images.size());

	int WindowWidth, WindowHeight;
	glfwGetWindowSize(m_pWindow, &WindowWidth, &WindowHeight);

	VkResult res;

	for (uint i = 0; i < m_images.size(); i++) {

		VkFramebufferCreateInfo fbCreateInfo = {};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.renderPass = m_renderPass;
		fbCreateInfo.attachmentCount = 1;
		fbCreateInfo.pAttachments = &m_imageViews[i];
		fbCreateInfo.width = WindowWidth;
		fbCreateInfo.height = WindowHeight;
		fbCreateInfo.layers = 1;

		res = vkCreateFramebuffer(m_device, &fbCreateInfo, NULL, &m_fbs[i]);
		CHECK_VK_RESULT(res, "vkCreateFramebuffer\n");
	}

	printf("Framebuffers created\n");
}


VkBuffer VulkanCore::CreateVertexBuffer(const std::vector<Vector3f>& Vertices)
{
	size_t verticesSize = sizeof(Vertices);

	VkBuffer StagingVB;
	VkDeviceMemory StagingVBMem;
	VkDeviceSize AllocationSize = CreateBuffer(verticesSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		StagingVB, StagingVBMem);

	void* MappedMemAddr = NULL;
	VkResult res = vkMapMemory(m_device, StagingVBMem, 0, AllocationSize, 0, &MappedMemAddr);
	memcpy(MappedMemAddr, &Vertices[0], verticesSize);
	vkUnmapMemory(m_device, StagingVBMem);

	VkBuffer vb;
	VkDeviceMemory vbMem;
	AllocationSize = CreateBuffer(verticesSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vb, vbMem);

	CopyBuffer(vb, StagingVB, verticesSize);

	return vb;
}


BufferAndMemory VulkanCore::CreateUniformBuffer(int Size)
{
	BufferAndMemory Buffer(&m_device);

	Buffer.m_allocationSize = CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		Buffer.m_buffer, Buffer.m_mem);

	return Buffer;
}


VkDeviceSize VulkanCore::CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties,
	VkBuffer& Buffer, VkDeviceMemory& BufferMemory)
{
	VkBufferCreateInfo vbCreateInfo = {};
	vbCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vbCreateInfo.size = Size;
	vbCreateInfo.usage = Usage;
	vbCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(m_device, &vbCreateInfo, NULL, &Buffer);
	CHECK_VK_RESULT(res, "vkCreateBuffer\n");
	printf("Create vertex buffer\n");

	VkMemoryRequirements MemReqs = {};
	vkGetBufferMemoryRequirements(m_device, Buffer, &MemReqs);
	printf("Vertex buffer requires %d bytes\n", (int)MemReqs.size);

	uint32_t MemoryTypeIndex = GetMemoryTypeIndex(MemReqs.memoryTypeBits, Properties);
	VkMemoryAllocateInfo MemAllocInfo = {};
	MemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	MemAllocInfo.allocationSize = MemReqs.size;
	MemAllocInfo.memoryTypeIndex = MemoryTypeIndex;
	printf("Memory type index %d\n", MemAllocInfo.memoryTypeIndex);

	res = vkAllocateMemory(m_device, &MemAllocInfo, NULL, &BufferMemory);
	CHECK_VK_RESULT(res, "vkAllocateMemory error %d\n");

	res = vkBindBufferMemory(m_device, Buffer, BufferMemory, 0);
	CHECK_VK_RESULT(res, "vkBindBufferMemory error %d\n");

	return MemAllocInfo.allocationSize;
}



void VulkanCore::CopyBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size)
{
	VkCommandBufferBeginInfo cmdBufBeginInfo = {};
	cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult res = vkBeginCommandBuffer(m_copyCmdBuf, &cmdBufBeginInfo);
	CHECK_VK_RESULT(res, "vkBeginCommandBuffer error %d\n");

	VkBufferCopy bufferCopy = {};
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = Size;
	vkCmdCopyBuffer(m_copyCmdBuf, Src, Dst, 1, &bufferCopy);

	vkEndCommandBuffer(m_copyCmdBuf);

	Submit(&m_copyCmdBuf, NULL, NULL);

	vkQueueWaitIdle(m_queue);
}


uint32_t VulkanCore::GetMemoryTypeIndex(uint32_t memTypeBits, VkMemoryPropertyFlags reqMemPropFlags)
{
	VkPhysicalDeviceMemoryProperties& MemProps = m_physDevices.m_memProps[m_devAndQueue.Device];

	for (uint i = 0; i < MemProps.memoryTypeCount; i++) {
		if ((memTypeBits & (1 << i)) &&
			((MemProps.memoryTypes[i].propertyFlags & reqMemPropFlags) == reqMemPropFlags)) {
			return i;
		}
	}

	printf("Cannot find memory type for type %x requested mem props %x\n", memTypeBits, reqMemPropFlags);
	exit(1);
	return -1;
}


void VulkanCore::CreateCommandBufferPool()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolCreateInfo.queueFamilyIndex = m_devAndQueue.Queue;

	VkResult res = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, NULL, &m_cmdBufPool);
	CHECK_VK_RESULT(res, "vkCreateCommandPool\n");

	printf("Command buffer pool created\n");
}


void VulkanCore::CreateCommandBuffers(int count, VkCommandBuffer* cmdBufs)
{
	VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
	cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocInfo.commandPool = m_cmdBufPool;
	cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocInfo.commandBufferCount = count;

	VkResult res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, cmdBufs);
	CHECK_VK_RESULT(res, "vkAllocateCommandBuffers\n");

	printf("Created %d command buffers\n", count);
}


VkPipeline VulkanCore::CreatePipeline(VkShaderModule vs, VkShaderModule fs)
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};

	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = vs;
	shaderStageCreateInfo[0].pName = "main";
	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = fs;
	shaderStageCreateInfo[1].pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo pipelineIACreateInfo = {};
	pipelineIACreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineIACreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	int WindowWidth, WindowHeight;
	glfwGetWindowSize(m_pWindow, &WindowWidth, &WindowHeight);

	VkViewport vp = {};
	vp.x = 0.0f;
	vp.y = 0.0f;
	vp.width = (float)WindowWidth;
	vp.height = (float)WindowHeight;
	vp.minDepth = 0.0f;
	vp.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = WindowWidth;
	scissor.extent.height = WindowHeight;

	VkPipelineViewportStateCreateInfo vpCreateInfo = {};
	vpCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vpCreateInfo.viewportCount = 1;
	vpCreateInfo.pViewports = &vp;
	vpCreateInfo.scissorCount = 1;
	vpCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rastCreateInfo = {};
	rastCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rastCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rastCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rastCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo pipelineMSCreateInfo = {};
	pipelineMSCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMSCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState blendAttachState = {};
	blendAttachState.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
	blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	blendCreateInfo.attachmentCount = 1;
	blendCreateInfo.pAttachments = &blendAttachState;

	VkDescriptorSetLayoutBinding layoutBinding = {};
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = 1;
	descriptorLayout.pBindings = &layoutBinding;

	VkDescriptorSetLayout descriptorSetLayout;
	VkResult res = vkCreateDescriptorSetLayout(m_device, &descriptorLayout, NULL, &descriptorSetLayout);
	CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout\n");

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptorSetLayout;

	res = vkCreatePipelineLayout(m_device, &layoutInfo, NULL, &m_pipelineLayout);
	CHECK_VK_RESULT(res, "vkCreatePipelineLayout\n");

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = ARRAY_SIZE_IN_ELEMENTS(shaderStageCreateInfo);
	pipelineInfo.pStages = &shaderStageCreateInfo[0];
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &pipelineIACreateInfo;
	pipelineInfo.pViewportState = &vpCreateInfo;
	pipelineInfo.pRasterizationState = &rastCreateInfo;
	pipelineInfo.pMultisampleState = &pipelineMSCreateInfo;
	pipelineInfo.pColorBlendState = &blendCreateInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.basePipelineIndex = -1;

	VkPipeline Pipeline;
	res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &Pipeline);
	CHECK_VK_RESULT(res, "vkCreateGraphicsPipelines\n");

	printf("Graphics pipeline created\n");

	return Pipeline;
}


void VulkanCore::CreateUniformBuffers()
{
	m_uniformBuffers.resize(m_images.size());

	for (int i = 0; i < m_uniformBuffers.size(); i++) {
		m_uniformBuffers[i].resize(m_numUniformBuffers);

		for (int j = 0; j < m_numUniformBuffers; j++) {
			m_uniformBuffers[i][j] = CreateUniformBuffer((int)m_uniformDataSize);
		}
	}
}


void VulkanCore::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> PoolSizes;

	if (m_numUniformBuffers > 0) {
		PoolSizes.push_back(VkDescriptorPoolSize{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
													.descriptorCount = (uint32_t)m_images.size() * m_numUniformBuffers });
	}

	VkDescriptorPoolCreateInfo PoolInfo = { };

	PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	PoolInfo.maxSets = (uint32_t)m_images.size();
	PoolInfo.poolSizeCount = (uint32_t)PoolSizes.size();
	PoolInfo.pPoolSizes = PoolSizes.data();

	VkResult res = vkCreateDescriptorPool(m_device, &PoolInfo, NULL, &m_descriptorPool);
	CHECK_VK_RESULT(res, "vkCreateDescriptorPool");
	printf("Descriptor pool created\n");
}


void VulkanCore::CreateDescriptorSet()
{
	std::vector<VkDescriptorSetLayoutBinding> Bindings;

	VkDescriptorSetLayoutBinding LayoutBinding = {};

	LayoutBinding.binding = 0;
	LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	LayoutBinding.descriptorCount = 1;
	LayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	Bindings.push_back(LayoutBinding);

	VkDescriptorSetLayoutCreateInfo LayoutInfo = {};

	LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	LayoutInfo.flags = 0;
	LayoutInfo.bindingCount = static_cast<uint32_t>(Bindings.size());
	LayoutInfo.pBindings = Bindings.data();

	VkResult res = vkCreateDescriptorSetLayout(m_device, &LayoutInfo, NULL, &m_descriptorSetLayout);
	CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout");

	std::vector<VkDescriptorSetLayout> Layouts(m_images.size(), m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo AllocInfo = {};
	AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	AllocInfo.descriptorPool = m_descriptorPool;
	AllocInfo.descriptorSetCount = (uint32_t)(m_images.size());
	AllocInfo.pSetLayouts = Layouts.data();

	m_descriptorSets.resize(m_images.size());

	res = vkAllocateDescriptorSets(m_device, &AllocInfo, m_descriptorSets.data());
	CHECK_VK_RESULT(res, "vkAllocateDescriptorSets");

/*	for (size_t i = 0; i < m_images.size(); i++) {
		VkDescriptorBufferInfo BufferInfo = {};
		BufferInfo.buffer = vkState.uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBuffer)
	};

	const std::array<VkWriteDescriptorSet, 4> descriptorWrites = {
		VkWriteDescriptorSet {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = vkState.descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo
		},
	};

	vkUpdateDescriptorSets(vkDev.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}*/
}


void VulkanCore::UpdateUniformBuffer(int ImageIndex, int UniformBufferIndex, const void* pData, size_t Size)
{
	if (ImageIndex >= m_uniformBuffers.size()) {
		OGLDEV_ERROR("UpdateUniformBuffer: image index %d array size %d\n", ImageIndex, m_uniformBuffers.size());
	}

	if (UniformBufferIndex >= m_numUniformBuffers) {
		OGLDEV_ERROR("UpdateUniformBuffer: uniform buffer index %d num uniform buffers %d\n", UniformBufferIndex, m_numUniformBuffers);
	}

	m_uniformBuffers[ImageIndex][UniformBufferIndex].Update(pData, Size);
}


void BufferAndMemory::Update(const void* pData, size_t Size)
{
	if (!m_pDevice) {
		OGLDEV_ERROR("Buffer has not been initialized with a device pointer");
	}

	void* pMem = NULL;
	vkMapMemory(*m_pDevice, m_mem, 0, Size, 0, &pMem);
	memcpy(pMem, pData, Size);
	vkUnmapMemory(*m_pDevice, m_mem);
}

}
