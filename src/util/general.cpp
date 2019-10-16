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

void copyBuffer(const vk::Device& device, const vk::Queue& queue, const vk::CommandPool& pool, const vk::Buffer& src, const vk::Buffer& dest, const vk::DeviceSize& size)
{
	auto copyCommand = vk::UniqueCommandBuffer(
		device.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				pool,
				vk::CommandBufferLevel::ePrimary,
				1
			)
		)[0],
		vk::PoolFree(
			device, 
			pool, 
			vk::DispatchLoaderStatic()
		)
	);

	copyCommand->begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		)
	);
	copyCommand->copyBuffer(src, dest, { vk::BufferCopy(0, 0, size) });
	copyCommand->end();

	auto copyFence = device.createFenceUnique(vk::FenceCreateInfo());
	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setPCommandBuffers(&copyCommand.get());
	queue.submit(1, &submitInfo, *copyFence);
	device.waitForFences({*copyFence}, VK_TRUE, std::numeric_limits<uint64_t>::max());
}
