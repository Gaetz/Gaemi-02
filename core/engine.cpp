//> includes
#include "engine.h"
#include "SDL3/SDL_init.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>
#include <VkBootstrap.h>
#include <vk_images.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <thread>
#include <vk_pipelines.h>

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
    InitDescriptors();
    InitPipelines();
    InitImGUI();

    // Everything went fine
    _is_initialized = true;
}

void Engine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(_device, 1, &_immFence));
    VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

    VkCommandBuffer cmd = _immCommandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = vkinit::CommandBufferSubmitInfo(cmd);
    const VkSubmitInfo2 submit = vkinit::SubmitInfo(&cmd_info, nullptr, nullptr);

    // Submit command buffer to the queue and execute it.
    //  _immFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, _immFence));

    VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));

    /* One way to improve it would be to run it on a different queue than the graphics queue,
     * and that way we could overlap the execution from this with the main render loop.
     */
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

    // vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
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
    _chosen_GPU = physical_device.physical_device;

    // Use vkbootstrap to get a Graphics queue
    _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize the memory allocator
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = _chosen_GPU;
    allocator_info.device = _device;
    allocator_info.instance = _instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocator_info, &_allocator);

    _main_deletion_queue.PushFunction([&]() {
        vmaDestroyAllocator(_allocator);
    });
}

void Engine::InitSwapchain() {
    CreateSwapchain(_window_extent.width, _window_extent.height);

    // We draw an additional window size image that we will use for rendering
    const VkExtent3D draw_image_extent = {
        _window_extent.width,
        _window_extent.height,
        1
    };

    _drawImage._image_format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcoding the draw format to 32 bit float
    _drawImage._image_extent = draw_image_extent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::ImageCreateInfo(_drawImage._image_format, drawImageUsages, draw_image_extent);

    // For the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate and create the image
    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage._image, &_drawImage._allocation, nullptr);

    // Build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::ImageViewCreateInfo(_drawImage._image_format, _drawImage._image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage._image_view));

    _main_deletion_queue.PushFunction([=]() {
        vkDestroyImageView(_device, _drawImage._image_view, nullptr);
        vmaDestroyImage(_allocator, _drawImage._image, _drawImage._allocation);
    });
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

    // ImGUI command pool
    VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_immCommandPool));
    VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::CommandBufferAllocateInfo(_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_immCommandBuffer));

    _main_deletion_queue.PushFunction([=]() {
        vkDestroyCommandPool(_device, _immCommandPool, nullptr);
    });
}
void Engine::InitSyncStructures()
{
    // Create syncronization structures
    // One fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain.
    // We want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fence_create_info = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_create_info = vkinit::SemaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i]._render_fence));

        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i]._render_semaphore));
    }

    // Fence for immediate submit
    VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_immFence));
    _main_deletion_queue.PushFunction([=]() { vkDestroyFence(_device, _immFence, nullptr); });
}

void Engine::InitDescriptors()
{
    // Create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
    {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
    };

    globalDescriptorAllocator.InitPool(_device, 10, sizes);

    // Make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _draw_image_descriptor_layout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // Allocate a descriptor set for our draw image
    _draw_image_descriptors = globalDescriptorAllocator.Allocate(_device,_draw_image_descriptor_layout);

    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_info.imageView = _drawImage._image_view;

    VkWriteDescriptorSet draw_image_write = {};
    draw_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    draw_image_write.pNext = nullptr;

    draw_image_write.dstBinding = 0;
    draw_image_write.dstSet = _draw_image_descriptors;
    draw_image_write.descriptorCount = 1;
    draw_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    draw_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    draw_image_write.pImageInfo = &img_info;

    vkUpdateDescriptorSets(_device, 1, &draw_image_write, 0, nullptr);

    // Clean up
    _main_deletion_queue.PushFunction([&]() {
        globalDescriptorAllocator.DestroyPool(_device);
        vkDestroyDescriptorSetLayout(_device, _draw_image_descriptor_layout, nullptr);
    });
}

void Engine::InitPipelines()
{
    InitBackgroundPipelines();
}

