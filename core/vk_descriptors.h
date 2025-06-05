#pragma once

#include <vk_types.h>

class DescriptorLayoutBuilder
{
public:
    std::vector<VkDescriptorSetLayoutBinding> _bindings;

    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shader_stages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};

class DescriptorAllocator
{
public:
    struct PoolSizeRatio{
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool _pool;

    void InitPool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void ClearDescriptors(VkDevice device);
    void DestroyPool(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
};