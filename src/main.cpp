#include <iostream>
#include <string>
#include <fstream>

#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>

#include <engineBase.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace CalApp {

	struct InTexImage {
		std::vector<char> imageBufferData;
	};

	struct InstanceDataInput {
		float posOffset[2];
		float uvOffset[2];
	};

	struct VertexInput {
		float position[2];
		float uv[2];
	};

	struct RenderBaseComponent {
		vk::raii::SwapchainKHR swapchain = nullptr;
		std::vector<vk::Image> swapchainImages;
		std::vector<vk::raii::ImageView> swapchainImageViews;
		std::vector<vk::raii::Framebuffer> framebuffers;
		vk::raii::RenderPass renderPass = nullptr;

		RenderBaseComponent(
			const vk::raii::SwapchainKHR& inSwapchain,
			std::vector<vk::Image>&& inSwapchainImages,
			std::vector<vk::raii::ImageView>&& inSwapchainImageViews,
			std::vector<vk::raii::Framebuffer>&& inFramebuffers,
			const vk::raii::RenderPass& inRenderPass) = delete;

		RenderBaseComponent(
			vk::raii::SwapchainKHR&& inSwapchain,
			std::vector<vk::Image>&& inSwapchainImages,
			std::vector<vk::raii::ImageView>&& inSwapchainImageViews,
			std::vector<vk::raii::Framebuffer>&& inFramebuffers,
			vk::raii::RenderPass&& inRenderPass)
			: swapchain(std::move(inSwapchain))
			, swapchainImages(std::move(inSwapchainImages))
			, swapchainImageViews(std::move(inSwapchainImageViews))
			, framebuffers(std::move(inFramebuffers))
			, renderPass(std::move(inRenderPass)) {}


		void kill() {
			framebuffers.clear();
			swapchainImageViews.clear();
			swapchainImages.clear();
			swapchain = nullptr;
		}
	};

	// haha this is a vibe coded function
	static void buildTextMesh(
		const char* text,
		const stbtt_bakedchar* baked,
		float atlasW, float atlasH,
		float screenW, float screenH,
		std::vector<VertexInput>& vertices,
		std::vector<uint32_t>& indices)
	{
		float cursorX = 0.0f;
		float cursorY = 8.f;

		for (const char* p = text; *p; p++) {
			char c = *p;
			if (c < 32 || c >= 128) continue;

			const stbtt_bakedchar& bc = baked[c - 32];

			float x0 = cursorX + bc.xoff;
			float y0 = cursorY + bc.yoff;
			float x1 = x0 + (bc.x1 - bc.x0);
			float y1 = y0 + (bc.y1 - bc.y0);

			float u0 = bc.x0 / atlasW;
			float v0 = bc.y0 / atlasH;
			float u1 = bc.x1 / atlasW;
			float v1 = bc.y1 / atlasH;

			auto toNDC = [&](float px, float py) {
				float ndcX = (px / screenW) * 2.0f - 1.0f;
				float ndcY = (py / screenH) * 2.0f - 1.0f;  // 注意：不翻转
				return std::array<float, 2>{ ndcX, ndcY };
				};

			auto p0 = toNDC(x0, y0);
			auto p1 = toNDC(x1, y0);
			auto p2 = toNDC(x1, y1);
			auto p3 = toNDC(x0, y1);


			uint32_t base = vertices.size();

			vertices.push_back({ {p0[0], p0[1]}, {u0, v0} });
			vertices.push_back({ {p1[0], p1[1]}, {u1, v0} });
			vertices.push_back({ {p2[0], p2[1]}, {u1, v1} });
			vertices.push_back({ {p3[0], p3[1]}, {u0, v1} });

			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 2);
			indices.push_back(base + 3);
			indices.push_back(base + 0);

			cursorX += bc.xadvance;
		}
	}


	static void transferBufferToImage(
		const vk::raii::CommandBuffer& cmd,
		uint32_t queueFamilyIndex,
		const vk::raii::Device& device,
		const vk::raii::Buffer& buffer,
		const vk::raii::Image& image,
		uint32_t texWidth,
		uint32_t texHeight,
		vk::DeviceSize bufferSize,
		const vk::raii::Queue& queue
	) {
		cmd.begin(
			{
				.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			}
			);

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{},
		{
			vk::BufferMemoryBarrier
			{
				 .srcAccessMask = vk::AccessFlagBits::eNone,
				 .dstAccessMask = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
				 .srcQueueFamilyIndex = queueFamilyIndex,
				 .dstQueueFamilyIndex = queueFamilyIndex,
				 .buffer = buffer,
				 .offset = 0,
				 .size = bufferSize,
			}
		},
		{
			vk::ImageMemoryBarrier
			{
				 .srcAccessMask = vk::AccessFlagBits::eNone,
				 .dstAccessMask = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
				 .oldLayout = vk::ImageLayout::eUndefined,
				 .newLayout = vk::ImageLayout::eTransferDstOptimal,
				 .srcQueueFamilyIndex = queueFamilyIndex,
				 .dstQueueFamilyIndex = queueFamilyIndex,
				 .image = image,
				 .subresourceRange =
					{
						.aspectMask = vk::ImageAspectFlagBits::eColor,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					},
			}
		}
		);

		cmd.copyBufferToImage(
			buffer, image, vk::ImageLayout::eTransferDstOptimal,
			{
				vk::BufferImageCopy
				{
					.bufferOffset = 0,
					.bufferRowLength = (uint32_t)texWidth,
					.bufferImageHeight = (uint32_t)texHeight ,
					.imageSubresource =
								{
									.aspectMask = vk::ImageAspectFlagBits::eColor,
									.mipLevel = 0,
									.baseArrayLayer = 0,
									.layerCount = 1,
								},
					.imageOffset = {0, 0, 0},
					.imageExtent = {(uint32_t)texWidth, (uint32_t)texHeight, 1}
				}
			}
		);

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader,
			{},
			{},
			{},
		{
			vk::ImageMemoryBarrier
			{
				 .srcAccessMask = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite,
				 .dstAccessMask = vk::AccessFlagBits::eShaderRead,
				 .oldLayout = vk::ImageLayout::eTransferDstOptimal,
				 .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
				 .srcQueueFamilyIndex = queueFamilyIndex,
				 .dstQueueFamilyIndex = queueFamilyIndex,
				 .image = image,
				 .subresourceRange =
					{
						.aspectMask = vk::ImageAspectFlagBits::eColor,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					},
			}
		}
		);

		cmd.end();

		auto transferFence = device.createFence({});

		queue.submit(
			{
				{
					.waitSemaphoreCount = 0,
					.pWaitSemaphores = nullptr,
					.pWaitDstStageMask = nullptr,
					.commandBufferCount = 1,
					.pCommandBuffers = &*cmd,
					.signalSemaphoreCount = 0,
					.pSignalSemaphores = nullptr,
				}
			},
			*transferFence
		);
		if (device.waitForFences({ *transferFence }, vk::True, UINT64_MAX) == vk::Result::eSuccess) {
			device.resetFences({ *transferFence });
		}
		cmd.reset();
	}

	class app {
	public:
		EngineBase::InitModule core;
		GLFWwindow* window = nullptr;
		vk::raii::SurfaceKHR surface = nullptr;

	private:
		vk::raii::CommandPool cmdPool = nullptr;

	public:

		void recreateSwapchain(
			RenderBaseComponent& renderBaseResources,
			vk::Extent2D windowExtent) const {
			core.device.waitIdle();

			renderBaseResources.kill();

			int w = 0, h = 0;
			while (w == 0 && h == 0) {
				glfwGetFramebufferSize(window, &w, &h);
				glfwWaitEvents();
			}

			renderBaseResources = createRenderBase(windowExtent);

			core.device.waitIdle();
		}

		auto createRenderPass(
			const std::vector<vk::AttachmentReference>& refs,
			const std::vector<vk::AttachmentDescription>& descs) const {

			const vk::SubpassDependency dependency = {
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = vk::PipelineStageFlagBits::eTransfer,
				.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eTransferRead,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
				.dependencyFlags = vk::DependencyFlagBits::eByRegion,
			};

			vk::SubpassDescription subpass = {
				.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
				.colorAttachmentCount = (uint32_t)refs.size(),
				.pColorAttachments = refs.data(),
			};

			vk::raii::RenderPass renderPass = core.device.createRenderPass(
				{
					.attachmentCount = (uint32_t)descs.size(),
					.pAttachments = descs.data(),
					.subpassCount = 1,
					.pSubpasses = &subpass,
					.dependencyCount = 1,
					.pDependencies = &dependency,
				});

			return renderPass;
		}

		auto createCmds(uint32_t size) const {
			return core.device.allocateCommandBuffers({
					.commandPool = cmdPool,
					.level = vk::CommandBufferLevel::ePrimary,
					.commandBufferCount = size,
				});
		}


		RenderBaseComponent createRenderBase(vk::Extent2D extent) const {

			/* 交换链 */
			auto swapchain = core.device.createSwapchainKHR(
				{
					.surface = surface,
					.minImageCount = 2,
					.imageFormat = vk::Format::eR8G8B8A8Srgb,
					.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
					.imageExtent = extent,
					.imageArrayLayers = 1,
					.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
					.imageSharingMode = vk::SharingMode::eExclusive,
					.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
					.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
					.presentMode = vk::PresentModeKHR::eImmediate,
					.clipped = vk::False,
					.oldSwapchain = nullptr,
				});
			/* 交换链图像 */
			auto swapchainImages = swapchain.getImages();
			/* 交换链图像上下文 */
			std::vector<vk::raii::ImageView> swapchainImageViews;
			swapchainImageViews.reserve(swapchainImages.size());
			for (int i = 0; i < swapchainImages.size(); ++i) {
				swapchainImageViews.emplace_back(
					core.device.createImageView(
						{
							.image = swapchainImages[i],
							.viewType = vk::ImageViewType::e2D,
							.format = vk::Format::eR8G8B8A8Srgb,
							.components =
							{
								.r = vk::ComponentSwizzle::eIdentity,
								.g = vk::ComponentSwizzle::eIdentity,
								.b = vk::ComponentSwizzle::eIdentity,
								.a = vk::ComponentSwizzle::eIdentity
							},
							.subresourceRange =
							{
								.aspectMask = vk::ImageAspectFlagBits::eColor,
								.baseMipLevel = 0,
								.levelCount = 1,
								.baseArrayLayer = 0,
								.layerCount = 1,
							}
						})
				);
			}

			/* renderPass */
			const vk::AttachmentDescription colorAttachment = {
				.format = vk::Format::eR8G8B8A8Srgb,
				.samples = vk::SampleCountFlagBits::e1,
				.loadOp = vk::AttachmentLoadOp::eDontCare,
				.storeOp = vk::AttachmentStoreOp::eStore,
				.initialLayout = vk::ImageLayout::eUndefined,
				.finalLayout = vk::ImageLayout::ePresentSrcKHR,
			};
			const vk::AttachmentReference colorRef = {
				.attachment = 0,
				.layout = vk::ImageLayout::eColorAttachmentOptimal,
			};
			auto renderPass = this->createRenderPass({ colorRef }, { colorAttachment });

			/* framebuffers */
			std::vector<vk::raii::Framebuffer> framebuffers;
			framebuffers.reserve(swapchainImageViews.size());
			for (int i = 0; i < swapchainImageViews.size(); ++i) {
				framebuffers.emplace_back(
					core.device.createFramebuffer(
						{
							.renderPass = renderPass,
							.attachmentCount = 1,
							.pAttachments = &*swapchainImageViews[i],
							.width = extent.width,
							.height = extent.height,
							.layers = 1,
						}
						)
				);
			}

			RenderBaseComponent result(
				std::move(swapchain),
				std::move(swapchainImages),
				std::move(swapchainImageViews),
				std::move(framebuffers),
				std::move(renderPass));

			return result;
		}

	public:
		app() = delete;
		app(int width = 800, int height = 600)
			: core(
				vk::PhysicalDeviceType::eIntegratedGpu
			)
			, window(
				[&] {
					glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
					return glfwCreateWindow(width, height, "calculatorDemo", nullptr, nullptr);
				}())
			, surface(
				[&] {
					VkSurfaceKHR surfaceHandle;
					auto res = glfwCreateWindowSurface(*core.instance, window, nullptr, &surfaceHandle);
					if (res == VK_SUCCESS) {
						return vk::raii::SurfaceKHR(core.instance, surfaceHandle);
					}
					else {
						throw std::runtime_error("create surface: failed");
					}
				}()) {
			cmdPool = core.device.createCommandPool({
							.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
							.queueFamilyIndex = core.queueFamilyIndex,
				});
		}

		template<typename RENDERFUNC>
		void loop(RENDERFUNC&& renderfunc) const {
			while (!glfwWindowShouldClose(window)) {

				renderfunc();
				glfwPollEvents();
			}
			core.device.waitIdle();
		}

		~app() {
			glfwDestroyWindow(window);
			glfwTerminate();
		}
	};

	static void cmdRecord(
		const vk::raii::CommandBuffer& cmd,
		uint32_t imageIdx,
		vk::Extent2D windowExtent,
		const RenderBaseComponent& renderBaseResources,
		const vk::raii::Pipeline& pipeline,
		const std::vector<vk::raii::Buffer>& vertexInputBuffers,
		const std::vector<vk::raii::Buffer>& indexBuffers,
		uint32_t indexCount,
		uint32_t instanceCount,
		const std::vector<vk::raii::DescriptorSet>& descSets,
		const vk::raii::PipelineLayout& pipelineLayout) {
		cmd.begin({});
		cmd.beginRenderPass(
			{
				.renderPass = renderBaseResources.renderPass,
				.framebuffer = renderBaseResources.framebuffers[imageIdx],
				.renderArea = {{0, 0}, windowExtent},
			},
			vk::SubpassContents::eInline);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		cmd.bindVertexBuffers(0, { vertexInputBuffers[0],  vertexInputBuffers[1] }, { 0, 0 });
		cmd.bindIndexBuffer(indexBuffers[0], 0, vk::IndexType::eUint32);
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0u,
			{ *descSets[0] },
			{});

		cmd.drawIndexed(indexCount, instanceCount, 0, 0, 0);

		cmd.endRenderPass();
		cmd.end();
	}
}

