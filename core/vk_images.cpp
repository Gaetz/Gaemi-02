#include <vk_images.h>
#include <vk_initializers.h>

void vkutil::TransitionImage(VkCommandBuffer cmd, const VkImage image, const VkImageLayout current_layout, const VkImageLayout new_layout)
{
    VkImageMemoryBarrier2 image_barrier {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    image_barrier.pNext = nullptr;

    image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    image_barrier.oldLayout = current_layout;
    image_barrier.newLayout = new_layout;

    const VkImageAspectFlags aspect_mask = (new_layout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange = vkinit::ImageSubresourceRange(aspect_mask);
    image_barrier.image = image;

    VkDependencyInfo depInfo {};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &image_barrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void vkutil::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, const VkExtent2D src_size, const VkExtent2D dst_size)
{
    VkImageBlit2 blit_region{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blit_region.srcOffsets[1].x = src_size.width;
    blit_region.srcOffsets[1].y = src_size.height;
    blit_region.srcOffsets[1].z = 1;

    blit_region.dstOffsets[1].x = dst_size.width;
    blit_region.dstOffsets[1].y = dst_size.height;
    blit_region.dstOffsets[1].z = 1;

    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.baseArrayLayer = 0;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcSubresource.mipLevel = 0;

    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.baseArrayLayer = 0;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blit_info{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blit_info.dstImage = destination;
    blit_info.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blit_info.srcImage = source;
    blit_info.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blit_info.filter = VK_FILTER_LINEAR;
    blit_info.regionCount = 1;
    blit_info.pRegions = &blit_region;

    vkCmdBlitImage2(cmd, &blit_info);
}