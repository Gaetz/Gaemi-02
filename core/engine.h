#pragma once

#include <vk_types.h>

class Engine {
public:

	bool _is_initialized{ false };
	int _frame_number {0};
	bool _stop_rendering{ false };
	VkExtent2D _windowExtent{ 1700 , 900 };

	VkInstance _instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT _debugMessenger{ VK_NULL_HANDLE };
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	struct SDL_Window* _window{ nullptr };

	static Engine& Get();

	//initializes everything in the engine
	void init();
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
};
