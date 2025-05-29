#pragma once

#include <vk_types.h>


struct FrameData {
	VkCommandPool _command_pool;
	VkCommandBuffer _main_command_buffer;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class Engine {
public:

	bool _is_initialized{ false };
	int _frame_number {0};
	bool _stop_rendering{ false };
	VkExtent2D _window_extent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

	VkInstance _instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT _debug_messenger{ VK_NULL_HANDLE };
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchain_image_format;

	std::vector<VkImage> _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;
	VkExtent2D _swapchain_extent;
	FrameData _frames[FRAME_OVERLAP];
	VkQueue _graphics_queue;
	uint32_t _graphics_queue_family;

	static Engine& Get();

	//initializes everything in the engine
	void Init();

	//shuts down the engine
	void Clean();

	//draw loop
	void Draw();

	//run main loop
	void Run();

	FrameData& GetCurrentFrame() { return _frames[_frame_number % FRAME_OVERLAP]; };

private:
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitSyncStructures();

	void CreateSwapchain(uint32_t width, uint32_t height);
	void DestroySwapchain();
};
