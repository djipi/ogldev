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


#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "ogldev_vulkan_core.h"
#include "ogldev_vulkan_shader.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080


void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}


class VulkanApp
{
public:

	VulkanApp(GLFWwindow* pWindow) : m_vkCore(pWindow)
	{
	}

	~VulkanApp()
	{
	}

	void Init(const char* pAppName)
	{
		int NumUniformBuffers = 1;
		m_vkCore.Init(pAppName, NumUniformBuffers, sizeof(UniformData));
		CreateCommandBuffer();		
		CreateSemaphores();
		CreateVertexBuffer();
		CreateShaders();
		CreatePipeline();
		RecordCommandBuffers();
	//	exit(0);
	}

	void RenderScene()
	{
		uint32_t ImageIndex = m_vkCore.AcquireNextImage(m_presentCompleteSem);

		UpdateUniformBuffers(ImageIndex);

		m_vkCore.Submit(&m_cmdBufs[ImageIndex], m_presentCompleteSem, m_renderCompleteSem);

		m_vkCore.QueuePresent(ImageIndex, m_renderCompleteSem);
	}

private:

	struct UniformData {
		Matrix4f WVP;
	};

	void CreateCommandBuffer()
	{
		m_cmdBufs.resize(m_vkCore.GetNumImages());
		m_vkCore.CreateCommandBuffers(m_vkCore.GetNumImages(), &m_cmdBufs[0]);
			
		printf("Created command buffers\n");
	}


	void RecordCommandBuffers()
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkClearColorValue clearColor = { 164.0f / 256.0f, 30.0f / 256.0f, 34.0f / 256.0f, 0.0f };
		VkClearValue clearValue = {};
		clearValue.color = clearColor;

		VkImageSubresourceRange imageRange = {};
		imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageRange.levelCount = 1;
		imageRange.layerCount = 1;

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vkCore.GetRenderPass();
		renderPassInfo.renderArea.offset.x = 0;
		renderPassInfo.renderArea.offset.y = 0;
		renderPassInfo.renderArea.extent.width = WINDOW_WIDTH;
		renderPassInfo.renderArea.extent.height = WINDOW_HEIGHT;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		for (uint i = 0; i < m_cmdBufs.size(); i++) {
			VkResult res = vkBeginCommandBuffer(m_cmdBufs[i], &beginInfo);
			CHECK_VK_RESULT(res, "vkBeginCommandBuffer\n");

			renderPassInfo.framebuffer = m_vkCore.GetFramebuffers()[i];

			vkCmdBeginRenderPass(m_cmdBufs[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_cmdBufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

			vkCmdBindDescriptorSets(m_cmdBufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkCore.GetPipelineLayout(), 0, 1, &m_vkCore.GetDescriptorSet(i), 0, nullptr);

			vkCmdDraw(m_cmdBufs[i], 3, 1, 0, 0);

			vkCmdEndRenderPass(m_cmdBufs[i]);

			res = vkEndCommandBuffer(m_cmdBufs[i]);
			CHECK_VK_RESULT(res, "vkEndCommandBuffer\n");
		}

		printf("Command buffers recorded\n");
	}

	void UpdateUniformBuffers(uint32_t ImageIndex)
	{
		static float foo = 0.0f;
		Matrix4f Rotate;
		Rotate.InitRotateTransform(0.0f, 0.0f, foo);
		foo += 0.01f;
		m_vkCore.UpdateUniformBuffer(ImageIndex, 0, &Rotate.m[0][0], sizeof(Matrix4f));
	}

	void CreateSemaphores()
	{
		m_presentCompleteSem = m_vkCore.CreateSemaphore();
		m_renderCompleteSem = m_vkCore.CreateSemaphore();
	}


	void CreateVertexBuffer()
	{
		std::vector<Vector3f> Vertices = {(-1.0f, -1.0f, 0.0f),
						 (1.0f, -1.0f, 0.0f),
						 (0.0f,  1.0f, 0.0f), };

		VkBuffer vb = m_vkCore.CreateVertexBuffer(Vertices);
	}

	void CreateShaders()
	{
		//m_vs = OgldevVK::CreateShaderModuleFromBinary(m_vkCore.GetDevice(), "Shaders/vs.spv");
		m_vs = OgldevVK::CreateShaderModuleFromText(m_vkCore.GetDevice(), "Shaders/test.vs");

		m_fs = OgldevVK::CreateShaderModuleFromBinary(m_vkCore.GetDevice(), "Shaders/fs.spv");
	}

	void CreatePipeline()
	{
		m_pipeline = m_vkCore.CreatePipeline(m_vs, m_fs);
	}

	OgldevVK::VulkanCore m_vkCore;	
	std::vector<VkCommandBuffer> m_cmdBufs;
	VkSemaphore m_renderCompleteSem;
	VkSemaphore m_presentCompleteSem;
	VkShaderModule m_vs;
	VkShaderModule m_fs;
	VkPipeline m_pipeline;
};


#define APP_NAME "Tutorial 02"

int main(int argc, char* argv[])
{
	if (!glfwInit()) {
		return 1;
	}

	if (!glfwVulkanSupported()) {
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, NULL, NULL);
	
	if (!pWindow) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(pWindow, GLFW_KeyCallback);

	VulkanApp App(pWindow);
	App.Init(APP_NAME);

	while (!glfwWindowShouldClose(pWindow)) {
		App.RenderScene();
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}