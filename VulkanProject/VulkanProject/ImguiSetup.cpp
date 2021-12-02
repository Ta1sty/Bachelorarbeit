#include "ImguiSetup.h"

#include <cstdlib>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "Util.h"
extern "C" {
	#include "VulkanUtil.h"
}
void check_result(VkResult res)
{
	if (res == VK_SUCCESS) return;
	throw "ImGui threw an error";
}

void create_imgui_RenderPass(VkInfo* vk)
{
	VkAttachmentDescription attachment = {};
	attachment.format = vk->swapchain.format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment;
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	check_result(vkCreateRenderPass(vk->device, &info, nullptr, &vk->imguiPass));
}

void init_imgui(App* app, int width, int height)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForVulkan(app->window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = app->vk_info.instance;
	init_info.PhysicalDevice = app->vk_info.physical_device;
	init_info.Device = app->vk_info.device;
	init_info.QueueFamily = app->vk_info.queue_family_index;
	init_info.Queue = app->vk_info.graphics_queue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPool = app->vk_info.descriptor_pool;
	init_info.Allocator = VK_NULL_HANDLE;
	init_info.MinImageCount = 2;
	init_info.ImageCount = app->vk_info.buffer_count;
	init_info.CheckVkResultFn = check_result;

	create_imgui_RenderPass(&app->vk_info);

	ImGui_ImplVulkan_Init(&init_info, app->vk_info.imguiPass);
	ImGui::StyleColorsDark();

	VkCommandBuffer buf = beginSingleTimeCommands(&app->vk_info);
	ImGui_ImplVulkan_CreateFontsTexture(buf);
	endSingleTimeCommands(&app->vk_info, buf);
}
void init_imgui_command_buffers(VkInfo* vk, Scene* scene)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = vk->queue_family_index;

	vkCreateCommandPool(vk->device, &pool_info, NULL, &vk->imgui_command_pool);

	vk->swapchain.imgui_frame_buffers = static_cast<VkFramebuffer*>(malloc(sizeof(VkFramebuffer) * vk->buffer_count));

	for (uint32_t i = 0; i < vk->buffer_count; i++)
	{
		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = vk->imguiPass;
		info.attachmentCount = 1;
		info.pAttachments = &vk->swapchain.image_views[i];
		info.width = vk->swapchain.extent.width;
		info.height = vk->swapchain.extent.height;
		info.layers = 1;
		check_result(vkCreateFramebuffer(vk->device, &info, nullptr, &vk->swapchain.imgui_frame_buffers[i]));
	}

	vk->imgui_command_buffers = static_cast<VkCommandBuffer*>(malloc(sizeof(VkCommandBuffer) * vk->buffer_count));
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = vk->imgui_command_pool;
	commandBufferAllocateInfo.commandBufferCount = vk->buffer_count;
	vkAllocateCommandBuffers(vk->device, &commandBufferAllocateInfo, vk->imgui_command_buffers);

	for(uint32_t i = 0;i < vk->buffer_count;i++)
	{
		update_imgui_commandBuffer(vk, scene, i);
	}
}

void update_imgui_commandBuffer(VkInfo* vk, Scene* scene, uint32_t index)
{
	check_result(vkResetCommandBuffer(vk->imgui_command_buffers[index], 0));

	draw_imgui_frame(vk, scene);

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	check_result(vkBeginCommandBuffer(vk->imgui_command_buffers[index], &begin_info));


	VkRenderPassBeginInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = vk->imguiPass;
	info.framebuffer = vk->swapchain.imgui_frame_buffers[index];
	info.renderArea.extent = vk->swapchain.extent;
	info.clearValueCount = 1;
	info.pClearValues = &clearColor;
	vkCmdBeginRenderPass(vk->imgui_command_buffers[index], &info, VK_SUBPASS_CONTENTS_INLINE);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk->imgui_command_buffers[index]);

	vkCmdEndRenderPass(vk->imgui_command_buffers[index]);

	check_result(vkEndCommandBuffer(vk->imgui_command_buffers[index]));
}

void draw_imgui_frame(VkInfo* info, Scene* scene)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Settings");
	ImGui::SliderFloat("FOV", &scene->camera.fov, 1, 3000);
	ImGui::End();
	ImGui::Render();
}

void resize_callback_imgui(VkInfo* vk, Scene* scene)
{
	ImGui_ImplVulkan_SetMinImageCount(2);
	init_imgui_command_buffers(vk, scene);
}