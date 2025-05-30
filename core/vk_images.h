#pragma once

namespace vkutil {

    void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);

    void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D src_size, VkExtent2D dst_size);

};