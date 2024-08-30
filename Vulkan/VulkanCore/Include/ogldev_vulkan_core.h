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

#pragma once

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "ogldev_vulkan_util.h"
#include "ogldev_vulkan_device.h"
#include "ogldev_vulkan_texture.h"
#include "ogldev_vulkan_queue.h"

namespace OgldevVK {


class BufferAndMemory {
public:
	BufferAndMemory() {}

	VkBuffer m_buffer = NULL;
	VkDeviceMemory m_mem = NULL;
	VkDeviceSize m_allocationSize = 0;

	void Update(VkDevice Device, const void* pData, size_t Size);

	void Destroy(VkDevice Device);
};


struct VulkanTexture {
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_mem = VK_NULL_HANDLE;
	VkImageView m_view = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};


struct SimpleMesh {
	VkDevice m_device = NULL;
	BufferAndMemory m_vb;
	BufferAndMemory m_ib;
	int m_numVertices = 0;
	int m_numIndices = 0;
	size_t m_vertexBufferSize = 0;
	size_t m_indexBufferSize = 0;

	void Destroy();
};

enum RenderPassType {
	RenderPassTypeDefault = 0x0,
	RenderPassTypeFirst = 0x01,
	RenderPassTypeLast = 0x02,
	RenderPassTypeOffscreen = 0x04,
	RenderPassTypeOffscreenInternal = 0x08,
};


class VulkanCore {

public:

	VulkanCore();

	~VulkanCore();

	void Init(const char* pAppName, GLFWwindow* pWindow);

	VkRenderPass CreateSimpleRenderPass(bool DepthEnabled, bool ClearColor, bool ClearDepth, RenderPassType RenderPassType) const;

	std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass RenderPass) const;

	void DestroyFramebuffers(std::vector<VkFramebuffer>& Framebuffers);

	VkDevice& GetDevice() { return m_device; }

	const PhysicalDevice& GetPhysicalDevice() { return m_physDevices.Selected(); }

	int GetNumImages() const { return (int)m_images.size(); }

	const VkImage& GetImage(int Index) const;

	VulkanQueue* GetQueue() { return &m_queue; }

	void CreateCommandBuffers(u32 Count, VkCommandBuffer* pCmdBufs);

	void FreeCommandBuffers(u32 Count, const VkCommandBuffer* pCmdBufs);

	BufferAndMemory CreateVertexBuffer(const void* pVertices, size_t Size, bool HostVisible = false);
	
	std::vector<BufferAndMemory> CreateUniformBuffers(size_t DataSize);

	void CreateTexture(const char* filename, VulkanTexture& Tex);
	
	void DestroyTexture(VulkanTexture& Tex);

	SimpleMesh LoadSimpleMesh(const char* pFilename);

	const VulkanTexture& GetDepthTexture() const { return m_depthTexture; }

	void GetFramebufferSize(int& Width, int& Height) const;

	void UploadBufferData(const VkDeviceMemory& BufferMemory, VkDeviceSize DeviceOffset, const void* pData, const size_t DataSize);

	GLFWwindow* GetWindow() const { return m_pWindow; }

	u32 GetQueueFamily() const { return m_queueFamily; }

	VkInstance GetInstance() const { return m_instance; }

private:

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface();
	void CreateDevice();
	void CreateSwapChain();
	void CreateCommandBufferPool();	
	BufferAndMemory CreateUniformBuffer(int Size);

	u32 GetMemoryTypeIndex(u32 memTypeBits, VkMemoryPropertyFlags memPropFlags);

	void CopyBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);

	VkDeviceSize CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties,
					          VkBuffer& buffer, VkDeviceMemory& bufferMemory);	

	void CreateTextureImageFromData(VulkanTexture& Tex, const void* pPixels, u32 ImageWidth, u32 ImageHeight,
									VkFormat TexFormat, u32 LayerCount, VkImageCreateFlags Flags);

	void CreateImage(VulkanTexture& Tex, u32 ImageWidth, u32 ImageHeight, VkFormat TexFormat, VkImageTiling ImageTiling, 
		             VkImageUsageFlags UsageFlags, VkMemoryPropertyFlagBits PropertyFlags, VkImageCreateFlags CreateFlags, u32 MipLevels);

	void UpdateTextureImage(VulkanTexture& Tex, u32 ImageWidth, u32 ImageHeight, VkFormat TexFormat, u32 LayerCount, const void* pPixels, VkImageLayout SourceImageLayout);

	void CopyBufferToImage(VkBuffer buffer, VkImage image, u32 ImageWidth, u32 ImageHeight, u32 LayerCount);

	void TransitionImageLayout(VkImage& Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, u32 LayerCount, u32 MipLevels);

	void TransitionImageLayoutCmd(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, u32 LayerCount, u32 MipLevels);

	VkCommandBuffer CreateAndBeginSingleUseCommand();

	void EndSingleTimeCommands(VkCommandBuffer CmdBuf);

	VkInstance m_instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	GLFWwindow* m_pWindow = NULL;
	VkSurfaceKHR m_surface = VK_NULL_HANDLE;
	VulkanPhysicalDevices m_physDevices;
	u32 m_queueFamily = 0;
	VkDevice m_device;
	VkSurfaceFormatKHR m_swapChainSurfaceFormat;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_images;	
	std::vector<VkImageView> m_imageViews;
	VkCommandPool m_cmdBufPool;
	VulkanQueue m_queue;
	VkCommandBuffer m_copyCmdBuf;
	VulkanTexture m_depthTexture;
};

}