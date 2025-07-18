﻿#pragma once

#include <vk_types.h>

namespace vkinit {
//> init_cmd
VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1);
//< init_cmd

VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo CommandBufferSubmitInfo(VkCommandBuffer cmd);

VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo SemaphoreCreateInfo(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signal_semaphore_info,
    VkSemaphoreSubmitInfo* wait_semaphore_info);
VkPresentInfoKHR PresentInfo();

VkRenderingAttachmentInfo AttachmentInfo(VkImageView view, VkClearValue* clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo DepthAttachmentInfo(VkImageView view,
    VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo RenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment,
    VkRenderingAttachmentInfo* depthAttachment);

VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask);

VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags,
    uint32_t binding);
VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(VkDescriptorSetLayoutBinding* bindings,
    uint32_t bindingCount);
VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet,
    VkDescriptorImageInfo* imageInfo, uint32_t binding);
VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet,
    VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
    VkShaderModule shaderModule,
    const char * entry = "main");
} // namespace vkinit
