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

#include "ogldev_vulkan_util.h"
#include "ogldev_vulkan_device.h"
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

class VulkanCore {

public:

	VulkanCore();

	~VulkanCore();

	void Init(const char* pAppName, GLFWwindow* pWindow);

	VkRenderPass CreateSimpleRenderPass();

	std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass RenderPass) const;

	void DestroyFramebuffers(std::vector<VkFramebuffer>& Framebuffers);

	VkDevice& GetDevice() { return m_device; }

	int GetNumImages() const { return (int)m_images.size(); }

	const VkImage& GetImage(int Index) const;

	VulkanQueue* GetQueue() { return &m_queue; }

	u32 GetQueueFamily() const { return m_queueFamily; }

	void CreateCommandBuffers(u32 Count, VkCommandBuffer* pCmdBufs);

	void FreeCommandBuffers(u32 Count, const VkCommandBuffer* pCmdBufs);

	BufferAndMemory CreateVertexBuffer(const void* pVertices, size_t Size);

	std::vector<BufferAndMemory> CreateUniformBuffers(size_t Size);

	void GetFramebufferSize(int& Width, int& Height) const;

private:

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface();
	void CreateDevice();
	void CreateSwapChain();
	void CreateCommandBufferPool();	
	BufferAndMemory CreateUniformBuffer(size_t Size);

	u32 GetMemoryTypeIndex(u32 memTypeBits, VkMemoryPropertyFlags memPropFlags);

	void CopyBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);

	BufferAndMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

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
};

}