#pragma once

namespace vkutil {

    void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout current_layout, VkImageLayout new_layout);

};