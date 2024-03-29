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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "ogldev_math_3d.h"

#include "ogldev_vulkan_util.h"
#include "ogldev_vulkan_device.h"
#include "ogldev_vulkan_texture.h"
#include "ogldev_vulkan_queue.h"

namespace OgldevVK {


class BufferAndMemory {
public:
	BufferAndMemory() {}
	BufferAndMemory(VkDevice* pDevice) : m_pDevice(pDevice) {}

	VkBuffer m_buffer = NULL;
	VkDeviceMemory m_mem = NULL;
	VkDeviceSize m_allocationSize = 0;

	void Update(const void* pData, size_t Size);

private:
	VkDevice* m_pDevice = NULL;
};


struct VulkanTexture {
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_mem = VK_NULL_HANDLE;
	VkImageView m_view = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};


class VulkanCore {
public:
	VulkanCore();
	~VulkanCore();

	void Init(const char* pAppName, GLFWwindow* pWindow, int NumUniformBuffers, size_t UniformDataSize);

	VkRenderPass CreateSimpleRenderPass();

	VkDevice& GetDevice() { return m_device; }

	int GetNumImages() const { return (int)m_images.size(); }

	std::vector<VkImage>& GetImages() { return m_images;  }

	VulkanQueue& GetQueue() { return m_queue; }

	const std::vector<VkFramebuffer>& GetFramebuffers() const { return m_fbs; }

	VkBuffer CreateVertexBuffer(const void* pVertices, size_t Size);

	BufferAndMemory CreateUniformBuffer(int Size);

	void CreateTexture(const char* filename, VulkanTexture& Tex);
	void DestroyTexture(VulkanTexture& Tex);

	void UpdateUniformBuffer(int ImageIndex, int UniformBufferIndex, const void* pData, size_t Size);

	void CreateCommandBuffers(u32 Count, VkCommandBuffer* pCmdBufs);
	void FreeCommandBuffers(u32 Count, const VkCommandBuffer* pCmdBufs);

	VkPipeline CreatePipeline(VkRenderPass RenderPass, VkShaderModule vs, VkShaderModule fs, const VulkanTexture& Tex);
	void DestroyPipeline(VkPipeline Pipeline);

	VkPipelineLayout& GetPipelineLayout() { return m_pipelineLayout; }

	VkDescriptorSet& GetDescriptorSet(int i) { return m_descriptorSets[i]; }

private:

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface();
	void CreateDevice();
	void CreateSwapChain();	
	void CreateFramebuffer(VkRenderPass RenderPass);
	void CreateCommandBufferPool();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSet(const VulkanTexture& Tex);

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

	void UploadBufferData(const VkDeviceMemory& BufferMemory, VkDeviceSize DeviceOffset, const void* pData, const size_t DataSize);

	void TransitionImageLayout(VkImage& Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, u32 LayerCount, u32 MipLevels);

	void TransitionImageLayoutCmd(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, u32 LayerCount, u32 MipLevels);

	VkCommandBuffer BeginSingleUseCommand();

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
	VulkanQueue m_queue;	
	std::vector<VkFramebuffer> m_fbs;
	VkCommandPool m_cmdBufPool;
	VkCommandBuffer m_copyCmdBuf;
	VkPipelineLayout m_pipelineLayout;
	int m_numUniformBuffers = 0;
	size_t m_uniformDataSize = 0;
	std::vector< std::vector<BufferAndMemory> > m_uniformBuffers;
	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::vector<VkDescriptorSet> m_descriptorSets;
};
}