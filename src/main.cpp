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
		: std::exception(), m_layerName(layerName), m_errorMessage(std::string("Validation layer not found: ") + layerName)
	{
	}

	const char* what() const noexcept override
	{
		return m_errorMessage.c_str();
	}

	const char* layer() const 
	{
		return m_layerName;
	}

private:
	const char* m_layerName;
	const std::string m_errorMessage;
};

class VkExtensionNotFoundError : public std::runtime_error
{
public:
	VkExtensionNotFoundError(const char* extensionName)
		: std::runtime_error(std::string("Validation layer not found: ") + extensionName), m_extensionName(extensionName)
	{
	}

	const char* extensions() const
	{
		return m_extensionName;
	}

private:
	const char* m_extensionName;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isReady() const 
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails 
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

	bool isAdequate() const
	{
		return !formats.empty() && !presentModes.empty();
	}
};

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;
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

const std::vector<const char*> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<char> readFile(const std::string& path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error(std::string("could not open shader file: ") + path);
	}

	auto fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

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

bool checkDeviceExtensionsSupported(VkPhysicalDevice dev)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> properties(extensionCount);
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, properties.data());

	bool isFound = false;
	for (const auto& extName : DEVICE_EXTENSIONS)
	{
		for (const auto& extProperties : properties)
		{
			if (!strcmp(extName, extProperties.extensionName))
			{
				isFound = true;
				break;
			}
		}

		if (!isFound)
		{
			return false;
		}
	}

	return true;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR renderSurface) {
    SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, renderSurface, &details.capabilities);

	uint32_t count;
	
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderSurface, &count, nullptr);
	if (count != 0)
	{
		details.formats.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, renderSurface, &count, details.formats.data());
	}
	
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderSurface, &count, nullptr);
	if (count != 0)
	{
		details.presentModes.resize(count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, renderSurface, &count, details.presentModes.data());
	}

    return details;
}

