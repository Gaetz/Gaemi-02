#pragma once 
#include <vk_types.h>

namespace vkutil {

    bool LoadShaderModule(const char* file_path, VkDevice device, VkShaderModule* out_shader_module);
};

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages;

    VkPipelineInputAssemblyStateCreateInfo _input_assembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _color_blend_attachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipeline_layout;
    VkPipelineDepthStencilStateCreateInfo _depth_stencil;
    VkPipelineRenderingCreateInfo _render_info;
    VkFormat _color_attachment_format;

    PipelineBuilder(){ Clear(); }

    // Clear all the structs we need back to 0 with their correct stype
    void Clear();

    VkPipeline BuildPipeline(VkDevice device);
    void SetShaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cull_mode, VkFrontFace front_face);
    void SetMultisamplingNone();
    void DisableBlending();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);
    void DisableDepthTest();

};