int wmain() {
	SetEnvironmentVariable(L"DISABLE_VULKAN_OBS_CAPTURE", L"1");
	SetEnvironmentVariable(L"DISABLE_RTSS_LAYER", L"1");

	vk::Extent2D windowExtent = { 400, 420 };

	CalApp::app appBase(windowExtent.width, windowExtent.height);
	auto& core = appBase.core;

	/* 队列 */
	auto queue = core.device.getQueue(core.queueFamilyIndex, 0u);
	/* 渲染基本组件 */
	auto renderBaseResources = appBase.createRenderBase(windowExtent);
	/* command buffers */
	auto cmds = appBase.createCmds(1u);
	auto& cmd = cmds[0];

	/* sampler and Image */
	vk::raii::Sampler sampler = core.device.createSampler(
		{
			.magFilter = vk::Filter::eLinear,
			.minFilter = vk::Filter::eLinear,
			.mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eRepeat,
			.addressModeV = vk::SamplerAddressMode::eRepeat,
			.addressModeW = vk::SamplerAddressMode::eRepeat,
			.mipLodBias = 0.f,
			.anisotropyEnable = vk::False,
			.compareEnable = vk::False,
			.minLod = 0.f,
			.maxLod = 1.f,
			.borderColor = vk::BorderColor::eIntOpaqueWhite,
			.unnormalizedCoordinates = vk::False,
		}
		);

	std::vector<unsigned char> fontData;
	vk::DeviceSize fontDataSize = 0;
	{
		std::ifstream file("ttf/SourceCodePro-Regular.ttf", std::ios_base::binary | std::ios_base::ate);
		fontDataSize = file.tellg();
		fontData.reserve(fontDataSize);
		fontData.resize(fontDataSize);
		file.seekg(0);
		file.read((char*)fontData.data(), fontDataSize * sizeof(unsigned char));
	}
	constexpr vk::DeviceSize fontTexWidth = 256;
	constexpr vk::DeviceSize fontTexHeight = 256;
	constexpr vk::DeviceSize fontTexSize = fontTexWidth * fontTexHeight;
	std::vector<unsigned char> fontAtlas(fontTexSize);
	std::vector<stbtt_bakedchar> bakedFontData(96);
	stbtt_BakeFontBitmap(
		fontData.data(), 0, 16.f,
		fontAtlas.data(), fontTexWidth, fontTexHeight, 32, 96, bakedFontData.data());

	EngineBase::HandleManager<vk::raii::Buffer> texImageBuffers(
		[]
		(const vk::raii::Device& device, EngineBase::HandleManager<vk::raii::Buffer>::CREATEINPUT inputs)
		{
			return device.createBuffer(
				{
					.size = inputs.size,
					.usage = vk::BufferUsageFlagBits::eTransferSrc,
					.sharingMode = vk::SharingMode::eExclusive,
				}
				);
		},
		core.device,
		{
			{.size = fontTexSize},
		}
		);

	EngineBase::MemParty texBufferMem =
	{
		core.device.allocateMemory(
			{
				.allocationSize = 42'161'540,
				.memoryTypeIndex = EngineBase::resourceMemTypeGet(
					texImageBuffers.handles[0],
					core.physicalDevice,
					vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
			}
		),
		42'161'540
	};

	EngineBase::resourceBindMemory(texBufferMem.handle, texImageBuffers.handles);
	auto texImageBufferMemCpu = texBufferMem.handle.mapMemory(0, texBufferMem.memSize);
	EngineBase::resourceCopyMemory2Cpu(texImageBufferMemCpu, std::vector<std::vector<unsigned char>>{ fontAtlas });
	texBufferMem.handle.unmapMemory();

	EngineBase::HandleManager<vk::raii::Image> texImages(
		[]
		(const vk::raii::Device& device, EngineBase::HandleManager<vk::raii::Image>::CREATEINPUT inputs)
		{
			return device.createImage(
				{
					.imageType = vk::ImageType::e2D,
					.format = vk::Format::eR8Unorm,
					.extent = inputs.extent3d,
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = vk::SampleCountFlagBits::e1,
					.tiling = vk::ImageTiling::eOptimal,
					.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
					.sharingMode = vk::SharingMode::eExclusive,
				}
				);
		},
		core.device,
		{
			{
				.extent3d = {(uint32_t)fontTexWidth, (uint32_t)fontTexHeight, 1},
			},
		}
		);

	EngineBase::MemParty texImageMem = {
		core.device.allocateMemory(
			{
				.allocationSize = 45'700'160,
				.memoryTypeIndex = EngineBase::resourceMemTypeGet(
					texImages.handles[0],
					core.physicalDevice,
					vk::MemoryPropertyFlagBits::eDeviceLocal)

			}
		)
		,
		45'700'160
	};

	EngineBase::resourceBindMemory(texImageMem.handle, texImages.handles);

	auto samplerImageView = core.device.createImageView(
		{
			.image = texImages.handles[0],
			.viewType = vk::ImageViewType::e2D,
			.format = vk::Format::eR8Unorm,
			.components =
				{
					.r = vk::ComponentSwizzle::eIdentity,
					.g = vk::ComponentSwizzle::eIdentity,
					.b = vk::ComponentSwizzle::eIdentity,
					.a = vk::ComponentSwizzle::eIdentity,
				},
			.subresourceRange =
				{
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				}
		}
	);

	/* 传输数据到 texImages */
	CalApp::transferBufferToImage(
		cmd,
		core.queueFamilyIndex,
		core.device,
		texImageBuffers.handles[0],
		texImages.handles[0],
		fontTexWidth,
		fontTexHeight,
		fontTexSize,
		queue
	);

	/* vertexInputs */
	std::vector<CalApp::VertexInput> quad;
	std::vector<uint32_t> indexDatas;

	CalApp::buildTextMesh(
		"hello,world",
		bakedFontData.data(),
		fontTexWidth,
		fontTexHeight,
		windowExtent.width,
		windowExtent.height,
		quad,
		indexDatas);
	std::vector<CalApp::InstanceDataInput> instanceDataInputs;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 5; ++j) {
			instanceDataInputs.push_back({ {.5f * i, .4f * j}, {0.f, 0.f} });
		}
	}

	EngineBase::HandleManager<vk::raii::Buffer> vertexInputBuffers(
		[]
		(const vk::raii::Device& device, EngineBase::HandleManager<vk::raii::Buffer>::CREATEINPUT inputs)
		{
			return device.createBuffer({
					.size = inputs.size,
					.usage = vk::BufferUsageFlagBits::eVertexBuffer,
					.sharingMode = vk::SharingMode::eExclusive,
				});
		}
		,
		core.device,
		{
			{.size = sizeof(CalApp::VertexInput) * quad.size()},
			{.size = sizeof(CalApp::InstanceDataInput) * instanceDataInputs.size()},
		}
		);
	EngineBase::MemParty vertMem = {
		core.device.allocateMemory({
			.allocationSize = 100000,
			.memoryTypeIndex = EngineBase::memTypeChoose(
				core.physicalDevice,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
		}),
		100000
	};
	EngineBase::resourceBindMemory(vertMem.handle, vertexInputBuffers.handles);
	auto vertMappedMem = vertMem.handle.mapMemory(0, vertMem.memSize);
	EngineBase::resourceCopyMemory2Cpu(vertMappedMem, std::vector<std::vector<CalApp::VertexInput>>{ quad });
	EngineBase::resourceCopyMemory2Cpu(
		vertMappedMem,
		std::vector<std::vector<CalApp::InstanceDataInput>>{ instanceDataInputs },
		sizeof(CalApp::VertexInput)* quad.size());

	/* index Buffer */
	EngineBase::HandleManager<vk::raii::Buffer> indexBuffers(
		[]
		(const vk::raii::Device& device, EngineBase::HandleManager<vk::raii::Buffer>::CREATEINPUT inputs)
		{
			return device.createBuffer({
					.size = inputs.size,
					.usage = vk::BufferUsageFlagBits::eIndexBuffer,
					.sharingMode = vk::SharingMode::eExclusive,
				});
		}
		,
		core.device,
		{
			{.size = sizeof(uint32_t) * indexDatas.size()},
		}
		);

	EngineBase::MemParty indexMem =
	{
		core.device.allocateMemory(
		{
			.allocationSize = 1000,
			.memoryTypeIndex = EngineBase::memTypeChoose(
				core.physicalDevice,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)}),
		1000
	};

	EngineBase::resourceBindMemory(indexMem.handle, indexBuffers.handles);
	auto indexMappedMem = indexMem.handle.mapMemory(0, indexMem.memSize);
	EngineBase::resourceCopyMemory2Cpu(indexMappedMem, std::vector<std::vector<uint32_t>>{ indexDatas });

	/* uiPipeline and descriptorSet configs */
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		{
			.type = vk::DescriptorType::eSampler,
			.descriptorCount = 1,
		},
		{
			.type = vk::DescriptorType::eSampledImage,
			.descriptorCount = 1,
		}
	};
	auto descPool = core.device.createDescriptorPool(
		{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = 32,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data(),
		}
		);
	std::vector<vk::DescriptorSetLayoutBinding> setBindings = {
		{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eSampler,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eFragment,
		},
		{
			.binding = 1,
			.descriptorType = vk::DescriptorType::eSampledImage,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eFragment,
		}
	};

	auto setLayout = core.device.createDescriptorSetLayout(
		{
			.bindingCount = (uint32_t)setBindings.size(),
			.pBindings = setBindings.data(),
		}
		);

	auto descriptorSets = core.device.allocateDescriptorSets(
		{
			.descriptorPool = descPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &*setLayout,
		}
		);

	const vk::DescriptorImageInfo samplerImageInfo = {
		.sampler = sampler,
		.imageView = samplerImageView,
		.imageLayout = vk::ImageLayout::eReadOnlyOptimal,
	};

	const vk::DescriptorImageInfo texImageDescInfo = {
		.sampler = sampler,
		.imageView = samplerImageView,
		.imageLayout = vk::ImageLayout::eReadOnlyOptimal,
	};


	core.device.updateDescriptorSets(
		{
			vk::WriteDescriptorSet
			{
				.dstSet = *descriptorSets[0],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eSampler,
				.pImageInfo = &samplerImageInfo,
			},
			vk::WriteDescriptorSet
			{
				.dstSet = *descriptorSets[0],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eSampledImage,
				.pImageInfo = &texImageDescInfo,
			}
		},
		{}
	);

	auto uiPipelineLayout = core.device.createPipelineLayout(
		{
			.setLayoutCount = 1,
			.pSetLayouts = &*setLayout,
		}
		);
	auto uiPipeline = EngineBase::gBasicPipelineBuilder(
		core.device,
		{ "shaders/uiVert.spv", "shaders/uiFrag.spv" },
		{
			{
				.binding = 0,
				.stride = sizeof(CalApp::VertexInput),
				.inputRate = vk::VertexInputRate::eVertex,
			},
			{
				.binding = 1,
				.stride = sizeof(CalApp::InstanceDataInput),
				.inputRate = vk::VertexInputRate::eInstance,
			}
		},
		{
			{
				.location = 0,
				.binding = 0,
				.format = vk::Format::eR32G32Sfloat,
				.offset = offsetof(CalApp::VertexInput, position)
			} ,
			{
				.location = 1,
				.binding = 0,
				.format = vk::Format::eR32G32Sfloat,
				.offset = offsetof(CalApp::VertexInput, uv)
			},
			{
				.location = 2,
				.binding = 1,
				.format = vk::Format::eR32G32Sfloat,
				.offset = offsetof(CalApp::InstanceDataInput, posOffset),
			},
			{
				.location = 3,
				.binding = 1,
				.format = vk::Format::eR32G32Sfloat,
				.offset = offsetof(CalApp::InstanceDataInput, uvOffset),
			}
		},
		{
			{
				.x = 0.f,
				.y = 0.f,
				.width = (float)windowExtent.width,
				.height = (float)windowExtent.height,
				.minDepth = 0.f,
				.maxDepth = 1.f,
			}
		},
		{
			{
				.offset = {0, 0},
				.extent = windowExtent,
			}
		},
		vk::False,
		uiPipelineLayout,
		renderBaseResources.renderPass
	);

	/* loop */
	vk::raii::Fence submitted = core.device.createFence({ .flags = vk::FenceCreateFlagBits::eSignaled });
	std::vector<vk::raii::Semaphore> renderFinisheds, imageAvas;
	renderFinisheds.reserve(renderBaseResources.framebuffers.size());
	for (int i = 0; i < renderBaseResources.framebuffers.size(); ++i) {
		renderFinisheds.emplace_back(
			core.device.createSemaphore({})
		);
		imageAvas.emplace_back(
			core.device.createSemaphore({})
		);
	}


	size_t frameIdx = 0;
	appBase.loop(
		[&]
		()
		{
			auto waitFenceRes = core.device.waitForFences({ submitted }, vk::True, UINT64_MAX);
			if (waitFenceRes == vk::Result::eSuccess) {
				core.device.resetFences({ submitted });
			}
			frameIdx = frameIdx + 1 == renderBaseResources.framebuffers.size() ? 0 : frameIdx + 1;
			auto imageNext = renderBaseResources.swapchain.acquireNextImage(UINT64_MAX, { *imageAvas[frameIdx] }, {});

			CalApp::cmdRecord(
				cmd,
				imageNext.value,
				windowExtent,
				renderBaseResources,
				uiPipeline,
				vertexInputBuffers.handles,
				indexBuffers.handles,
				(uint32_t)indexDatas.size(),
				(uint32_t)instanceDataInputs.size(),
				descriptorSets,
				uiPipelineLayout);

			const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eFragmentShader };
			queue.submit(
				{
					{
						.waitSemaphoreCount = 1,
						.pWaitSemaphores = &*imageAvas[frameIdx],
						.pWaitDstStageMask = waitStages,
						.commandBufferCount = 1,
						.pCommandBuffers = &*cmd,
						.signalSemaphoreCount = 1,
						.pSignalSemaphores = &*renderFinisheds[imageNext.value],
					}
				},
				*submitted);

			auto presentRes = queue.presentKHR(
				{
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &*renderFinisheds[imageNext.value],
					.swapchainCount = 1,
					.pSwapchains = &*renderBaseResources.swapchain,
					.pImageIndices = &imageNext.value,
					.pResults = nullptr,
				});

			if (presentRes != vk::Result::eSuccess) {

				appBase.recreateSwapchain(renderBaseResources, windowExtent);
			}

		});

	return 0;
}