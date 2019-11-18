#include "callbacks.h"

#include <iostream>
#include <stdexcept>

void onKeyPress(GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

void onGLFWError(const int error, const char* description)
{
	std::cerr << description << '(' << error << ")\n";
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT flags,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *userData)
{
	switch(severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			std::cout << "INFO:" << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			std::cout << "VERBOSE:" << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			std::cerr << "WARNING:" << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			std::cerr << "ERROR:" << std::endl;
			break;
		default:
			std::cerr << "UNKNOWN (" << severity << "):" << std::endl;
	}
	
	std::cerr << '\t' << data->pMessageIdName << " - " << data->pMessage << std::endl;
}