#pragma once

#include <vk_descriptors.h>
#include <vk_types.h>

constexpr unsigned int FRAME_OVERLAP = 2;

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void PushFunction(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	/**
	 * Remove stored functions from the deletion queue and execute them.
	 *
	 * If we need to delete thousands of objects and want them deleted faster,
	 * a better implementation would be to store arrays of vulkan handles of various
	 * types such as VkImage, VkBuffer, and so on. And then delete those from a loop.
	 */
	void Flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
			(*it)(); //call functors
		}

		deletors.clear();
	}
};

struct FrameData {
	VkCommandPool _command_pool;
	VkCommandBuffer _main_command_buffer;
	VkSemaphore _swapchain_semaphore, _render_semaphore;
	VkFence _render_fence;
	DeletionQueue _deletion_queue;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
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
	AllocatedImage _drawImage;
	VkExtent2D _draw_extent;
	VkPipeline _gradient_pipeline;
	VkPipelineLayout _gradient_pipeline_layout;

	// Immediate submit structures
	VkFence _immFence;
	VkCommandBuffer _immCommandBuffer;
	VkCommandPool _immCommandPool;

	static Engine& Get();

	//initializes everything in the engine
	void Init();

	//shuts down the engine
	void Clean();

	//draw loop
	void Draw();
	void DrawBackground(VkCommandBuffer cmd);

	//run main loop
	void Run();

	FrameData& GetCurrentFrame() { return _frames[_frame_number % FRAME_OVERLAP]; };

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();
	void InitDescriptors();
	void InitPipelines();
	void InitBackgroundPipelines();
	void InitImGUI();

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();

	void DrawImGUI(VkCommandBuffer cmd, VkImageView target_image_view) const;
};
