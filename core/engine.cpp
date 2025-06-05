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

#include <glm/gtx/transform.hpp>

Engine* loaded_engine = nullptr;

Engine& Engine::Get() { return *loaded_engine; }

void Engine::Init() {
    // Only one engine initialization is allowed with the application.
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);
    constexpr SDL_WindowFlags WINDOW_FLAGS = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    _window = SDL_CreateWindow(
        "Vulkan Engine",
        _window_extent.width,
        _window_extent.height,
        WINDOW_FLAGS);

    // Initialize Vulkan
    InitVulkan();
    InitSwapchain();
    InitCommands();
    InitSyncStructures();
    InitDescriptors();
    InitPipelines();
    InitImGUI();
    InitDefaultData();

    // Everything went fine
    _is_initialized = true;
}

void Engine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) const
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
    VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features.dynamicRendering = true;
    features.synchronization2 = true;

    // vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // Use vkbootstrap to select a gpu.
    // We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
    vkb::PhysicalDeviceSelector selector{vkb_instance};
    vkb::PhysicalDevice physical_device = selector
                                          .set_minimum_version(1, 3)
                                          .set_required_features_13(features)
                                          .set_required_features_12(features12)
                                          .set_surface(_surface)
                                          .select()
                                          .value();

    // Create the final vulkan device
    vkb::DeviceBuilder deviceBuilder{physical_device};
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

    _main_deletion_queue.PushFunction([&]() { vmaDestroyAllocator(_allocator); });
}

void Engine::InitSwapchain() {
    CreateSwapchain(_window_extent.width, _window_extent.height);

    // We draw an additional window size image that we will use for rendering
    const VkExtent3D draw_image_extent = {
        _window_extent.width,
        _window_extent.height,
        1
    };
    _draw_image.image_format = VK_FORMAT_R16G16B16A16_SFLOAT; // hardcoding the draw format to 32 bit float
    _draw_image.image_extent = draw_image_extent;

    VkImageUsageFlags draw_image_usages{};
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_STORAGE_BIT;
    draw_image_usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkImageCreateInfo rimg_info = vkinit::ImageCreateInfo(_draw_image.image_format, draw_image_usages, draw_image_extent);

    // For the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimg_allocinfo = {};
    rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_draw_image.image, &_draw_image.allocation, nullptr);
    VkImageViewCreateInfo rview_info = vkinit::ImageViewCreateInfo(_draw_image.image_format, _draw_image.image,
                                                                   VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_draw_image.image_view));

    // Depth image
    _depth_image.image_format = VK_FORMAT_D32_SFLOAT;
    _depth_image.image_extent = draw_image_extent;
    VkImageUsageFlags depth_image_usages{};
    depth_image_usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo dimg_info = vkinit::ImageCreateInfo(_depth_image.image_format, depth_image_usages, draw_image_extent);
    vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depth_image.image, &_depth_image.allocation, nullptr);
    VkImageViewCreateInfo dview_info = vkinit::ImageViewCreateInfo(_depth_image.image_format, _depth_image.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depth_image.image_view));

    // Clean
    _main_deletion_queue.PushFunction([=, this]() {
        vkDestroyImageView(_device, _draw_image.image_view, nullptr);
        vmaDestroyImage(_allocator, _draw_image.image, _draw_image.allocation);

        vkDestroyImageView(_device, _depth_image.image_view, nullptr);
        vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation);
    });
}

void Engine::InitCommands() {
    // Create a command pool for commands submitted to the graphics queue.
    // We also want the pool to allow for resetting of individual command buffers
    VkCommandPoolCreateInfo command_pool_info = vkinit::CommandPoolCreateInfo(
        _graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_frames[i].command_pool));

        // Allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::CommandBufferAllocateInfo(_frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i].main_command_buffer));
    }

    // ImGUI command pool
    VK_CHECK(vkCreateCommandPool(_device, &command_pool_info, nullptr, &_immCommandPool));
    VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::CommandBufferAllocateInfo(_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &_immCommandBuffer));

    _main_deletion_queue.PushFunction([=, this]() { vkDestroyCommandPool(_device, _immCommandPool, nullptr); });
}

