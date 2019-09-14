#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


class VkError : public std::runtime_error
{
public:
	VkError(const char* msg, const VkResult errcode)
		: std::runtime_error(msg), status(errcode)
	{
	}

	const VkResult status;
};

class VkLayerNotFoundError : public std::exception
{
public:
	VkLayerNotFoundError(const char* layerName)
		: std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + m_layerName)
	{
	}

	const char* what() const noexcept override
	{
		return m_errorMessage.c_str();
	}

private:
	const char* m_layerName;
	const std::string m_errorMessage;
};

class HelloTriangleApp
{
public:	
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	static constexpr int WIDTH = 640;
	static constexpr int HEIGHT = 480;
	static constexpr const char* NAME = "triangle";
#ifdef NDEBUG
	static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
	static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

	const std::vector<const char*> VALIDATION_LAYERS = 
	{
		"VK_LAYER_LUNARG_standard_validation"
	};

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, nullptr, nullptr);
	}

	void createInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = NAME;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		VkInstanceCreateInfo instInfo = {};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pApplicationInfo = &appInfo;
		instInfo.enabledExtensionCount = glfwExtensionCount;
		instInfo.ppEnabledExtensionNames = glfwExtensions;
		
		if (ENABLE_VALIDATION_LAYERS)
		{
			instInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}
		else
		{
			instInfo.enabledLayerCount = 0;
		}

		VkResult status = vkCreateInstance(&instInfo, nullptr, &m_instance);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create instance!", status);
		}
	}

	void checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> layers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

		for (auto layerName : VALIDATION_LAYERS)
		{
			bool isFound = false;
			for (const auto& layerProperties : layers)
			{
				if (!strcmp(layerName, layerProperties.layerName))
				{
					isFound = true;
					break;
				}
			}

			if (!isFound)
			{
				throw VkLayerNotFoundError(layerName);
			}
		}	
	}

	void initVulkan()
	{
		createInstance();
		checkValidationLayerSupport();
	}

	void mainLoop()
	{
		while(!glfwWindowShouldClose(m_window))
		{
			
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		vkDestroyInstance(m_instance, nullptr);

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	VkInstance m_instance;
	GLFWwindow* m_window;
};

int main() {
	auto app = HelloTriangleApp();

	try
	{
		app.run();
	}
	catch (const VkError& ex)
	{
		std::cerr << "Error while running app: " << ex.what() << "(" << ex.status << ")\n";
		return EXIT_FAILURE;
	}
	catch (const VkLayerNotFoundError& err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
