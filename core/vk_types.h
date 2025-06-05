#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::println("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

struct AllocatedImage {
    VkImage _image;
    VkImageView _image_view;
    VmaAllocation _allocation;
    VkExtent3D _image_extent;
    VkFormat _image_format;
};

struct AllocatedBuffer {
    VkBuffer _buffer;
    VmaAllocation _allocation;
    VmaAllocationInfo _info;
};

struct Vertex
{
    glm::vec3 _position;
    float _uv_x;
    glm::vec3 _normal;
    float _uv_y;
    glm::vec4 _color;
};

// Holds the resources needed for a mesh
struct GPUMeshBuffers
{
    AllocatedBuffer _index_buffer;
    AllocatedBuffer _vertex_buffer;
    VkDeviceAddress _vertex_buffer_address;
};

// Push constants for our mesh object draws
struct GPUDrawPushConstants
{
    glm::mat4 _world_matrix;
    VkDeviceAddress _vertex_buffer;
};