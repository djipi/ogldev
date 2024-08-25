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
#include <string>
#include <assert.h>

#include "ogldev_types.h"
#include "ogldev_util.h"
#include "ogldev_vulkan_core.h"
#include "ogldev_vulkan_util.h"

namespace OgldevVK {

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	printf("Debug callback: %s\n", pCallbackData->pMessage);
	printf("  Severity %s\n", GetDebugSeverityStr(Severity));
	printf("  Type %s\n", GetDebugType(Type));
	printf("  Objects ");

	for (u32 i = 0; i < pCallbackData->objectCount; i++) {
		printf("%llux ", pCallbackData->pObjects[i].objectHandle);
	}

	printf("\n");

	return VK_FALSE;  // The calling function should not be aborted
}


VulkanCore::VulkanCore()
{
}


VulkanCore::~VulkanCore()
{
	printf("-------------------------------\n");

	vkDestroyCommandPool(m_device, m_cmdBufPool, NULL);

	m_queue.Destroy();

	for (int i = 0; i < m_imageViews.size(); i++) {
		vkDestroyImageView(m_device, m_imageViews[i], NULL);
	}

	vkDestroySwapchainKHR(m_device, m_swapChain, NULL);

	vkDestroyDevice(m_device, NULL);

	PFN_vkDestroySurfaceKHR vkDestroySurface = VK_NULL_HANDLE;
	vkDestroySurface = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(m_instance, "vkDestroySurfaceKHR");
	if (!vkDestroySurface) {
		OGLDEV_ERROR0("Cannot find address of vkDestroySurfaceKHR\n");
		exit(1);
	}

	vkDestroySurface(m_instance, m_surface, NULL);

	printf("GLFW window surface destroyed\n");

	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = VK_NULL_HANDLE;
	vkDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
	if (!vkDestroyDebugUtilsMessenger) {
		OGLDEV_ERROR0("Cannot find address of vkDestroyDebugUtilsMessengerEXT\n");
		exit(1);
	}
	vkDestroyDebugUtilsMessenger(m_instance, m_debugMessenger, NULL);

	printf("Debug callback destroyed\n");

	vkDestroyInstance(m_instance, NULL);
	printf("Vulkan instance destroyed\n");
}


void VulkanCore::Init(const char* pAppName, GLFWwindow* pWindow)
{
	m_pWindow = pWindow;
	CreateInstance(pAppName);
	CreateDebugCallback();
	if (!pWindow) {
		printf("You are probably in one of the initial tutorials so we can end the Init function here.\n");
		return;
	}
	CreateSurface();
	m_physDevices.Init(m_instance, m_surface);
	m_queueFamily = m_physDevices.SelectDevice(VK_QUEUE_GRAPHICS_BIT, true);
	CreateDevice();
	CreateSwapChain();
	CreateCommandBufferPool();
	m_queue.Init(m_device, m_swapChain, m_queueFamily, 0);
}


const VkImage& VulkanCore::GetImage(int Index) const
{
	if (Index >= m_images.size()) {
		OGLDEV_ERROR("Invalid image index %d\n", Index);
		exit(1);
	}

	return m_images[Index];
}

void VulkanCore::CreateInstance(const char* pAppName)
{
	std::vector<const char*> Layers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> Extensions = {
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
	};

	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = NULL
	};

	VkApplicationInfo AppInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = pAppName,
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "Ogldev Vulkan Tutorials",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	VkInstanceCreateInfo CreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &MessengerCreateInfo,
		.flags = 0,				// reserved for future use. Must be zero
		.pApplicationInfo = &AppInfo,
		.enabledLayerCount = (u32)(Layers.size()),
		.ppEnabledLayerNames = Layers.data(),
		.enabledExtensionCount = (u32)(Extensions.size()),
		.ppEnabledExtensionNames = Extensions.data()
	};

	VkResult res = vkCreateInstance(&CreateInfo, NULL, &m_instance);
	CHECK_VK_RESULT(res, "Create instance");
	printf("Vulkan instance created\n");
}


void VulkanCore::CreateDebugCallback()
{
	VkDebugUtilsMessengerCreateInfoEXT MessengerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = NULL,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = NULL
	};

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = VK_NULL_HANDLE;
	vkCreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
	if (!vkCreateDebugUtilsMessenger) {
		OGLDEV_ERROR0("Cannot find address of vkCreateDebugUtilsMessenger\n");
		exit(1);
	}

	VkResult res = vkCreateDebugUtilsMessenger(m_instance, &MessengerCreateInfo, NULL, &m_debugMessenger);
	CHECK_VK_RESULT(res, "debug utils messenger");

	printf("Debug utils messenger created\n");
}


