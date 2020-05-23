#include "general.h"

#include <fstream>
#include <stdexcept>

std::vector<char> readFile(const std::string &path)
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

vk::UniqueDeviceMemory createMemory(
        const vk::Device& dev,
        const vk::MemoryRequirements& requirements,
        const vk::PhysicalDeviceMemoryProperties& physicalProperties,
        const vk::MemoryPropertyFlags& properties)
{
    return dev.allocateMemoryUnique(
        vk::MemoryAllocateInfo(
            requirements.size, 
            findMemoryType(
	    		physicalProperties,
	    		requirements.memoryTypeBits, 
	    		properties
	    	)
        )
    );
}

uint32_t findMemoryType(const vk::PhysicalDeviceMemoryProperties& properties, const uint32_t typeFilter, vk::MemoryPropertyFlags propertyFlags)
{
	for (auto i = 0u; i < properties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) && ((properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags))
		{
			return i;
		}
	}

	throw std::runtime_error("could not find compatible memory");
}

vk::Format findSupportedFormat(vk::PhysicalDevice dev, const std::vector<vk::Format>& candidates, vk::ImageTiling requiredTiling, vk::FormatFeatureFlags requiredFormatFeatures) {
	for (auto format : candidates) {
		auto supportedFeatures = dev.getFormatProperties(format);

		if (requiredTiling == vk::ImageTiling::eOptimal && ((supportedFeatures.optimalTilingFeatures & requiredFormatFeatures) == requiredFormatFeatures)) {
			return format;
		} else if (requiredTiling == vk::ImageTiling::eLinear && ((supportedFeatures.linearTilingFeatures & requiredFormatFeatures) == requiredFormatFeatures)) {
			return format;
		}
	}	

	throw std::runtime_error("could not find appropriate format");
}

bool hasStencilAttachment(vk::Format format) {
	switch (format) {
		case vk::Format::eD32SfloatS8Uint:
			return true;
		case vk::Format::eD24UnormS8Uint:
			return true;
		default:
			return false;
	}
}
