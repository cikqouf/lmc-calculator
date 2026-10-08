#pragma once

#include <iostream>
#include <vector>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/* 可以用 vulkan_loader.json 代替这些功能 */
#ifndef CHECKMISCS
#define CHECKMISCS(host, misc, misc2, name)                                                               \
		do														      									  \
		{																								  \
			int idx = 0;																				  \
			auto props = host.enumerate##misc();														  \
			for (const auto& prop : props) {															  \
				auto fullName = std::string(prop.misc2.data());											  \
				if (fullName == std::string(name)) {													  \
					std::cerr << #misc << " check: " << fullName << " ok\n";						      \
					break;																				  \
				}																						  \
				else if (idx == props.size() - 1) {														  \
				    std::cerr << #misc << ": \"" << name << "\" didn't be found. below is the list:\n";	  \
				    for (const auto& prop : props) {												      \
						auto fullName = std::string(prop.misc2.data());						              \
				      	std::cerr << "CHECKMISCS: " << fullName << "\n";							      \
					}																					  \
				}																						  \
				++idx;																					  \
			}																							  \
		} while (0);
#endif

namespace EngineBase {

	/* only support vk::raii */
	template<typename T>
	class HandleManager {

	public:
		union CREATEINPUT {
			/* buffer size */
			vk::DeviceSize size;
			/* image */
			vk::Extent2D extent2d;
			/* image */
			vk::Extent3D extent3d;
			const char* shaderPath;
		};

	private:
		using CREATE = T(*)(const vk::raii::Device&, CREATEINPUT);
	public:
		std::vector<T> handles;
		size_t MAX_CAPACITY = 256;

	public:
		HandleManager(
			const CREATE& create,
			const vk::raii::Device& device,
			std::vector<CREATEINPUT> inputs,
			size_t max = 256)
			: MAX_CAPACITY(
				[&] {
					handles.reserve(max);
					return max;
				} ()) {
			for (int i = 0; i < inputs.size(); ++i) {
				handles.emplace_back(
					create(device, inputs[i])
				);
			}
		}
		virtual ~HandleManager() {}
	};

	struct MemParty {
		vk::raii::DeviceMemory handle = nullptr;
		vk::DeviceSize memSize = 0;

		MemParty() = delete;
		MemParty(const vk::raii::DeviceMemory&, vk::DeviceSize) = delete;
		MemParty(
			vk::raii::DeviceMemory&& inMem,
			vk::DeviceSize&& inSize)
			: handle(std::move(inMem))
			, memSize(inSize) {}
	};

	vk::raii::Pipeline gBasicPipelineBuilder(
		const vk::raii::Device& device,
		const std::array<const char*, 2>& shaderPathes,
		const std::vector<vk::VertexInputBindingDescription> vertexBindings,
		const std::vector<vk::VertexInputAttributeDescription> vertexAttrs,
		const std::vector<vk::Viewport> viewports,
		const std::vector<vk::Rect2D> scissors,
		const uint32_t depthTestCond,
		const vk::raii::PipelineLayout& layout,
		const vk::raii::RenderPass& renderPass
	) {
		EngineBase::HandleManager<vk::raii::ShaderModule> shaders(
			[]
			(const vk::raii::Device& device,
				EngineBase::HandleManager<vk::raii::ShaderModule>::CREATEINPUT inputs) {

					auto file = std::ifstream(inputs.shaderPath, std::ios_base::ate | std::ios_base::binary);
					size_t size = file.tellg();
					file.seekg(0);
					std::vector<char> code;
					code.reserve(size);
					file.read(code.data(), size);

					auto shader = device.createShaderModule(
						{
							.codeSize = size,
							.pCode = (uint32_t*)code.data(),
						}
						);

					return shader;
			},
			device,
			{ {.shaderPath = shaderPathes[0]}, {.shaderPath = shaderPathes[1]} }
		);

		const vk::PipelineShaderStageCreateInfo shaderStage[] = { {
			.stage = vk::ShaderStageFlagBits::eVertex,
			.module = shaders.handles[0],
			.pName = "main",
		},
		{
			.stage = vk::ShaderStageFlagBits::eFragment,
			.module = shaders.handles[1],
			.pName = "main",}
		};


		const vk::PipelineVertexInputStateCreateInfo vertexInputState = {
			.vertexBindingDescriptionCount = (uint32_t)vertexBindings.size(),
			.pVertexBindingDescriptions = vertexBindings.data(),
			.vertexAttributeDescriptionCount = (uint32_t)vertexAttrs.size(),
			.pVertexAttributeDescriptions = vertexAttrs.data(),
		};

		const vk::PipelineInputAssemblyStateCreateInfo assemblyState = {
			.topology = vk::PrimitiveTopology::eTriangleList,
		};

		const vk::PipelineViewportStateCreateInfo viewportState = {
			.viewportCount = (uint32_t)viewports.size(),
			.pViewports = viewports.data(),
			.scissorCount = (uint32_t)scissors.size(),
			.pScissors = scissors.data()
		};

		const vk::PipelineRasterizationStateCreateInfo rasterizationState = {
			.polygonMode = vk::PolygonMode::eFill,
			.cullMode = vk::CullModeFlagBits::eNone,
			.frontFace = vk::FrontFace::eClockwise,
			.lineWidth = 1.f,
		};

		const vk::PipelineMultisampleStateCreateInfo multiSampleState = {
			.rasterizationSamples = vk::SampleCountFlagBits::e1,
		};

		const vk::PipelineDepthStencilStateCreateInfo depthStencilState = {
			.depthTestEnable = depthTestCond,
			.depthWriteEnable = vk::False,
			.depthCompareOp = vk::CompareOp::eGreater,
			.minDepthBounds = 0.f,
			.maxDepthBounds = 1.f
		};

		const vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
			.blendEnable = vk::True,
			.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
			.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
			.colorBlendOp = vk::BlendOp::eAdd,
			.colorWriteMask =
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		};