void VulkanCore::CreateSurface()
{
	VkResult res = glfwCreateWindowSurface(m_instance, m_pWindow, NULL, &m_surface);
	CHECK_VK_RESULT(res, "glfwCreateWindowSurface");

	printf("GLFW window surface created\n");
}


void VulkanCore::CreateDevice()
{
	float qPriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo qInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0, // must be zero
		.queueFamilyIndex = m_queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qPriorities[0]
	};

	std::vector<const char*> DevExts = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	if (m_physDevices.Selected().m_features.geometryShader == VK_FALSE) {
		OGLDEV_ERROR0("The Geometry Shader is not supported!\n");
	}

	if (m_physDevices.Selected().m_features.tessellationShader == VK_FALSE) {
		OGLDEV_ERROR0("The Tessellation Shader is not supported!\n");
	}

	VkPhysicalDeviceFeatures DeviceFeatures = { 0 };
	DeviceFeatures.geometryShader = VK_TRUE;
	DeviceFeatures.tessellationShader = VK_TRUE;

	VkDeviceCreateInfo DeviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qInfo,
		.enabledLayerCount = 0,			// DEPRECATED
		.ppEnabledLayerNames = NULL,    // DEPRECATED
		.enabledExtensionCount = (u32)DevExts.size(),
		.ppEnabledExtensionNames = DevExts.data(),
		.pEnabledFeatures = &DeviceFeatures
	};

	VkResult res = vkCreateDevice(m_physDevices.Selected().m_physDevice, &DeviceCreateInfo, NULL, &m_device);
	CHECK_VK_RESULT(res, "Create device\n");

	printf("\nDevice created\n");
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


static u32 ChooseNumImages(const VkSurfaceCapabilitiesKHR& Capabilities)
{
	u32 RequestedNumImages = Capabilities.minImageCount + 1;

	int FinalNumImages = 0;

	if ((Capabilities.maxImageCount > 0) && (RequestedNumImages > Capabilities.maxImageCount)) {
		FinalNumImages = Capabilities.maxImageCount;
	}
	else {
		FinalNumImages = RequestedNumImages;
	}

	return FinalNumImages;
}


static VkSurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<VkSurfaceFormatKHR>& SurfaceFormats)
{
	for (int i = 0; i < SurfaceFormats.size(); i++) {
		if ((SurfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
			(SurfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)) {
			return SurfaceFormats[i];
		}
	}

	return SurfaceFormats[0];
}

VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags,
	VkImageViewType ViewType, u32 LayerCount, u32 mipLevels)
{
	VkImageViewCreateInfo ViewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = Image,
		.viewType = ViewType,
		.format = Format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = {
			.aspectMask = AspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = LayerCount
		}
	};

	VkImageView ImageView;
	VkResult res = vkCreateImageView(Device, &ViewInfo, NULL, &ImageView);
	CHECK_VK_RESULT(res, "vkCreateImageView");
	return ImageView;
}


void VulkanCore::CreateSwapChain()
{
	const VkSurfaceCapabilitiesKHR& SurfaceCaps = m_physDevices.Selected().m_surfaceCaps;

	u32 NumImages = ChooseNumImages(SurfaceCaps);

	const std::vector<VkPresentModeKHR>& PresentModes = m_physDevices.Selected().m_presentModes;
	VkPresentModeKHR PresentMode = ChoosePresentMode(PresentModes);

	m_swapChainSurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_physDevices.Selected().m_surfaceFormats);

	VkSwapchainCreateInfoKHR SwapChainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = m_surface,
		.minImageCount = NumImages,
		.imageFormat = m_swapChainSurfaceFormat.format,
		.imageColorSpace = m_swapChainSurfaceFormat.colorSpace,
		.imageExtent = SurfaceCaps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &m_queueFamily,
		.preTransform = SurfaceCaps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = PresentMode,
		.clipped = VK_TRUE
	};

	VkResult res = vkCreateSwapchainKHR(m_device, &SwapChainCreateInfo, NULL, &m_swapChain);
	CHECK_VK_RESULT(res, "vkCreateSwapchainKHR\n");

	printf("Swap chain created\n");

	uint NumSwapChainImages = 0;
	res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, NULL);
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");
	assert(NumImages <= NumSwapChainImages);

	printf("Requested %d images, created %d images\n", NumImages, NumSwapChainImages);

	m_images.resize(NumSwapChainImages);
	m_imageViews.resize(NumSwapChainImages);

	res = vkGetSwapchainImagesKHR(m_device, m_swapChain, &NumSwapChainImages, m_images.data());
	CHECK_VK_RESULT(res, "vkGetSwapchainImagesKHR\n");

	int LayerCount = 1;
	int MipLevels = 1;
	for (u32 i = 0; i < NumSwapChainImages; i++) {
		m_imageViews[i] = CreateImageView(m_device, m_images[i], m_swapChainSurfaceFormat.format,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, LayerCount, MipLevels);
	}
}


