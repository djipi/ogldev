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

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "ogldev_vulkan_core.h"

namespace OgldevVK {

class GraphicsPipeline {

public:

	GraphicsPipeline(VkDevice Device,
					int WindowWidth,
					int WindowHeight,
					VkRenderPass RenderPass,
					VkShaderModule vs,
					VkShaderModule fs,
					VkBuffer VB,
					size_t VBSize,
					VkBuffer IB,
					size_t IBSize,
					int NumImages,
					std::vector<BufferAndMemory>& UniformBuffers,
					int UniformDataSize,
					const VulkanTexture* pTex);

	~GraphicsPipeline();

	void Bind(VkCommandBuffer CmdBuf, int ImageIndex);

private:

	void CreateDescriptorPool(int NumImages);
	void CreateDescriptorSet(int NumImages, const VkBuffer& VB, size_t VBSize, const VkBuffer& IB, size_t IBSize,
		std::vector<BufferAndMemory>& UniformBuffers, int UniformDataSize, const VulkanTexture* pTex);

	VkDevice m_device = NULL;
	VkPipeline m_pipeline = NULL;
	VkPipelineLayout m_pipelineLayout = NULL;
	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_descriptorSetLayout;
	std::vector<VkDescriptorSet> m_descriptorSets;
};
}