void Engine::InitSyncStructures() {
    // Create syncronization structures
    // One fence to control when the gpu has finished rendering the frame,
    // and 2 semaphores to syncronize rendering with swapchain.
    // We want the fence to start signalled so we can wait on it on the first frame
    VkFenceCreateInfo fence_create_info = vkinit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_create_info = vkinit::SemaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_frames[i].render_fence));

        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i].swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &_frames[i].render_semaphore));
    }

    // Fence for immediate submit
    VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &_immFence));
    _main_deletion_queue.PushFunction([=, this]() { vkDestroyFence(_device, _immFence, nullptr); });
}

void Engine::InitDescriptors() {
    // Create a descriptor pool that will hold 10 sets with 1 image each
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
    {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
    };

    globalDescriptorAllocator.InitPool(_device, 10, sizes);

    // Make the descriptor set layout for our compute draw
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        _draw_image_descriptor_layout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    // Allocate a descriptor set for our draw image
    _draw_image_descriptors = globalDescriptorAllocator.Allocate(_device, _draw_image_descriptor_layout);

    VkDescriptorImageInfo img_info{};
    img_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    img_info.imageView = _draw_image.image_view;

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
    //InitTrianglePipeline();
    InitMeshPipeline();
}

void Engine::InitBackgroundPipelines() {
    VkPipelineLayoutCreateInfo compute_layout{};
    compute_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compute_layout.pNext = nullptr;
    compute_layout.pSetLayouts = &_draw_image_descriptor_layout;
    compute_layout.setLayoutCount = 1;

    VkPushConstantRange push_constant{};
    push_constant.offset = 0;
    push_constant.size = sizeof(ComputePushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    compute_layout.pPushConstantRanges = &push_constant;
    compute_layout.pushConstantRangeCount = 1;

    VkPipelineLayout _background_pipeline_layout;
    VK_CHECK(vkCreatePipelineLayout(_device, &compute_layout, nullptr, &_background_pipeline_layout));

    VkShaderModule gradient_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/gradient_color.comp.spv", _device, &gradient_shader))
    {
        fmt::println("Error when building the gradient compute shader \n");
    }

    VkShaderModule sky_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/sky.comp.spv", _device, &sky_shader))
    {
        fmt::println("Error when building the sky compute shader \n");
    }

    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.pNext = nullptr;
    stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_info.module = gradient_shader;
    stage_info.pName = "main";

    VkComputePipelineCreateInfo compute_pipeline_create_info{};
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.pNext = nullptr;
    compute_pipeline_create_info.layout = _background_pipeline_layout;
    compute_pipeline_create_info.stage = stage_info;

    ComputeEffect gradient;
    gradient.layout = _background_pipeline_layout;
    gradient.name = "gradient";
    gradient.data = {};
    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);
    VK_CHECK(
        vkCreateComputePipelines(_device,VK_NULL_HANDLE,1,&compute_pipeline_create_info, nullptr, &gradient.pipeline));

    compute_pipeline_create_info.stage.module = sky_shader;
    ComputeEffect sky;
    sky.layout = _background_pipeline_layout;
    sky.name = "sky";
    sky.data = {};
    sky.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);
    VK_CHECK(
        vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &compute_pipeline_create_info, nullptr, &sky.pipeline));

    _background_effects.push_back(gradient);
    _background_effects.push_back(sky);

    vkDestroyShaderModule(_device, gradient_shader, nullptr);
    vkDestroyShaderModule(_device, sky_shader, nullptr);

    _main_deletion_queue.PushFunction([&]() {
        // Destroy each pipeline from background effects
        for (const auto& effect : _background_effects)
        {
            vkDestroyPipeline(_device, effect.pipeline, nullptr);
        }
        vkDestroyPipelineLayout(_device, _background_effects[0].layout, nullptr);
    });
}

void Engine::InitImGUI() {
    // 1: create descriptor pool for IMGUI
    //  the size of the pool is very oversize, but it's copied from imgui demo
    //  itself.
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

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
    _main_deletion_queue.PushFunction([=, this]() {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(_device, imguiPool, nullptr);
    });
}

void Engine::CreateSwapchain(const uint32_t width, const uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder{_chosen_GPU, _device, _surface};

    _swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
                                   //.use_default_format_selection()
                                   .set_desired_format(VkSurfaceFormatKHR{
                                       .format = _swapchain_image_format,
                                       .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                                   })
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

void Engine::DestroySwapchain() const
{
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < _swapchain_image_views.size(); i++)
    {
        vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    }
}

void Engine::ResizeSwapchain()
{
    vkDeviceWaitIdle(_device);
    DestroySwapchain();
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    _window_extent.width = w;
    _window_extent.height = h;
    CreateSwapchain(_window_extent.width, _window_extent.height);
    _resize_requested = false;
}