void VulkanCore::CreateCommandBufferPool()
{
	VkCommandPoolCreateInfo cmdPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = m_queueFamily
	};

	VkResult res = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, NULL, &m_cmdBufPool);
	CHECK_VK_RESULT(res, "vkCreateCommandPool\n");

	printf("Command buffer pool created\n");
}


void VulkanCore::CreateCommandBuffers(u32 count, VkCommandBuffer* cmdBufs)
{
	VkCommandBufferAllocateInfo cmdBufAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = m_cmdBufPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = count
	};

	VkResult res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, cmdBufs);
	CHECK_VK_RESULT(res, "vkAllocateCommandBuffers\n");

	printf("%d command buffers created\n", count);
}


void VulkanCore::FreeCommandBuffers(u32 Count, const VkCommandBuffer* pCmdBufs)
{
	m_queue.WaitIdle();
	vkFreeCommandBuffers(m_device, m_cmdBufPool, Count, pCmdBufs);
}


VkRenderPass VulkanCore::CreateSimpleRenderPass()
{
	VkAttachmentDescription AttachDesc = {
		.flags = 0,
		.format = m_swapChainSurfaceFormat.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference AttachRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription SubpassDesc = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &AttachRef,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkRenderPassCreateInfo RenderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &AttachDesc,
		.subpassCount = 1,
		.pSubpasses = &SubpassDesc,
		.dependencyCount = 0,
		.pDependencies = NULL
	};

	VkRenderPass RenderPass;

	VkResult res = vkCreateRenderPass(m_device, &RenderPassCreateInfo, NULL, &RenderPass);
	CHECK_VK_RESULT(res, "vkCreateRenderPass\n");

	printf("Created a simple render pass\n");

	return RenderPass;
}


std::vector<VkFramebuffer> VulkanCore::CreateFramebuffer(VkRenderPass RenderPass)
{
	std::vector<VkFramebuffer> frameBuffers;
	frameBuffers.resize(m_images.size());

	int WindowWidth, WindowHeight;
	glfwGetWindowSize(m_pWindow, &WindowWidth, &WindowHeight);

	VkResult res;

	for (uint i = 0; i < m_images.size(); i++) {

		VkFramebufferCreateInfo fbCreateInfo = {};
		fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbCreateInfo.renderPass = RenderPass;
		fbCreateInfo.attachmentCount = 1;
		fbCreateInfo.pAttachments = &m_imageViews[i];
		fbCreateInfo.width = WindowWidth;
		fbCreateInfo.height = WindowHeight;
		fbCreateInfo.layers = 1;

		res = vkCreateFramebuffer(m_device, &fbCreateInfo, NULL, &frameBuffers[i]);
		CHECK_VK_RESULT(res, "vkCreateFramebuffer\n");
	}

	printf("Framebuffers created\n");

	return frameBuffers;
}


void VulkanCore::DestroyFramebuffers(std::vector<VkFramebuffer>& Framebuffers)
{
	for (int i = 0; i < Framebuffers.size(); i++) {
		vkDestroyFramebuffer(m_device, Framebuffers[i], NULL);
	}
}


VkPipeline VulkanCore::CreatePipeline(VkRenderPass RenderPass, VkShaderModule vs, VkShaderModule fs)
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
	pipelineIACreateInfo.primitiveRestartEnable = VK_FALSE;

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
	rastCreateInfo.cullMode = VK_CULL_MODE_NONE;
	rastCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rastCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo pipelineMSCreateInfo = {};
	pipelineMSCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMSCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineMSCreateInfo.sampleShadingEnable = VK_FALSE;
	pipelineMSCreateInfo.minSampleShading = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachState = {};
	blendAttachState.blendEnable = VK_FALSE;
	blendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
	blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendCreateInfo.logicOpEnable = VK_FALSE;
	blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	blendCreateInfo.attachmentCount = 1;
	blendCreateInfo.pAttachments = &blendAttachState;

	VkPipelineLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pSetLayouts = NULL
	};

	VkResult res = vkCreatePipelineLayout(m_device, &layoutInfo, NULL, &m_pipelineLayout);
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
	pipelineInfo.renderPass = RenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline Pipeline;
	res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &Pipeline);
	CHECK_VK_RESULT(res, "vkCreateGraphicsPipelines\n");

	printf("Graphics pipeline created\n");

	return Pipeline;
}


void VulkanCore::DestroyPipeline(VkPipeline Pipeline)
{
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, NULL);
	vkDestroyPipeline(m_device, Pipeline, NULL);
}

}
