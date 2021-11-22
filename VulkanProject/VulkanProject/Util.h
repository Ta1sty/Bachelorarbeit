#pragma once
#include <vulkan/vulkan_core.h>
extern int SUCCESS;
extern int FAILURE;

typedef void (*ExceptionCallback)(void);
extern ExceptionCallback exception_callback;

void check(VkResult result, char* errorMsg);
void check_b(VkBool32 boolean, char* errorMsg);
void error(char* err);
void setExceptionCallback(ExceptionCallback callback);