﻿#pragma once

#include <vk_descriptors.h>
#include <vk_types.h>

#include "vk_loader.h"

constexpr unsigned int FRAME_OVERLAP = 2;

class DeletionQueue
{
public:
	void PushFunction(std::function<void()>&& function)
	{
		_deletors.push_back(function);
	}

	/**
	 * Remove stored functions from the deletion queue and execute them.
	 *
	 * If we need to delete thousands of objects and want them deleted faster,
	 * a better implementation would be to store arrays of vulkan handles of various
	 * types such as VkImage, VkBuffer, and so on. And then delete those from a loop.
	 */
	void Flush()
	{
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = _deletors.rbegin(); it != _deletors.rend(); ++it) {
			(*it)(); //call functors
	}

		_deletors.clear();
	}

private:
	std::deque<std::function<void()>> _deletors;
};

struct FrameData {
	VkCommandPool command_pool;
	VkCommandBuffer main_command_buffer;
	VkSemaphore swapchain_semaphore, render_semaphore;
	VkFence render_fence;
	DeletionQueue deletion_queue;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

struct ComputeEffect {
	const char* name;
	VkPipeline pipeline;
	VkPipelineLayout layout;

	ComputePushConstants data;
};

class Engine {
public:
	bool _is_initialized{ false };
	int _frame_number {0};
	bool _stop_rendering{ false };
	VkExtent2D _window_extent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };
	DeletionQueue _main_deletion_queue;

	VkInstance _instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT _debug_messenger{ VK_NULL_HANDLE };
	VkPhysicalDevice _chosen_GPU;
	VkDevice _device;
	VkSurfaceKHR _surface;
	VmaAllocator _allocator;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchain_image_format;

	std::vector<VkImage> _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;
	VkExtent2D _swapchain_extent;
	FrameData _frames[FRAME_OVERLAP];
	VkQueue _graphics_queue;
	uint32_t _graphics_queue_family;

	DescriptorAllocator globalDescriptorAllocator;
	VkDescriptorSet _draw_image_descriptors;
	VkDescriptorSetLayout _draw_image_descriptor_layout;

	// Draw resources
	AllocatedImage _draw_image;
	AllocatedImage _depth_image;
	VkExtent2D _draw_extent;
	std::vector<ComputeEffect> _background_effects;
	int _current_background_effect{0};
	bool _resize_requested{ false };
	float _render_scale{ 1.0f };

	// Immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	static Engine& Get();
	FrameData& GetCurrentFrame() { return _frames[_frame_number % FRAME_OVERLAP]; };

	void Init();
	void Clean();

	// Draw loop
	void Draw();
	void DrawBackground(VkCommandBuffer cmd);

	void Run();

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const;
	GPUMeshBuffers UploadMeshToGpu(std::span<uint32_t> indices, std::span<Vertex> vertices);

private:
	//VkPipelineLayout _triangle_pipeline_layout;
	//VkPipeline _triangle_pipeline;
	//GPUMeshBuffers _rectangle;

	VkPipelineLayout _mesh_pipeline_layout;
	VkPipeline _mesh_pipeline;

	std::vector<std::shared_ptr<MeshAsset>> _test_meshes;

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain() const;
	void ResizeSwapchain();
	AllocatedBuffer CreateBuffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) const;
	void DestroyBuffer(const AllocatedBuffer& buffer) const;

	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitBackgroundPipelines();
	void InitImGUI();

	void DrawImGUI(VkCommandBuffer cmd, VkImageView target_image_view) const;
	void DrawGeometry(VkCommandBuffer cmd) const;

	//void InitTrianglePipeline();
	void InitMeshPipeline();
	void InitDefaultData();

};
