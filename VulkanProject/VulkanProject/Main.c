// VulkanProject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <corecrt_math_defines.h>
#include <math.h>
#include <string.h>

#include "Vulkan.h"
#include "Window.h"
#include "Globals.h"
#include "Util.h"
#include "Presentation.h"
#include "Raytrace.h"
#include "ImguiSetup.h"
#include "Shader.h"

int resizeW = -1;
int resizeH = -1;


App* globalApplication;
void exception_callback_impl(void)
{
	if(globalApplication!=NULL)
	{
        destroy_vulkan(&globalApplication->vk_info, &globalApplication->scene, &globalApplication->sceneSelection);
        destroy_scene(&globalApplication->scene);
        destroy_window(globalApplication->window);
	}
}

void resize_callback(GLFWwindow* window, int width, int height) {
    resizeW = width;
    resizeH = height;
}

int dragging = 0;
float mouseX = 0; // current mouse x
float mouseY = 0; // current mouse y
float dragX = 0; // mouseX where dragging started
float dragY = 0; // mouseY where dragging started
float dragRotX = 0; // rotation along x-axis when dragging started
float dragRotY = 0; // rotation along y-axis when dragging started
void mouse__move_callback (GLFWwindow* window, double x, double y)
{
    mouseX = x;
    mouseY = y;
    if(dragging)
    {
        globalApplication->scene.camera.rotation_x = dragRotX - ((mouseY - dragY)/10);
        globalApplication->scene.camera.rotation_y = dragRotY + ((mouseX - dragX)/10);
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int pressed, int idk)
{
    if (button == 0) {
        if (pressed == 1) {
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                globalApplication->scene.camera.settings.recordQueryTrace = 1;
                globalApplication->scene.camera.settings.pixelX = mouseX;
                globalApplication->scene.camera.settings.pixelY = mouseY;
            }
        }
        else {
            globalApplication->scene.camera.settings.recordQueryTrace = 0;
        }
    }

    if(button == 1)
    {
        if(pressed == 1)
        {
            dragging = 1;
            dragX = mouseX;
            dragY = mouseY;
            dragRotX = globalApplication->scene.camera.rotation_x;
            dragRotY = globalApplication->scene.camera.rotation_y;
        }
        if(pressed == 0)
        {
            dragging = 0;
        }
    }
}
double last_time = 0;
void updatePosition(GLFWwindow* window, Camera* camera)
{
    double now = glfwGetTime();
    double diff = now - last_time;
    last_time = now;

    float straight = 0;
    float sideways = 0;
    float up = 0;

    float spd = 1;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        spd *= 10;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        spd *= 100;
    }

    float dst = spd * (float)diff;
    straight += (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) * dst;
    straight -= (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) * dst;
    sideways -= (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) * dst;
    sideways += (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) * dst;
    up += (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) * dst;
    up -= (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) * dst;

    float y_radians = camera->rotation_y * (float)M_PI / 180.0f;
    float cos_y = cosf(y_radians), sin_y = sinf(y_radians);


    camera->pos[0] += sin_y * straight;
    camera->pos[2] -= cos_y * straight;

    camera->pos[1] += up;

    camera->pos[0] += cos_y * sideways;
    camera->pos[2] += sin_y * sideways;
}
void changeScene(App* app)
{
    printf("Switching to scene:\n");
    printf(app->sceneSelection.availableScenes[app->sceneSelection.nextScene]);
    printf("\n");
    vkDeviceWaitIdle(app->vk_info.device);

    Camera oldCam = app->scene.camera;

    destroy_shaders(&app->vk_info, &app->scene);
    destroy_scene(&app->scene);
    load_scene(&app->scene, app->sceneSelection.availableScenes[app->sceneSelection.nextScene]);

    destroy_imgui_buffers(&app->vk_info);
    create_or_resize_swapchain(&app->vk_info, &app->window, WINDOW_WIDTH, WINDOW_HEIGHT, &app->scene);
    resize_callback_imgui(&app->vk_info, &app->scene, &app->sceneSelection);

    set_global_buffers(&app->vk_info, &app->scene);
    printSceneSizes(&app->scene);
    app->scene.camera = oldCam;
    app->sceneSelection.currentScene = app->sceneSelection.nextScene;

}

int main()
{
    App app = {0};

    get_available_scenes(&app.sceneSelection);
    if (app.sceneSelection.numScenes == 0) {
        error("no scenes found");
    }

    App* appPtr = &app;
    globalApplication = appPtr;
    setExceptionCallback(exception_callback_impl);
    app.vk_info.rasterize = VK_TRUE;
    app.vk_info.vsync = 1;
    load_scene(&app.scene, app.sceneSelection.availableScenes[app.sceneSelection.nextScene]);
    init_window(&app.window);
    init_vulkan(&app.vk_info, &app.window, &app.scene);

	glfwSetFramebufferSizeCallback(app.window, resize_callback);
	glfwSetCursorPosCallback(app.window, mouse__move_callback);
	glfwSetMouseButtonCallback(app.window, mouse_button_callback);

    // everything else needs to be build before creating the swapchain
    create_or_resize_swapchain(&app.vk_info, &app.window, WINDOW_WIDTH, WINDOW_HEIGHT, &app.scene);

    init_imgui(&app, WINDOW_WIDTH, WINDOW_HEIGHT);
    init_imgui_command_buffers(&app.vk_info, &app.scene, &app.sceneSelection);
    // resize_callback_imgui(&app.vk_info, &app.scene, &app.sceneSelection);

    //app.scene.scene_data.numTriangles = min(app.scene.scene_data.numTriangles, 500);
    // scene renderes fluently for up to 1000 triangles when intersecting with linear time complexity
    // provided the same complexity one can allow for a total of 1000 intersection tests per ray
    // which means 1000 during BVH traversal, we expect a depth of log(n) for the BVH, so say we have 2^16 triangles
    // we will have a depth of 16. 
    set_global_buffers(&app.vk_info, &app.scene);
    printSceneSizes(&app.scene);
	while (!glfwWindowShouldClose(app.window)) {
		glfwPollEvents();
		if(WINDOW_WIDTH > 0 && WINDOW_HEIGHT > 0)
            {
                updatePosition(app.window, &app.scene.camera);
                drawFrame(&app.vk_info, &app.scene, &app.sceneSelection);
                compile_query_trace(&app.vk_info, &app.scene);
            }
		if (resizeW >= 0 || resizeH >= 0 || app.vk_info.reload != 0)
		{
            if (resizeW == -1 || resizeH == -1) {
                resizeH = WINDOW_HEIGHT;
                resizeW = WINDOW_WIDTH;
            }

            vkDeviceWaitIdle(app.vk_info.device);
			create_or_resize_swapchain(&app.vk_info, &app.window, resizeW, resizeH, &app.scene);
            if (resizeW != 0 && resizeH != 0) {
                destroy_imgui_buffers(&app.vk_info);
                resize_callback_imgui(&app.vk_info, &app.scene, &app.sceneSelection);
            }

			WINDOW_WIDTH = resizeW;
			WINDOW_HEIGHT = resizeH;

            resizeW = -1;
            resizeH = -1;
            app.vk_info.reload = 0;
		}
        if (app.sceneSelection.currentScene != app.sceneSelection.nextScene)
            changeScene(&app);
	}
	destroy_vulkan(&app.vk_info, &app.scene, &app.sceneSelection);
	destroy_scene(&app.scene);
	destroy_window(app.window);
    printf("\n");
    printf("Press ENTER to exit\n");
    getchar();
}