#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isReady() const 
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

constexpr int WIDTH = 640;
constexpr int HEIGHT = 480;
constexpr const char* NAME = "triangle";

#ifdef NDEBUG
	static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
	static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char*> VALIDATION_LAYERS = 
{
	"VK_LAYER_LUNARG_standard_validation"
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT flags,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* userData)
{
	if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << data->pMessage << std::endl;
	}
	else
	{
		std::cerr << data->pMessage << std::endl;
	}
}


void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info)
{
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
							VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 	  | 
						VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
						VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info.pfnUserCallback = debugCallback;
	info.pUserData = nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(
	VkInstance instance, 
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT& messenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, &messenger);
	}
	else 
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT& messenger) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
		func(instance, messenger, pAllocator);
	}
	else
	{
		std::cerr << "Could not destroy debug messenger" << std::endl;
	}
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR renderSurface) 
{
    QueueFamilyIndices indices;
    
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());
	
	VkBool32 presentSupport = false;
		
	uint32_t queueIdx = 0;
	for (const auto& family : families)
	{
		if (family.queueCount > 0)
		{
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = queueIdx;
			}

			vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIdx, renderSurface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = queueIdx;
			}
		}

		queueIdx++;
		if (indices.isReady())
		{
			break;
		}
	}

    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR renderSurface)
{
	return findQueueFamilies(dev, renderSurface).isReady();
}

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
	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(WIDTH, HEIGHT, NAME, nullptr, nullptr);
	}


	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t extCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
		
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extCount);
		if (ENABLE_VALIDATION_LAYERS)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
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

		VkInstanceCreateInfo instInfo = {};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pApplicationInfo = &appInfo;
		auto exts = getRequiredExtensions();
		instInfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
		instInfo.ppEnabledExtensionNames = exts.data();
		
		if (ENABLE_VALIDATION_LAYERS)
		{
			instInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
			populateDebugMessengerCreateInfo(debugInfo);
			instInfo.pNext = &debugInfo;
		}
		else
		{
			instInfo.enabledLayerCount = 0;
			instInfo.pNext = nullptr;
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

	void setupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		populateDebugMessengerCreateInfo(createInfo);
		auto status = CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, m_debugMessenger);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create debug messenger", status);
		}
	}

	void createRenderSurface()
	{
		auto status = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_renderSurface);
		if (status != VK_SUCCESS)
		{
			throw VkError("Could not create render surface", status);
		}
	}

	void pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
		
		if (deviceCount == 0) 
		{
    		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (const auto& device : devices) 
		{
		    if (isDeviceSuitable(device, m_renderSurface)) 
			{
		        m_physicalDevice = device;
		        break;
		    }
		}

		if (m_physicalDevice == VK_NULL_HANDLE) 
		{
		    throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	void createLogicalDevice()
	{
		auto indices = findQueueFamilies(m_physicalDevice, m_renderSurface);
		
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		for (const auto& family : uniqueQueueFamilies)
		{
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

		if (ENABLE_VALIDATION_LAYERS) 
		{
		    createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		    createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		} 
		else 
		{
		    createInfo.enabledLayerCount = 0;
		}

		auto status = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create a logical device", status);
		}

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void initVulkan()
	{
		createInstance();
		if (ENABLE_VALIDATION_LAYERS) 
		{
				checkValidationLayerSupport();
				setupDebugMessenger();
		}
		createRenderSurface();
		pickPhysicalDevice();
		createLogicalDevice();
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
		vkDestroyDevice(m_device, nullptr);

		if (ENABLE_VALIDATION_LAYERS)
		{
			DestroyDebugUtilsMessengerEXT(m_instance, nullptr, m_debugMessenger);
		}		
		vkDestroySurfaceKHR(m_instance, m_renderSurface, nullptr);
		vkDestroyInstance(m_instance, nullptr);

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkQueue m_presentQueue;
	VkSurfaceKHR m_renderSurface;
	GLFWwindow* m_window = nullptr;
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
	catch (const std::exception& err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