bool isDeviceSuitable(VkPhysicalDevice dev, VkSurfaceKHR renderSurface)
{
	auto queuesFound = findQueueFamilies(dev, renderSurface).isReady();
	auto extensionsSupported = checkDeviceExtensionsSupported(dev);
	auto swapChainAdequate = querySwapChainSupport(dev, renderSurface).isAdequate();
	return queuesFound && extensionsSupported && swapChainAdequate;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
	for (const auto& fmt : availableFormats)
	{
		if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return fmt;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

template <typename T>
T clamp(const T& min, const T& value, const T& max)
{
	return std::max(min, std::min(value, max));
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	return {
		clamp(capabilities.minImageExtent.width, WIDTH, capabilities.maxImageExtent.width),
		clamp(capabilities.minImageExtent.height, HEIGHT, capabilities.maxImageExtent.height) 
	};
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
		createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());

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

	void createSwapChain()
	{
		auto support = querySwapChainSupport(m_physicalDevice, m_renderSurface);

		auto format = chooseSwapSurfaceFormat(support.formats);
		auto presentationMode = chooseSwapPresentMode(support.presentModes);
		auto extent = chooseSwapExtent(support.capabilities);

		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0)
		{
			imageCount = std::min(imageCount, support.capabilities.maxImageCount);
		}

		VkSwapchainCreateInfoKHR chainInfo = { };
		chainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		chainInfo.surface = m_renderSurface;
		chainInfo.minImageCount = imageCount;
		chainInfo.imageFormat = format.format;
		chainInfo.imageColorSpace = format.colorSpace;
		chainInfo.imageExtent = extent;
		chainInfo.imageArrayLayers = 1;
		chainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_renderSurface);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily != indices.presentFamily) 
		{
		    chainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		    chainInfo.queueFamilyIndexCount = 2;
		    chainInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else 
		{
		    chainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		    chainInfo.queueFamilyIndexCount = 0; // Optional
		    chainInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		chainInfo.preTransform = support.capabilities.currentTransform;
		chainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		chainInfo.presentMode = presentationMode;
		chainInfo.clipped = VK_TRUE;
		chainInfo.oldSwapchain = VK_NULL_HANDLE;

		auto status = vkCreateSwapchainKHR(m_device, &chainInfo, nullptr, &m_swapChain);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create swap chain", status);
		}

		m_swapChainExtent = extent;
		m_swapChainImageFormat = format.format;

		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
	}

	void createImageViews()
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		
		VkResult status;
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImageViews.size(); i++) 
		{
			createInfo.image = m_swapChainImages[i];
			status = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
			if (status != VK_SUCCESS)
			{
				throw VkError("could not create image view", status);
			}
		}
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) 
	{
		VkShaderModuleCreateInfo shaderInfo = { };
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.codeSize = code.size();
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule ret;
		auto status = vkCreateShaderModule(m_device, &shaderInfo, nullptr, &ret);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create shader module", status);
		}

		return ret;
	}

	void createRenderPass()
	{
		VkAttachmentDescription color = { };
		color.format = m_swapChainImageFormat;
		color.samples = VK_SAMPLE_COUNT_1_BIT;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorRef = { };
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDesc = { };
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &colorRef;

		VkRenderPassCreateInfo renderPassInfo = { };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &color;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDesc;

		auto status = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
		if (status != VK_SUCCESS)
		{
			throw VkError("could not create render pass", status);
		}
	}

	void createGraphicsPipeline()
	{
		auto vertShaderCode = readFile("vert.spv");
		auto fragShaderCode = readFile("frag.spv");

		auto fragShader = createShaderModule(fragShaderCode);
		auto vertShader = createShaderModule(vertShaderCode);

		VkPipelineShaderStageCreateInfo vertInfo = { };
		vertInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertInfo.module = vertShader;
		vertInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragInfo = {};
		fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragInfo.module = fragShader;
		fragInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertInfo, fragInfo};

		VkPipelineVertexInputStateCreateInfo vertexInput = { };
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 0;
		vertexInput.pVertexBindingDescriptions = nullptr;
		vertexInput.vertexAttributeDescriptionCount = 0;
		vertexInput.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { };
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = { };
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = { };
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = { };
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizerState = { };
		rasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizerState.depthClampEnable = VK_FALSE;
		rasterizerState.rasterizerDiscardEnable = VK_FALSE;
		rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizerState.lineWidth = 1.0f;
		rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizerState.depthBiasClamp = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisamplingState = { };
		multisamplingState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplingState.sampleShadingEnable = VK_FALSE;
		multisamplingState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkDynamicState dynamicStates[] = {
		    VK_DYNAMIC_STATE_VIEWPORT,
		    VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		auto status = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
		if (status != VK_SUCCESS) 
		{
    		throw VkError("failed to create pipeline layout!", status);
		}

		VkGraphicsPipelineCreateInfo graphicsInfo = { };
		graphicsInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsInfo.stageCount = 2;
		graphicsInfo.pStages = shaderStages;
		graphicsInfo.pVertexInputState = &vertexInput;
		graphicsInfo.pInputAssemblyState = &inputAssembly;
		graphicsInfo.pViewportState = &viewportState;
		graphicsInfo.pRasterizationState = &rasterizerState;
		graphicsInfo.pMultisampleState = &multisamplingState;
		graphicsInfo.pColorBlendState = &colorBlending;
		graphicsInfo.layout = m_pipelineLayout;
		graphicsInfo.renderPass = m_renderPass;
		graphicsInfo.subpass = 0;

		status = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &graphicsInfo, nullptr, &m_pipeline);
		if (status != VK_SUCCESS)
		{
			throw VkError("failed to create graphics pipeline", status);
		}

		vkDestroyShaderModule(m_device, vertShader, nullptr);
		vkDestroyShaderModule(m_device, fragShader, nullptr);
	}

	void createFramebuffers()
	{
		m_frameBuffers.resize(m_swapChainImages.size());

		VkResult status;
		VkFramebufferCreateInfo framebufferInfo = { };
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		for (auto i = 0u; i < m_swapChainImageViews.size(); ++i)
		{
			framebufferInfo.pAttachments = &m_swapChainImageViews[i];
			status = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_frameBuffers[i]);
			if (status != VK_SUCCESS)
			{
				throw VkError("could not create framebuffer", status);
			}
		}
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
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
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
		for (auto framebuffer : m_frameBuffers) 
		{
        	vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    	}

		vkDestroyPipeline(m_device, m_pipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		for (auto view : m_swapChainImageViews)
		{
			vkDestroyImageView(m_device, view, nullptr);
		}

		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
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
	std::vector<VkFramebuffer> m_frameBuffers;
	VkQueue m_graphicsQueue;
	VkInstance m_instance;
	VkPipeline m_pipeline;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout;
	VkQueue m_presentQueue;
	VkRenderPass m_renderPass;
	VkSurfaceKHR m_renderSurface;
	VkSwapchainKHR m_swapChain;
	VkExtent2D m_swapChainExtent;
	VkFormat m_swapChainImageFormat;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
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
