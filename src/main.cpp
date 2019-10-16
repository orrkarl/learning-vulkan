#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "config.h"
#include "util/util.h"

class VkError : public std::runtime_error
{
public:
	VkError(const char *msg, const VkResult errcode)
		: std::runtime_error(msg), status(errcode)
	{
	}

	const VkResult status;
};

class VkLayerNotFoundError : public std::exception
{
public:
	VkLayerNotFoundError(const char *layerName)
		: std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + layerName)
	{
	}

	const char *what() const noexcept override
	{
		return m_errorMessage.c_str();
	}

	const char *layer() const
	{
		return m_layerName;
	}

private:
	const char *m_layerName;
	const std::string m_errorMessage;
};

class VkExtensionNotFoundError : public std::runtime_error
{
public:
	VkExtensionNotFoundError(const char *extensionName)
		: std::runtime_error(std::string("Validation layer not found: ") + extensionName), m_extensionName(extensionName)
	{
	}

	const char *extensions() const
	{
		return m_extensionName;
	}

private:
	const char *m_extensionName;
};

const std::array<Vertex, 4> g_vertecies
{
    Vertex{ {-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	Vertex{ { 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    Vertex{ { 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    Vertex{ {-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::array<uint16_t, 6> g_indices
{
	0, 1, 2, 2, 3, 0
};

class HelloTriangleApp
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
	}

	~HelloTriangleApp()
	{
		m_device->waitIdle();

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}

private:
	static void glfwFramebufferResize(GLFWwindow* window, int w, int h)
	{
		auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
		app->m_windowSizeChanged = true;
	}

	void initWindow()
	{
		glfwInit();
		glfwSetErrorCallback(&onGLFWError);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_window = glfwCreateWindow(config::WIDTH, config::HEIGHT, config::NAME, nullptr, nullptr);
		glfwSetWindowUserPointer(m_window, this);
		glfwSetKeyCallback(m_window, &onKeyPress);
		glfwSetFramebufferSizeCallback(m_window, &glfwFramebufferResize);
	}

	void createInstance()
	{
		vk::ApplicationInfo appInfo(
			config::NAME,
			VK_MAKE_VERSION(1, 0, 0),
			"No Engine",
			VK_MAKE_VERSION(1, 0, 0),
			VK_API_VERSION_1_0
		);

		auto exts = getRequiredExtensions();
		vk::InstanceCreateInfo instInfo(
			vk::InstanceCreateFlags(),
			&appInfo,
			config::VALIDATION_LAYERS.size(), config::VALIDATION_LAYERS.data(),
			exts.size(), exts.data()
		);

		vk::DebugUtilsMessengerCreateInfoEXT debugInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);
		instInfo.pNext = &debugInfo;
		
		m_instance = vk::createInstanceUnique(instInfo);
		m_dispatchDynamic = vk::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr);
	}

	void checkValidationLayerSupport()
	{
		auto layers = vk::enumerateInstanceLayerProperties();

		for (auto layerName : config::VALIDATION_LAYERS)
		{
			bool isFound = false;
			for (const auto &layerProperties : layers)
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
		vk::DebugUtilsMessengerCreateInfoEXT createInfo(
			vk::DebugUtilsMessengerCreateFlagsEXT(),
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			debugCallback, nullptr
		);

		m_debugMessenger = m_instance->createDebugUtilsMessengerEXTUnique(createInfo, nullptr, m_dispatchDynamic);
	}

	void createRenderSurface()
	{
		VkSurfaceKHR surface;
		auto status = glfwCreateWindowSurface(
			static_cast<VkInstance>(m_instance.get()), 
			m_window, 
			nullptr, 
			&surface
		);
		if (status != VK_SUCCESS)
		{
			throw VkError("Could not create render surface", status);
		}

		m_renderSurface = vk::UniqueSurfaceKHR(
			vk::SurfaceKHR(surface), 
			vk::ObjectDestroy(m_instance.get(), nullptr, vk::DispatchLoaderStatic())
		);
	}

	void pickPhysicalDevice()
	{
		auto devices = m_instance->enumeratePhysicalDevices();

		auto deviceCount = devices.size();
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		for (const auto &device : devices)
		{
			if (isDeviceSuitable(device, m_renderSurface.get(), config::DEVICE_EXTENSIONS))
			{
				m_physicalDevice = device;
				return;
			}
		}

		throw std::runtime_error("failed to find a suitable GPU!");
	}

	void createLogicalDevice()
	{
		auto indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

		std::set<uint32_t> uniqueQueueFamilies = {indices.graphics(), indices.present()};
		float queuePriority = 1.0f;
		vk::DeviceQueueCreateInfo queueCreateInfo(
			vk::DeviceQueueCreateFlags(),
			0, 
			1, &queuePriority
		);
		for (const auto &family : uniqueQueueFamilies)
		{
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures;

		vk::DeviceCreateInfo createInfo(
			vk::DeviceCreateFlags(),
			queueCreateInfos.size(), queueCreateInfos.data(),
			config::VALIDATION_LAYERS.size(), config::VALIDATION_LAYERS.data(),
			config::DEVICE_EXTENSIONS.size(), config::DEVICE_EXTENSIONS.data(),
			&deviceFeatures
		);
		m_device = m_physicalDevice.createDeviceUnique(createInfo);

		m_computeQueue = m_device->getQueue(indices.compute(), 0);
	}

	void createPresent()
	{
		m_present = Present(*m_device, m_physicalDevice, *m_renderSurface, m_window);
	}

	void createSyncObjects()
	{
		auto count = config::MAX_FRAMES_IN_FLIGHT;

		m_imageAvailable.resize(count);
		m_renderCompleted.resize(count);
		m_inFlightImages.resize(count);

		auto semaphoreInfo = vk::SemaphoreCreateInfo();
		vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlags(vk::FenceCreateFlagBits::eSignaled));

		for (auto i = 0u; i < count; ++i)
		{
			m_imageAvailable[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			m_renderCompleted[i] = m_device->createSemaphoreUnique(semaphoreInfo);
			m_inFlightImages[i] = m_device->createFenceUnique(fenceInfo);
		}
	}

	void recreatePresent()
	{
		int w = 0, h = 0;
		while (w == 0 and h == 0)
		{
			glfwGetFramebufferSize(m_window, &w, &h);
			glfwWaitEvents();
		}

		m_device->waitIdle();

		createPresent();
		m_graphics.update(m_present);
	}

	void createGraphics()
	{
		auto indices = QueueFamilyIndices(m_physicalDevice, *m_renderSurface);
		m_graphics = Graphics(*m_device, m_present, indices.graphics(), m_physicalDevice);
	}

	void initVulkan()
	{
		createInstance();
		
		checkValidationLayerSupport();
		setupDebugMessenger();
		
		createRenderSurface();
		pickPhysicalDevice();
		createLogicalDevice();

		createPresent();
		createGraphics();
		createSyncObjects();
	}

	uint32_t acquireNextImage(const vk::Semaphore& wait)
	{
		uint32_t imageIndex;
		auto status = m_present.acquireNextImage(wait, imageIndex);
		if (status == vk::Result::eErrorOutOfDateKHR)
		{
			recreatePresent();
			return acquireNextImage(wait);
		}
		
		else if (status != vk::Result::eSuccess and status != vk::Result::eSuboptimalKHR)
		{
			vk::throwResultException(status, "could not aquire next image");
		}

		return imageIndex;
	}

	void drawFrame(const vk::Semaphore& wait, const vk::Semaphore& signal, const vk::Fence& hostNotify)
	{
		m_device->waitForFences(1, &hostNotify, VK_TRUE, std::numeric_limits<uint64_t>::max());
		m_device->resetFences(1, &hostNotify);

		auto imageIndex = acquireNextImage(wait);
		
		m_graphics.render(wait, signal, hostNotify, imageIndex);
		
		auto status = m_present.present(signal, imageIndex);
		
		if (status == vk::Result::eErrorOutOfDateKHR or status == vk::Result::eSuboptimalKHR)
		{
			recreatePresent();
		}
		else if (status != vk::Result::eSuccess)
		{
			vk::throwResultException(status, "could not present queue!");
		}
	}

	void drawFrame()
	{
		drawFrame(*m_imageAvailable[m_currentFrame], *m_renderCompleted[m_currentFrame], *m_inFlightImages[m_currentFrame]);
		m_currentFrame = (m_currentFrame + 1) % config::MAX_FRAMES_IN_FLIGHT;
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			drawFrame();
			glfwPollEvents();
		}

		m_graphics.queue.waitIdle();
		m_present.await();
	}

// Order of fields is important for destructors
	vk::UniqueInstance 				m_instance;
	vk::UniqueSurfaceKHR 			m_renderSurface;
	vk::UniqueHandle<
		vk::DebugUtilsMessengerEXT, 
		vk::DispatchLoaderDynamic> 	m_debugMessenger;
	vk::UniqueDevice 				m_device;
	
	Present m_present;
	Graphics m_graphics;

    std::vector<vk::UniqueSemaphore> 	m_imageAvailable;
    std::vector<vk::UniqueFence> 		m_inFlightImages;
    std::vector<vk::UniqueSemaphore>	m_renderCompleted;
	
	vk::Queue					m_computeQueue;
	int 						m_currentFrame;
	vk::DispatchLoaderDynamic 	m_dispatchDynamic;
	vk::PhysicalDevice 			m_physicalDevice;
	GLFWwindow*					m_window;
	bool						m_windowSizeChanged;

};

int main()
{
	auto app = HelloTriangleApp();

	try
	{
		app.run();
	}
	catch (const VkError &ex)
	{
		std::cerr << "Error while running app: " << ex.what() << "(" << ex.status << ")\n";
		return EXIT_FAILURE;
	}
	catch (const std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
