// VulkanProject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#include <math.h>
#include <string.h>

#include "Vulkan.h"
#include "Window.h"
#include "Globals.h"
#include "Util.h"
#include "Presentation.h"

int resizeW = -1;
int resizeH = -1;

App* globalApplication;

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
        globalApplication->scene.camera.rotation_x = dragRotX + ((mouseY - dragY)/10);
        globalApplication->scene.camera.rotation_y = dragRotY + ((mouseX - dragX)/10);
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int pressed, int idk)
{
    if(button == 0)
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
ULONGLONG pressedKeys[] = {0,0,0,0}; // WASD 87 65 83 68
void setKey(int index, int pressed)
{
    if (pressed == 0) pressedKeys[index] = 0;
    if (pressed == 1) pressedKeys[index] = GetTickCount64();
}

void key_callback(GLFWwindow* window, int key, int scanCode, int action, int mods)
{
    int pressed = -1;
    if (action == GLFW_PRESS) {
        pressed = 1;
    }
    if (action == GLFW_RELEASE) {
        pressed = 0;
    }

    if (pressed == -1) return;

    switch (key)
    {
		case 87: {
            setKey(0, pressed);
            break;
		}
        case 65: {
            setKey(1, pressed);
            break;
        }
        case 83: {
            setKey(2, pressed);
            break;
        }
        case 68: {
            setKey(3, pressed);
            break;
        }
        default:return;
    }
}

void updatePosition()
{
    ULONGLONG diffL;
    ULONGLONG now = GetTickCount64();
    float diff;
    // W
    if(pressedKeys[0])
    {
        diffL = now - pressedKeys[0];
        diff = (float)diffL;
        float zAdd = diff / 2000.f * 5;
        globalApplication->scene.camera.pos[2] += zAdd;
        pressedKeys[0] = now;
    }
    // A
    if (pressedKeys[1])
    {
        diffL = now - pressedKeys[1];
        diff = (float)diffL;
        float xSub = diff / 2000.f * 5;
        globalApplication->scene.camera.pos[0] -= xSub;
        pressedKeys[1] = now;
    }
    // S
    if (pressedKeys[2])
    {
        diffL = now - pressedKeys[2];
        diff = (float)diffL;
        float zSub = diff / 2000.f * 5;
        globalApplication->scene.camera.pos[2] -= zSub;
        pressedKeys[2] = now;
    }
    // D
    if (pressedKeys[3])
    {
        diffL = now - pressedKeys[3];
        diff = (float)diffL;
        float xAdd = diff / 2000.f * 5;
        globalApplication->scene.camera.pos[0] += xAdd;
        pressedKeys[3] = now;
    }
}


int main()
{
    App app;
    memset(&app, 0, sizeof(app));
    App* appPtr = &app;
    globalApplication = appPtr;
    app.vk_info.rasterize = VK_TRUE;
    init_scene(&app.scene);
    if(init_window(&app.window) && init_vulkan(&app.vk_info, &app.window, &app.scene.scene_data))
    {
        glfwSetFramebufferSizeCallback(app.window, resize_callback);
        glfwSetCursorPosCallback(app.window, mouse__move_callback);
        glfwSetMouseButtonCallback(app.window, mouse_button_callback);
        glfwSetKeyCallback(app.window, key_callback);
        set_global_buffers(&app.vk_info, &app.scene);
        while (!glfwWindowShouldClose(app.window)) {
            glfwPollEvents();
            if(WINDOW_WIDTH != 0 && WINDOW_HEIGHT != 0)
            {
                updatePosition();
                if (drawFrame(&app.vk_info, &app.scene) == FAILURE)
                {
                    break;
                }
            }

            if (resizeW >= 0 || resizeH >= 0)
            {
                create_or_resize_swapchain(&app.vk_info, &app.window, resizeW, resizeH, &app.scene.scene_data);
                WINDOW_WIDTH = resizeW;
                WINDOW_HEIGHT = resizeH;
                resizeW = -1;
                resizeH = -1;
            }
        }
        destroy_vulkan(&app.vk_info);
        destroy_window(app.window);
        int a = 5;
        return 0;
    }
    printf("Init failed\n");
    destroy_vulkan(&app.vk_info);
    destroy_window(app.window);
    return -1;
}
 