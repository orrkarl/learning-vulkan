#include "callbacks.h"

#include <iostream>
#include <iomanip>
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
			std::cout << std::setw(10) << std::left << "INFO:" << data->pMessage << " (" << data->pMessageIdName << ')' << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			std::cout << std::setw(10) << std::left << "VERBOSE:" << data->pMessage << " (" << data->pMessageIdName << ')' << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			std::cerr << std::setw(10) << std::left << "WARNING:" << data->pMessage << " (" << data->pMessageIdName << ')' << std::endl;
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			std::cerr << std::setw(10) << std::left << "ERROR:" << data->pMessage << " (" << data->pMessageIdName << ')' << std::endl;
			break;
		default:
			std::cerr << "UNKNOWN (" << severity << "):" << data->pMessage << " (" << data->pMessageIdName << ')' << std::endl;
	}
}