		const vk::PipelineColorBlendStateCreateInfo colorBlendState = {
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
		};

		auto gPipeline = device.createGraphicsPipeline(nullptr,
			{
				.stageCount = 2,
				.pStages = shaderStage,
				.pVertexInputState = &vertexInputState,
				.pInputAssemblyState = &assemblyState,
				.pViewportState = &viewportState,
				.pRasterizationState = &rasterizationState,
				.pMultisampleState = &multiSampleState,
				.pDepthStencilState = &depthStencilState,
				.pColorBlendState = &colorBlendState,
				.layout = layout,
				.renderPass = renderPass,
				.subpass = 0,
			}
			);

		return gPipeline;
	}

	static uint32_t memTypeChoose(
		const vk::PhysicalDevice& phyDevice,
		vk::MemoryPropertyFlags cond
	) {
		auto memProps = phyDevice.getMemoryProperties();
		for (uint32_t idx = 0; idx < memProps.memoryTypeCount; ++idx) {
			if ((memProps.memoryTypes[idx].propertyFlags & cond) == cond) {
				return idx;
			}
		}

		return 0u;
	}

	/* only support vk:: */
	template<typename T>
	static uint32_t	resourceMemTypeGet(
		const T& handle,
		const vk::raii::PhysicalDevice& phyDevice,
		vk::MemoryPropertyFlags cond) {
		auto memTypeBits = handle.getMemoryRequirements().memoryTypeBits;
		for (uint32_t idx = 0u; idx < (uint32_t)sizeof(uint32_t) * 8u; ++idx) {
			if ((1 << idx) & memTypeBits) {
				if (idx == memTypeChoose(phyDevice, cond) ) {
					return idx;
				}
			}
		}

		std::cerr << "fallback to 0u...\n";
		return 0u;
	}

	template<typename T>
	static void resourceBindMemory(
		const vk::raii::DeviceMemory& mem,
		const std::vector<T>& handles,
		vk::DeviceSize offset = 0) {

		for (int i = 0; i < handles.size(); ++i) {
			auto req = handles[i].getMemoryRequirements();

			offset = offset % req.alignment == 0 ?
				offset : (offset + req.alignment - offset % req.alignment);

			handles[i].bindMemory(mem, offset);

			offset += req.size;
		}

	}


	// cpuMem must be mapped from VkDeviceMemory
	template<typename T>
	static void resourceCopyMemory2Cpu(
		void*& cpuMem,
		const std::vector<std::vector<T>>& datas,
		vk::DeviceSize offset = 0) {

		for (int i = 0; i < datas.size(); ++i) {
			auto addSize = sizeof(T) * datas[i].size();
			memcpy((char*)cpuMem + offset, datas[i].data(), addSize);
			offset += addSize;
		}
	}

	/* vulkan loader 可以绘制和呈现的(基于glfw)最小 vk::raii 初始化 */
	class InitModule {

	private:
		static VKAPI_ATTR vk::Bool32 VKAPI_CALL callBackFunc(
			vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
			vk::DebugUtilsMessageTypeFlagsEXT             messageTypes,
			const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) {
			std::cerr <<
				pCallbackData->pMessageIdName <<
				" [" << pCallbackData->messageIdNumber << "]: " <<
				pCallbackData->pMessage << "\n";
			return vk::False;
		}
	public:
		vk::raii::Context context = vk::raii::Context();
		vk::raii::Instance instance;

		vk::raii::DebugUtilsMessengerEXT messenger;

		vk::raii::PhysicalDevice physicalDevice;

		uint32_t queueFamilyIndex = 0u;
		vk::raii::Device device;

	public:
		InitModule() = delete;
		InitModule(vk::PhysicalDeviceType phyDeviceType)
			: instance(
				[&] {
					glfwInit();
					glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

					CHECKMISCS(context, InstanceLayerProperties, layerName, "VK_LAYER_KHRONOS_validation");
					CHECKMISCS(context, InstanceExtensionProperties, extensionName, "VK_KHR_surface");
					CHECKMISCS(context, InstanceExtensionProperties, extensionName, "VK_KHR_win32_surface");
					CHECKMISCS(context, InstanceExtensionProperties, extensionName, "VK_EXT_debug_utils");

					const char* layerNames[] = { "VK_LAYER_KHRONOS_validation" };
					const char* extensionNames[] = { "VK_KHR_surface" , "VK_KHR_win32_surface" , "VK_EXT_debug_utils" };

					const vk::ApplicationInfo appInfo = {
						.pApplicationName = "uiTest",
						.applicationVersion = vk::makeVersion(0, 0, 1),
						.pEngineName = "renderEngine",
						.engineVersion = vk::makeVersion(0, 0, 1),
						.apiVersion = vk::makeApiVersion(0, 1, 3, 0),
					};

					return context.createInstance({
						   .pApplicationInfo = &appInfo,
						   .enabledLayerCount = 1,
						   .ppEnabledLayerNames = layerNames,
						   .enabledExtensionCount = 3,
						   .ppEnabledExtensionNames = extensionNames,
						});
				} ())
			, messenger(
				[&] {
					return instance.createDebugUtilsMessengerEXT({
							.messageSeverity =
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
								vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
							   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
							.messageType =
								vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
								vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
								vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
							.pfnUserCallback = &callBackFunc,
							.pUserData = nullptr,
						});
				} ())
			, physicalDevice(
				[&] {
					auto physicalDevices = instance.enumeratePhysicalDevices();
					for (const auto& item : physicalDevices) {
						auto props = item.getProperties();
						if (props.deviceType == phyDeviceType) {
							std::cerr << "deviceName: " << props.deviceName << "\n";
							return item;
						}
					}
					return physicalDevices[0];
				} ())
			, device(
				[&] {
					{
						auto queueProps = physicalDevice.getQueueFamilyProperties();
						uint32_t idx = 0;
						for (const auto& item : queueProps) {
							if (item.queueFlags & vk::QueueFlagBits::eTransfer &&
								item.queueFlags & vk::QueueFlagBits::eGraphics
								) {
								break;
							}
							++idx;
						}
						queueFamilyIndex = idx;
						std::cerr << "queueFamilyIndex: " << queueFamilyIndex << "\n";
					}
					const float queuePriorties[] = { 1.f };
					const vk::DeviceQueueCreateInfo info[] = { {
						.queueFamilyIndex = queueFamilyIndex,
						.queueCount = 1,
						.pQueuePriorities = queuePriorties,
					} };
					std::cerr << "queueCount: " << 1 << "\n";

					CHECKMISCS(physicalDevice, DeviceExtensionProperties, extensionName, "VK_KHR_swapchain");
					CHECKMISCS(physicalDevice, DeviceExtensionProperties, extensionName, "VK_KHR_shader_draw_parameters");
					const char* extNames[] = { "VK_KHR_swapchain", "VK_KHR_shader_draw_parameters" };

					return physicalDevice.createDevice({
							.queueCreateInfoCount = 1,
							.pQueueCreateInfos = info,
							.enabledLayerCount = 0,
							.ppEnabledLayerNames = nullptr,
							.enabledExtensionCount = 2,
							.ppEnabledExtensionNames = extNames,
							.pEnabledFeatures = nullptr,
						});
				}()) {}
	};
};