void Engine::InitBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo compute_layout{};
    compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compute_layout.pNext = nullptr;
    compute_layout.pSetLayouts = &_draw_image_descriptor_layout;
    compute_layout.setLayoutCount = 1;

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(ComputePushConstants) ;
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    compute_layout.pPushConstantRanges = &push_constant;
    compute_layout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &compute_layout, nullptr, &_gradient_pipeline_layout));

    VkShaderModule compute_draw_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/gradient_color.comp.spv", _device, &compute_draw_shader))
    {
        fmt::print("Error when building the compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext = nullptr;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = compute_draw_shader;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo compute_pipeline_create_info{};
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.layout = _gradient_pipeline_layout;
    compute_pipeline_create_info.stage = stage_info;

    VK_CHECK(vkCreateComputePipelines(_device,VK_NULL_HANDLE,1,&compute_pipeline_create_info, nullptr, &_gradient_pipeline));

    vkDestroyShaderModule(_device, compute_draw_shader, nullptr);

    _main_deletion_queue.PushFunction([&]() {
        vkDestroyPipelineLayout(_device, _gradient_pipeline_layout, nullptr);
        vkDestroyPipeline(_device, _gradient_pipeline, nullptr);
    });
}

void Engine::InitImGUI()
{
	// 1: create descriptor pool for IMGUI
	//  the size of the pool is very oversize, but it's copied from imgui demo
	//  itself.
	VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

	// 2: initialize imgui library

	// this initializes the core structures of imgui
	ImGui::CreateContext();

	// this initializes imgui for SDL
	ImGui_ImplSDL3_InitForVulkan(_window);

	// this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosen_GPU;
	init_info.Device = _device;
	init_info.Queue = _graphics_queue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.UseDynamicRendering = true;

	// Dynamic rendering parameters for imgui to use
	init_info.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &_swapchain_image_format;


	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

	// Add destroy the imgui created structures
	_main_deletion_queue.PushFunction([=]() {
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
	});
}

void Engine::CreateSwapchain(const uint32_t width, const uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder{ _chosen_GPU,_device,_surface };

    _swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchain_image_format, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        // use vsync present mode
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

void Engine::DrawImGUI(const VkCommandBuffer cmd, const VkImageView target_image_view) const
{
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(target_image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo render_info = vkinit::RenderingInfo(_swapchain_extent, &colorAttachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

void Engine::Clean()
{
    if (_is_initialized) {
        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            // Already written from before
            vkDestroyCommandPool(_device, _frames[i]._command_pool, nullptr);

            // Destroy sync objects
            vkDestroyFence(_device, _frames[i]._render_fence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._render_semaphore, nullptr);
            vkDestroySemaphore(_device ,_frames[i]._swapchain_semaphore, nullptr);
        }

        _main_deletion_queue.Flush();

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
    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &GetCurrentFrame()._render_fence, true, 1000000000));
    GetCurrentFrame()._deletion_queue.Flush();
    VK_CHECK(vkResetFences(_device, 1, &GetCurrentFrame()._render_fence));

    // Request image from the swapchain
    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, GetCurrentFrame()._swapchain_semaphore, nullptr, &swapchain_image_index));

    // Now that we are sure that the commands finished executing, we can safely
    // reset the command buffer and restart recording commands
    const VkCommandBuffer cmd = GetCurrentFrame()._main_command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    const VkCommandBufferBeginInfo cmd_begin_info = vkinit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // We will use this command buffer exactly on
    _draw_extent.width = _drawImage._image_extent.width;
    _draw_extent.height = _drawImage._image_extent.height;
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // Transition our main draw image into a general layout so we can write into it
    // we will overwrite it all so we don't care about what was the older layout
    vkutil::TransitionImage(cmd, _drawImage._image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    DrawBackground(cmd);

    // Transition the draw image and the swapchain image into their correct transfer layouts
    vkutil::TransitionImage(cmd, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::TransitionImage(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Execute a copy from the draw image into the swapchain
    vkutil::CopyImageToImage(cmd, _drawImage._image, _swapchain_images[swapchain_image_index], _draw_extent, _swapchain_extent);

    // Draw imgui into the swapchain image
    DrawImGUI(cmd, _swapchain_image_views[swapchain_image_index]);

    // Set the swapchain image layout to Present so we can show it on the screen
    vkutil::TransitionImage(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // Prepare the submission to the queue:
    // - we want to wait on the _swapchain_semaphore, as that semaphore is signaled when the swapchain is ready
    // - we will signal the _render_semaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmd_info = vkinit::CommandBufferSubmitInfo(cmd);
    VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,GetCurrentFrame()._swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, GetCurrentFrame()._render_semaphore);

    const VkSubmitInfo2 submit = vkinit::SubmitInfo(&cmd_info, &signal_info, &wait_info);

    // Submit command buffer to the queue and execute it.
    // _render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, GetCurrentFrame()._render_fence));

    // Prepare present
    // This will put the image we just rendered to into the visible window.
    // We want to wait on the _render_semaphore for that,
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &GetCurrentFrame()._render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;

    VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

    // Increase the number of frames drawn
    _frame_number++;
}

void Engine::DrawBackground(VkCommandBuffer cmd)
{
    // Make a clear-color from the frame number. This will flash with a 120-frame period.
    float flash = std::abs(std::sin(static_cast<float>(_frame_number) / 120.f));
    VkClearColorValue clear_value = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clear_range = vkinit::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Clear image
    //vkCmdClearColorImage(cmd, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clear_range);

    // Bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradient_pipeline);

    // Bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradient_pipeline_layout, 0, 1, &_draw_image_descriptors, 0, nullptr);

    ComputePushConstants pc;
    pc.data1 = glm::vec4(1, 0, 0, 1);
    pc.data2 = glm::vec4(0, 0, 1, 1);
    vkCmdPushConstants(cmd, _gradient_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);

    // Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(_draw_extent.width / 16.0)), static_cast<uint32_t>(std::ceil(_draw_extent.height / 16.0)), 1);
}

void Engine::Run()
{
    SDL_Event e;
    bool quit = false;

    // Main loop
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT)
                quit = true;

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                _stop_rendering = true;
            }
            else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                _stop_rendering = false;
            }
        }

        // Send SDL event to imgui for handling
        ImGui_ImplSDL3_ProcessEvent(&e);

        // Do not draw if we are minimized
        if (_stop_rendering) {
            // Throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Some imgui to test
        ImGui::ShowDemoWindow();

        // Make imgui calculate internal draw structures
        ImGui::Render();

        Draw();
    }
}