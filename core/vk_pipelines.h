#pragma once 
#include <vk_types.h>

namespace vkutil {

    bool LoadShaderModule(const char* file_path, VkDevice device, VkShaderModule* out_shader_module);
};

class PipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder(){ Clear(); }

    // Clear all the structs we need back to 0 with their correct stype
    void Clear();

    VkPipeline BuildPipeline(VkDevice device);
    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();
    void DisableBlending();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);
    void DisableDepthTest();

};