AllocatedBuffer Engine::CreateBuffer(const size_t alloc_size, const VkBufferUsageFlags usage, const VmaMemoryUsage memory_usage) const
{
    VkBufferCreateInfo buffer_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    buffer_info.pNext = nullptr;
    buffer_info.size = alloc_size;

    buffer_info.usage = usage;

    VmaAllocationCreateInfo vma_alloc_info = {};
    vma_alloc_info.usage = memory_usage;
    vma_alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer newBuffer;

    VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_alloc_info, &newBuffer.buffer, &newBuffer.allocation,
        &newBuffer.info));

    return newBuffer;
}

void Engine::DestroyBuffer(const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

void Engine::DrawImGUI(const VkCommandBuffer cmd, const VkImageView target_image_view) const {
    VkRenderingAttachmentInfo color_attachment = vkinit::AttachmentInfo(
        target_image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    const VkRenderingInfo render_info = vkinit::RenderingInfo(_swapchain_extent, &color_attachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRendering(cmd);
}

void Engine::DrawBackground(VkCommandBuffer cmd) {
    /*
    // Make a clear-color from the frame number. This will flash with a 120-frame period.
    float flash = std::abs(std::sin(static_cast<float>(_frame_number) / 120.f));
    VkClearColorValue clear_value = {{0.0f, 0.0f, flash, 1.0f}};

    VkImageSubresourceRange clear_range = vkinit::ImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

    // Clear image
    vkCmdClearColorImage(cmd, _drawImage._image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clear_range);
    */

    ComputeEffect& effect = _background_effects[_current_background_effect];
    // Bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
    // Bind the descriptor set containing the draw image for the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.layout, 0, 1, &_draw_image_descriptors, 0,
                            nullptr);

    const ComputePushConstants& pc = _background_effects[_current_background_effect].data;
    vkCmdPushConstants(cmd, effect.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pc);

    // Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
    vkCmdDispatch(cmd, static_cast<uint32_t>(std::ceil(_draw_extent.width / 16.0)),
                  static_cast<uint32_t>(std::ceil(_draw_extent.height / 16.0)), 1);
}

void Engine::DrawGeometry(VkCommandBuffer cmd) const
{
    // Begin a render pass connected to our draw image, not to the swapchain image
    VkRenderingAttachmentInfo color_attachment = vkinit::AttachmentInfo(_draw_image.image_view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depth_attachment = vkinit::DepthAttachmentInfo(_depth_image.image_view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    const VkRenderingInfo render_info = vkinit::RenderingInfo(_draw_extent, &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    /*
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _triangle_pipeline);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(_draw_extent.width);
    viewport.height = static_cast<float>(_draw_extent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _draw_extent.width;
    scissor.extent.height = _draw_extent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Test triangle
    vkCmdDraw(cmd, 3, 1, 0, 0);


    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline);

    GPUDrawPushConstants push_constants;
    push_constants.world_matrix = glm::mat4{ 1.f };
    push_constants.vertex_buffer = _rectangle.vertex_buffer_address;

    vkCmdPushConstants(cmd, _mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
    vkCmdBindIndexBuffer(cmd, _rectangle.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Test rectangle
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
    */

    // Test mesh
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(_draw_extent.width);
    viewport.height = static_cast<float>(_draw_extent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = _draw_extent.width;
    scissor.extent.height = _draw_extent.height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    const glm::mat4 view = glm::translate(glm::vec3{ 0,0,-5 });
    glm::mat4 projection = glm::perspective(glm::radians(70.f), static_cast<float>(_draw_extent.width) / static_cast<float>(_draw_extent.height), 10000.f, 0.1f);
    // Invert the Y direction on projection matrix so that we are more similar to opengl and gltf axis
    projection[1][1] *= -1;

    GPUDrawPushConstants push_constants;
    push_constants.vertex_buffer = _test_meshes[2]->meshBuffers.vertex_buffer_address;
    push_constants.world_matrix = projection * view;

    vkCmdPushConstants(cmd, _mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
    vkCmdBindIndexBuffer(cmd, _test_meshes[2]->meshBuffers.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, _test_meshes[2]->surfaces[0].count, 1, _test_meshes[2]->surfaces[0].startIndex, 0, 0);

    vkCmdEndRendering(cmd);
}

/*
void Engine::InitTrianglePipeline()
{
    VkShaderModule triangle_frag_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/colored_triangle.frag.spv", _device, &triangle_frag_shader)) {
        fmt::println("Error when building the triangle fragment shader module");
    }
    else {
        fmt::println("Triangle fragment shader succesfully loaded");
    }

    VkShaderModule triangle_vertex_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/colored_triangle.vert.spv", _device, &triangle_vertex_shader)) {
        fmt::println("Error when building the triangle vertex shader module");
    }
    else {
        fmt::println("Triangle vertex shader succesfully loaded");
    }

    // Build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::PipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_triangle_pipeline_layout));

    PipelineBuilder pipeline_builder;
    // Use the triangle layout we created
    pipeline_builder._pipeline_layout = _triangle_pipeline_layout;
    // Connecting the vertex and pixel shaders to the pipeline
    pipeline_builder.SetShaders(triangle_vertex_shader, triangle_frag_shader);
    // It will draw triangles
    pipeline_builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // Filled triangles
    pipeline_builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    // No backface culling
    pipeline_builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // No multisampling
    pipeline_builder.SetMultisamplingNone();
    // No blending
    pipeline_builder.DisableBlending();
    // No depth testing
    pipeline_builder.DisableDepthTest();

    // Connect the image format we will draw into, from draw image
    pipeline_builder.SetColorAttachmentFormat(_draw_image.image_format);
    pipeline_builder.SetDepthFormat(_depth_image.image_format);

    // Finally build the pipeline
    _triangle_pipeline = pipeline_builder.BuildPipeline(_device);

    // Clean structures
    vkDestroyShaderModule(_device, triangle_frag_shader, nullptr);
    vkDestroyShaderModule(_device, triangle_vertex_shader, nullptr);

    _main_deletion_queue.PushFunction([&]() {
        vkDestroyPipelineLayout(_device, _triangle_pipeline_layout, nullptr);
        vkDestroyPipeline(_device, _triangle_pipeline, nullptr);
    });
}
*/

void Engine::InitMeshPipeline()
{
    VkShaderModule triangle_frag_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/colored_triangle.frag.spv", _device, &triangle_frag_shader)) {
        fmt::print("Error when building the triangle fragment shader module");
    }
    else {
        fmt::print("Triangle fragment shader succesfully loaded");
    }

    VkShaderModule triangle_vertex_shader;
    if (!vkutil::LoadShaderModule("../assets/shaders/colored_triangle_mesh.vert.spv", _device, &triangle_vertex_shader)) {
        fmt::print("Error when building the triangle vertex shader module");
    }
    else {
        fmt::print("Triangle vertex shader succesfully loaded");
    }

    VkPushConstantRange buffer_range{};
    buffer_range.offset = 0;
    buffer_range.size = sizeof(GPUDrawPushConstants);
    buffer_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::PipelineLayoutCreateInfo();
    pipeline_layout_info.pPushConstantRanges = &buffer_range;
    pipeline_layout_info.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_mesh_pipeline_layout));

    PipelineBuilder pipeline_builder;
    // Use the triangle layout we created
    pipeline_builder._pipeline_layout = _mesh_pipeline_layout;
    // Connecting the vertex and pixel shaders to the pipeline
    pipeline_builder.SetShaders(triangle_vertex_shader, triangle_frag_shader);
    // It will draw triangles
    pipeline_builder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // Filled triangles
    pipeline_builder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    // No backface culling
    pipeline_builder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // No multisampling
    pipeline_builder.SetMultisamplingNone();
    // No blending
    pipeline_builder.EnableBlendingAdditive();
    // Setup depth testing
    pipeline_builder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL); // Depth goes from 1 to 0, so we want to test for greater or equal

    // Connect the image format we will draw into, from draw image
    pipeline_builder.SetColorAttachmentFormat(_draw_image.image_format);
    pipeline_builder.SetDepthFormat(_depth_image.image_format);

    // Finally, build the pipeline
    _mesh_pipeline = pipeline_builder.BuildPipeline(_device);

    // Clean structures
    vkDestroyShaderModule(_device, triangle_frag_shader, nullptr);
    vkDestroyShaderModule(_device, triangle_vertex_shader, nullptr);
    _main_deletion_queue.PushFunction([&]() {
        vkDestroyPipelineLayout(_device, _mesh_pipeline_layout, nullptr);
        vkDestroyPipeline(_device, _mesh_pipeline, nullptr);
    });
}

void Engine::InitDefaultData()
{
    /*
    std::array<Vertex,4> rect_vertices;

    rect_vertices[0].position = {0.5,-0.5, 0};
    rect_vertices[1].position = {0.5,0.5, 0};
    rect_vertices[2].position = {-0.5,-0.5, 0};
    rect_vertices[3].position = {-0.5,0.5, 0};

    rect_vertices[0].color = {0,0, 0,1};
    rect_vertices[1].color = { 0.5,0.5,0.5 ,1};
    rect_vertices[2].color = { 1,0, 0,1 };
    rect_vertices[3].color = { 0,1, 0,1 };

    std::array<uint32_t,6> rect_indices;

    rect_indices[0] = 0;
    rect_indices[1] = 1;
    rect_indices[2] = 2;

    rect_indices[3] = 2;
    rect_indices[4] = 1;
    rect_indices[5] = 3;

    _rectangle = UploadMeshToGpu(rect_indices, rect_vertices);


    //delete the rectangle data on engine shutdown
    _main_deletion_queue.PushFunction([&](){
        DestroyBuffer(_rectangle.index_buffer);
        DestroyBuffer(_rectangle.vertex_buffer);
    });
    */

    _test_meshes = LoadGltfMeshes(this,"../assets/meshes/basicmesh.glb").value();
}

GPUMeshBuffers Engine::UploadMeshToGpu(const std::span<uint32_t> indices, const std::span<Vertex> vertices)
{
    const size_t vertex_buffer_size = vertices.size() * sizeof(Vertex);
    const size_t index_buffer_size = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    // Create vertex buffer
    newSurface.vertex_buffer = CreateBuffer(vertex_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Find the address of the vertex buffer
    const VkBufferDeviceAddressInfo device_address_info{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertex_buffer.buffer };
    newSurface.vertex_buffer_address = vkGetBufferDeviceAddress(_device, &device_address_info);

    // Create index buffer
    newSurface.index_buffer = CreateBuffer(index_buffer_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Transfer data through staging buffer
    /** Note that this pattern is not very efficient, as we are waiting for the GPU command
     * to fully execute before continuing with our CPU side logic. This is something people
     * generally put on a background thread, whose sole job is to execute uploads like this
     * one, and deleting/reusing the staging buffers.
     */
    const AllocatedBuffer staging = CreateBuffer(vertex_buffer_size + index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data = staging.allocation->GetMappedData();
    // copy vertex buffer
    memcpy(data, vertices.data(), vertex_buffer_size);
    // copy index buffer
    memcpy(static_cast<char*>(data) + vertex_buffer_size, indices.data(), index_buffer_size);

    ImmediateSubmit([&](VkCommandBuffer cmd) {
        VkBufferCopy vertexCopy{ 0 };
        vertexCopy.dstOffset = 0;
        vertexCopy.srcOffset = 0;
        vertexCopy.size = vertex_buffer_size;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertex_buffer.buffer, 1, &vertexCopy);

        VkBufferCopy indexCopy{ 0 };
        indexCopy.dstOffset = 0;
        indexCopy.srcOffset = vertex_buffer_size;
        indexCopy.size = index_buffer_size;

        vkCmdCopyBuffer(cmd, staging.buffer, newSurface.index_buffer.buffer, 1, &indexCopy);
    });

    DestroyBuffer(staging);
    return newSurface;
}

void Engine::Clean() {
    if (_is_initialized)
    {
        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            // Already written from before
            vkDestroyCommandPool(_device, _frames[i].command_pool, nullptr);

            // Destroy sync objects
            vkDestroyFence(_device, _frames[i].render_fence, nullptr);
            vkDestroySemaphore(_device, _frames[i].render_semaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i].swapchain_semaphore, nullptr);
        }

        for (auto& mesh : _test_meshes) {
            DestroyBuffer(mesh->meshBuffers.index_buffer);
            DestroyBuffer(mesh->meshBuffers.vertex_buffer);
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

void Engine::Draw() {
    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &GetCurrentFrame().render_fence, true, 1000000000));
    GetCurrentFrame().deletion_queue.Flush();
    VK_CHECK(vkResetFences(_device, 1, &GetCurrentFrame().render_fence));

    // Request image from the swapchain
    uint32_t swapchain_image_index;
    const VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, GetCurrentFrame().swapchain_semaphore, nullptr, &swapchain_image_index);
    if (e == VK_ERROR_OUT_OF_DATE_KHR) {
        _resize_requested = true;
        return;
    }

    // Now that we are sure that the commands finished executing, we can safely
    // reset the command buffer and restart recording commands
    const VkCommandBuffer cmd = GetCurrentFrame().main_command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    const VkCommandBufferBeginInfo cmd_begin_info = vkinit::CommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // We will use this command buffer exactly on
    _draw_extent.width = std::min(_swapchain_extent.width, _draw_image.image_extent.width) * _render_scale;
    _draw_extent.height = std::min(_swapchain_extent.height, _draw_image.image_extent.height) * _render_scale;
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // Transition our main draw image into a general layout so we can write into it
    // we will overwrite it all so we don't care about what was the older layout
    vkutil::TransitionImage(cmd, _draw_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    DrawBackground(cmd);

    // Transition the draw and depth images into a color/depth optimal attachment layout so we can render into it
    vkutil::TransitionImage(cmd, _draw_image.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkutil::TransitionImage(cmd, _depth_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    DrawGeometry(cmd);

    // Transition the draw image and the swapchain image into their correct transfer layouts
    vkutil::TransitionImage(cmd, _draw_image.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::TransitionImage(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Execute a copy from the draw image into the swapchain
    vkutil::CopyImageToImage(cmd, _draw_image.image, _swapchain_images[swapchain_image_index], _draw_extent,
                             _swapchain_extent);

    // Draw imgui into the swapchain image
    DrawImGUI(cmd, _swapchain_image_views[swapchain_image_index]);

    // Set the swapchain image layout to Present so we can show it on the screen
    vkutil::TransitionImage(cmd, _swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Finalize the command buffer (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // Prepare the submission to the queue:
    // - we want to wait on the _swapchain_semaphore, as that semaphore is signaled when the swapchain is ready
    // - we will signal the _render_semaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmd_info = vkinit::CommandBufferSubmitInfo(cmd);
    VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                    GetCurrentFrame().swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                                                                      GetCurrentFrame().render_semaphore);

    const VkSubmitInfo2 submit = vkinit::SubmitInfo(&cmd_info, &signal_info, &wait_info);

    // Submit command buffer to the queue and execute it.
    // _render_fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphics_queue, 1, &submit, GetCurrentFrame().render_fence));

    // Prepare present
    // This will put the image we just rendered to into the visible window.
    // We want to wait on the _render_semaphore for that,
    // as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &_swapchain;
    present_info.swapchainCount = 1;
    present_info.pWaitSemaphores = &GetCurrentFrame().render_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.pImageIndices = &swapchain_image_index;

    // TODO: Check why suboptimal
    // VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));
    VkResult present_result = vkQueuePresentKHR(_graphics_queue, &present_info);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR) {
        _resize_requested = true;
    }

    // Increase the number of frames drawn
    _frame_number++;
}



void Engine::Run() {
    SDL_Event e;
    bool quit = false;

    // Main loop
    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_EVENT_QUIT) quit = true;
            if (e.type == SDL_EVENT_WINDOW_MINIMIZED) { _stop_rendering = true; }
            else if (e.type == SDL_EVENT_WINDOW_RESTORED) { _stop_rendering = false; }

            // Send SDL event to imgui for handling
            ImGui_ImplSDL3_ProcessEvent(&e);
        }

        // Do not draw if we are minimized
        if (_stop_rendering)
        {
            // Throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (_resize_requested) {
            ResizeSwapchain();
        }

        // imgui new frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("background"))
        {
            ImGui::SliderFloat("Render Scale",&_render_scale, 0.3f, 1.f);

            ComputeEffect& selected = _background_effects[_current_background_effect];

            ImGui::Text("Selected effect: ", selected.name);

            ImGui::SliderInt("Effect Index", &_current_background_effect, 0,
                             static_cast<int>(_background_effects.size()) - 1);

            ImGui::InputFloat4("data1", reinterpret_cast<float*>(&selected.data.data1));
            ImGui::InputFloat4("data2", reinterpret_cast<float*>(&selected.data.data2));
            ImGui::InputFloat4("data3", reinterpret_cast<float*>(&selected.data.data3));
            ImGui::InputFloat4("data4", reinterpret_cast<float*>(&selected.data.data4));
        }
        ImGui::End();

        // Make imgui calculate internal draw structures
        ImGui::Render();

        Draw();
    }
}
