//> includes
#include "engine.h"
#include "SDL3/SDL_init.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>
#include <VkBootstrap.h>

#include <chrono>
#include <thread>

Engine* loaded_engine = nullptr;

Engine& Engine::Get() { return *loaded_engine; }

void Engine::Init()
{
    // Only one engine initialization is allowed with the application.
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
    _window = SDL_CreateWindow(
        "Vulkan Engine",
        _window_extent.width,
        _window_extent.height,
        window_flags);

    // Initialize Vulkan
    InitVulkan();
    InitSwapchain();
    InitCommands();
    InitSyncStructures();

    // Everything went fine
    _is_initialized = true;
}

void Engine::InitVulkan() {
    bool use_validation_layers = true;

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Gaem")
        .request_validation_layers(use_validation_layers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();
    vkb::Instance vkb_instance = inst_ret.value();

    _instance = vkb_instance.instance;
    _debug_messenger = vkb_instance.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, nullptr, &_surface);

    //vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    //vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // Use vkbootstrap to select a gpu.
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{ vkb_instance };
    vkb::PhysicalDevice physical_device = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(_surface)
        .select()
        .value();

    // Create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{ physical_device };
    vkb::Device vkb_device = deviceBuilder.build().value();
    _device = vkb_device.device;
    _chosenGPU = physical_device.physical_device;

    // Use vkbootstrap to get a Graphics queue
    _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void Engine::InitSwapchain() {
    CreateSwapchain(_window_extent.width, _window_extent.height);
}

void Engine::InitCommands()
{
    // Create a command pool for commands submitted to the graphics queue.
    // We also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_info = vkinit::CommandPoolCreateInfo(_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[i]._command_pool));

        // Allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::CommandBufferAllocateInfo(_frames[i]._command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._main_command_buffer));
    }
}
void Engine::InitSyncStructures() {}

void Engine::CreateSwapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder{ _chosenGPU,_device,_surface };

    _swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        //use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchain_extent = vkb_swapchain.extent;
    // Store swapchain and its related images
    _swapchain = vkb_swapchain.swapchain;
    _swapchain_images = vkb_swapchain.get_images().value();
    _swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void Engine::DestroySwapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < _swapchain_image_views.size(); i++) {

        vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    }
}

void Engine::Clean()
{
    if (_is_initialized) {
        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(_device, _frames[i]._command_pool, nullptr);
        }

        DestroySwapchain();

        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);

        vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loaded_engine = nullptr;
}

void Engine::Draw()
{
    // nothing yet
}

void Engine::Run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_EVENT_QUIT)
                bQuit = true;

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                _stop_rendering = true;
            }
            else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                _stop_rendering = false;
            }
        }

        // do not draw if we are minimized
        if (_stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Draw();
    }
}