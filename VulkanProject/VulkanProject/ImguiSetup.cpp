#include "ImguiSetup.h"

#include <cstdlib>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")

#include <string>

#include "Util.h"
extern "C" {
	#include "VulkanUtil.h"
}
void check_result(VkResult res)
{
	if (res == VK_SUCCESS) return;
	throw "ImGui threw an error";
}
// see https://frguthmann.github.io/posts/vulkan_imgui/
// I've found the source to be sometimes kind of confusing as the post isn't a walkthrough or tutorial
// but kind of a review of an already implemented imgui integration into a vulkan project
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

void init_imgui(App* app, int width, int height) // see https://frguthmann.github.io/posts/vulkan_imgui/
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
void init_imgui_command_buffers(VkInfo* vk, Scene* scene, SceneSelection* scene_selection)
{
	// see https://frguthmann.github.io/posts/vulkan_imgui/
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = vk->queue_family_index;

	vkCreateCommandPool(vk->device, &pool_info, NULL, &vk->imgui_command_pool);
	if(vk->swapchain.imgui_frame_buffers)
	{
		for(uint32_t i = 0;i<vk->buffer_count; i++)
		{
			vkDestroyFramebuffer(vk->device, vk->swapchain.imgui_frame_buffers[i], nullptr);
		}
		free(vk->swapchain.imgui_frame_buffers);
		vk->swapchain.imgui_frame_buffers = nullptr;
	}
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
		update_imgui_commandBuffer(vk, scene, scene_selection, i);
	}
}

void update_imgui_commandBuffer(VkInfo* vk, Scene* scene, SceneSelection* scene_selection, uint32_t index)
{
	check_result(vkResetCommandBuffer(vk->imgui_command_buffers[index], 0));

	draw_imgui_frame(vk, scene, scene_selection);

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

void draw_imgui_frame(VkInfo* info, Scene* scene, SceneSelection* scene_selection)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Settings");

	bool enable = true;
	//ImGui::ShowDemoWindow(&enable);

	ImGui::Text("Framerate: %f", info->frameRate);
	ImGui::SliderFloat("FOV", &scene->camera.settings.fov, 1, 89);
	if (ImGui::CollapsingHeader("DEBUG")) {
		ImGui::Checkbox("Enable Debug", (bool*)&scene->camera.settings.debug);
		ImGui::SliderInt("Color Sensitvity", &scene->camera.settings.colorSensitivity, 5, 30);
		ImGui::BeginDisabled(scene->camera.settings.debug == 0);
		ImGui::Checkbox("UV", (bool*)&scene->camera.settings.displayUV);
		ImGui::Checkbox("TEX", (bool*)&scene->camera.settings.displayTex);
		ImGui::Checkbox("Triangles", (bool*)&scene->camera.settings.displayTriangles);
		ImGui::Checkbox("Lights", (bool*)&scene->camera.settings.displayLights);
		ImGui::Checkbox("Intersection T", (bool*)&scene->camera.settings.displayIntersectionT);
		ImGui::Checkbox("AABBs", (bool*)&scene->camera.settings.displayAABBs);

		ImGui::Checkbox("TraversalDepth", (bool*)&scene->camera.settings.displayTraversalDepth);
		ImGui::Checkbox("TraversalCount", (bool*)&scene->camera.settings.displayTraversalCount);
		ImGui::Checkbox("QueryCount", (bool*)&scene->camera.settings.displayQueryCount);

		ImGui::EndDisabled();
	}

	ImGui::Combo("combo", &scene_selection->nextScene, scene_selection->availableScenes, scene_selection->numScenes);

	ImGui::End();
	ImGui::Render();
}

bool hasEnding(std::string const& fullString, std::string const& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else {
		return false;
	}
}
// https://docs.microsoft.com/en-us/windows/win32/fileio/listing-the-files-in-a-directory
void get_available_scenes(SceneSelection* scene_selection)
{
	TCHAR text[] =  TEXT("../Scenes");

	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError = 0;
	
	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	StringCchLength(text, MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		return;
	}

	_tprintf(TEXT("\nTarget directory is %s\n\n"), text);

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, text);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		return;
	}

	// List all the files in the directory with some info about them.
	const std::string fileEnding(".vksc");
	const int extensionOffset = 6;

	const std::string defaultFile("default.vksc");
	int defaultSceneIndex = -1;

	char** names = static_cast<char**>(malloc(sizeof(char*) * 32));
	int numScenes = 0;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{

		}
		else
		{
#define BUFFER_SIZE 100
			size_t i;
			char* pMBBuffer = (char*)malloc(BUFFER_SIZE);
			const wchar_t* pWCBuffer = ffd.cFileName;

			wcstombs_s(&i, pMBBuffer, (size_t)BUFFER_SIZE,
				pWCBuffer, (size_t)BUFFER_SIZE - 1);

			std::string stringName(pMBBuffer);
			if(!hasEnding(stringName, ".vksc"))
			{
				free(pMBBuffer);
				continue;
			}

			if(stringName == defaultFile)
			{
				defaultSceneIndex = numScenes;
			}

			pMBBuffer[i - extensionOffset] = '\0';
			names[numScenes] = pMBBuffer;
			numScenes++;
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	printf("Available Scenes:\n");
	for(int i = 0;i<numScenes;i++)
	{
		printf(names[i]);
		printf("\n");
	}

	FindClose(hFind);

	scene_selection->availableScenes = names;
	scene_selection->numScenes = numScenes;
	scene_selection->nextScene = defaultSceneIndex;
	scene_selection->currentScene = defaultSceneIndex;
}

void resize_callback_imgui(VkInfo* vk, Scene* scene, SceneSelection* scene_selection)
{
	ImGui_ImplVulkan_SetMinImageCount(2);
	init_imgui_command_buffers(vk, scene, scene_selection);
}

void destroy_imgui_buffers(VkInfo* info)
{
	free(info->imgui_command_buffers);
	vkDestroyCommandPool(info->device, info->imgui_command_pool, nullptr);
	if (info->swapchain.imgui_frame_buffers)
	{
		for (uint32_t i = 0; i < info->buffer_count; i++)
		{
			vkDestroyFramebuffer(info->device, info->swapchain.imgui_frame_buffers[i], nullptr);
		}
		free(info->swapchain.imgui_frame_buffers);
		info->swapchain.imgui_frame_buffers = nullptr;
	}
}

void destroy_imgui(VkInfo* info, SceneSelection* scene_selection)
{
	destroy_imgui_buffers(info);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyRenderPass(info->device, info->imguiPass, nullptr);

	for(uint32_t i = 0;i<scene_selection->numScenes;i++)
	{
		free(scene_selection->availableScenes[i]);
	}
	free(scene_selection->availableScenes);
}
