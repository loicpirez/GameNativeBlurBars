#include "BlurBarsVulkan.h"
#include "blur_bars_vert.h"
#include "blur_bars_frag.h"
#include <algorithm>
#include <stdexcept>

namespace { void check(VkResult result) { if (result != VK_SUCCESS) throw std::runtime_error("Blur bars Vulkan allocation/pipeline failed"); } }

BlurBarsVulkan::Rect BlurBarsVulkan::sceneRect(float ox,float oy,float sx,float sy,float containerW,float containerH,float contentW,float contentH) {
    if(containerW<=0.f || containerH<=0.f) return {0,0,1,1};
    return {std::clamp(ox/containerW,0.f,1.f),std::clamp(oy/containerH,0.f,1.f),
        std::clamp((ox+contentW*sx)/containerW,0.f,1.f),std::clamp((oy+contentH*sy)/containerH,0.f,1.f)};
}

BlurBarsVulkan::BlurBarsVulkan(VkDevice d, PFN_vkGetDeviceProcAddr getProc,
    const VkPhysicalDeviceMemoryProperties& m, VkFormat f) : device(d), memory(m), format(f) {
#define BB_LOAD(name) name = reinterpret_cast<PFN_vk##name>(getProc(device, "vk" #name));
    BB_FUNCTIONS(BB_LOAD)
#undef BB_LOAD
    try { create(); } catch (...) {
        // The owning renderer keeps the normal black-bar path if setup fails.
        // Explicit cleanup also covers partially initialized construction.
        if (pipeline) DestroyPipeline(device,pipeline,nullptr);
        if (layout) DestroyPipelineLayout(device,layout,nullptr);
        if (setLayout) DestroyDescriptorSetLayout(device,setLayout,nullptr);
        if (sampler) DestroySampler(device,sampler,nullptr);
        if (pass) DestroyRenderPass(device,pass,nullptr);
        throw;
    }
}

void BlurBarsVulkan::create() {
    static_assert(sizeof(Params)==36, "GLSL push constant ABI");
    VkAttachmentDescription a{};
    a.format=format; a.samples=VK_SAMPLE_COUNT_1_BIT;
    a.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; a.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    a.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; a.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    a.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; a.finalLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference ref{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub{}; sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; sub.colorAttachmentCount=1; sub.pColorAttachments=&ref;
    VkSubpassDependency deps[2]{};
    deps[0].srcSubpass=VK_SUBPASS_EXTERNAL; deps[0].dstSubpass=0;
    deps[0].srcStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; deps[0].dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].srcAccessMask=VK_ACCESS_SHADER_READ_BIT; deps[0].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].srcSubpass=0; deps[1].dstSubpass=VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; deps[1].dstStageMask=VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; deps[1].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    VkRenderPassCreateInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    ri.attachmentCount=1; ri.pAttachments=&a; ri.subpassCount=1; ri.pSubpasses=&sub; ri.dependencyCount=2; ri.pDependencies=deps;
    check(CreateRenderPass(device,&ri,nullptr,&pass));
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter=si.minFilter=VK_FILTER_LINEAR; si.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU=si.addressModeV=si.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    check(CreateSampler(device,&si,nullptr,&sampler));
    VkDescriptorSetLayoutBinding bindings[2]{};
    for(int i=0;i<2;++i) { bindings[i].binding=i; bindings[i].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; bindings[i].descriptorCount=1; bindings[i].stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT; }
    VkDescriptorSetLayoutCreateInfo di{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; di.bindingCount=2; di.pBindings=bindings;
    check(CreateDescriptorSetLayout(device,&di,nullptr,&setLayout));
    VkPushConstantRange range{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(Params)};
    VkPipelineLayoutCreateInfo li{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; li.setLayoutCount=1; li.pSetLayouts=&setLayout; li.pushConstantRangeCount=1; li.pPushConstantRanges=&range;
    check(CreatePipelineLayout(device,&li,nullptr,&layout));
    createPipeline(pass,pipeline);
}

void BlurBarsVulkan::createPipeline(VkRenderPass renderPass,VkPipeline& result) {
    VkShaderModule vs{},fs{};
    try {
        VkShaderModuleCreateInfo mi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        mi.codeSize=sizeof(blur_bars_vert_code); mi.pCode=blur_bars_vert_code; check(CreateShaderModule(device,&mi,nullptr,&vs));
        mi.codeSize=sizeof(blur_bars_frag_code); mi.pCode=blur_bars_frag_code; check(CreateShaderModule(device,&mi,nullptr,&fs));
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(int i=0;i<2;++i) { stages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT; stages[i].module=i?fs:vs; stages[i].pName="main"; }
        VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO}; ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; vp.viewportCount=vp.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO}; rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_NONE; rs.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState ba{}; ba.colorWriteMask=0xf;
        VkPipelineColorBlendStateCreateInfo bs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO}; bs.attachmentCount=1; bs.pAttachments=&ba;
        VkDynamicState dynamic[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}; ds.dynamicStateCount=2; ds.pDynamicStates=dynamic;
        VkGraphicsPipelineCreateInfo pi{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia; pi.pViewportState=&vp;
        pi.pRasterizationState=&rs; pi.pMultisampleState=&ms; pi.pColorBlendState=&bs; pi.pDynamicState=&ds; pi.layout=layout; pi.renderPass=renderPass;
        check(CreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,nullptr,&result));
    } catch (...) { if(fs) DestroyShaderModule(device,fs,nullptr); if(vs) DestroyShaderModule(device,vs,nullptr); throw; }
    DestroyShaderModule(device,fs,nullptr); DestroyShaderModule(device,vs,nullptr);
}

void BlurBarsVulkan::target(Target& t,uint32_t w,uint32_t h) {
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.imageType=VK_IMAGE_TYPE_2D; ii.format=format; ii.extent={w,h,1}; ii.mipLevels=ii.arrayLayers=1; ii.samples=VK_SAMPLE_COUNT_1_BIT;
    ii.tiling=VK_IMAGE_TILING_OPTIMAL; ii.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    check(CreateImage(device,&ii,nullptr,&t.image));
    VkMemoryRequirements req{}; GetImageMemoryRequirements(device,t.image,&req);
    uint32_t type=0; for(;type<memory.memoryTypeCount;++type) if((req.memoryTypeBits&(1u<<type)) && (memory.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) break;
    if(type==memory.memoryTypeCount) throw std::runtime_error("Blur bars: no image memory");
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ai.allocationSize=req.size; ai.memoryTypeIndex=type;
    check(AllocateMemory(device,&ai,nullptr,&t.memory)); check(BindImageMemory(device,t.image,t.memory,0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; vi.image=t.image; vi.viewType=VK_IMAGE_VIEW_TYPE_2D; vi.format=format; vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    check(CreateImageView(device,&vi,nullptr,&t.view));
    VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; fi.renderPass=pass; fi.attachmentCount=1; fi.pAttachments=&t.view; fi.width=w; fi.height=h; fi.layers=1;
    check(CreateFramebuffer(device,&fi,nullptr,&t.fb));
}

void BlurBarsVulkan::resize(uint32_t w,uint32_t h) {
    if(w==width && h==height) return;
    check(DeviceWaitIdle(device)); // Only on allocation/resize; protects every frame slot.
    destroyTargets();
    smallW=std::max(1u,(w+3)/4); smallH=std::max(1u,(h+3)/4);
    try {
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,12};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; pi.maxSets=6; pi.poolSizeCount=1; pi.pPoolSizes=&size;
        check(CreateDescriptorPool(device,&pi,nullptr,&pool));
        for(auto& f:frames) {
            target(f.targets[0],w,h); target(f.targets[1],smallW,smallH); target(f.targets[2],smallW,smallH);
            VkDescriptorSetLayout layouts[]={setLayout,setLayout,setLayout};
            VkDescriptorSetAllocateInfo ai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; ai.descriptorPool=pool; ai.descriptorSetCount=3; ai.pSetLayouts=layouts;
            check(AllocateDescriptorSets(device,&ai,f.sets.data()));
            for(int i=0;i<3;++i) {
                VkDescriptorImageInfo images[2]={{sampler,f.targets[i==1?1:0].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},{sampler,f.targets[i==2?2:(i==1?1:0)].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
                VkWriteDescriptorSet writes[2]{};
                for(int j=0;j<2;++j) { writes[j].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; writes[j].dstSet=f.sets[i]; writes[j].dstBinding=j; writes[j].descriptorCount=1; writes[j].descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; writes[j].pImageInfo=&images[j]; }
                UpdateDescriptorSets(device,2,writes,0,nullptr);
            }
        }
        width=w; height=h;
    } catch (...) { destroyTargets(); throw; }
}

void BlurBarsVulkan::draw(VkCommandBuffer cb,VkDescriptorSet set,uint32_t w,uint32_t h,const Params& pc,VkPipeline selectedPipeline) {
    VkViewport vp{0,0,float(w),float(h),0,1}; VkRect2D sc{{0,0},{w,h}};
    CmdSetViewport(cb,0,1,&vp); CmdSetScissor(cb,0,1,&sc);
    CmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,selectedPipeline);
    CmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&set,0,nullptr);
    CmdPushConstants(cb,layout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(pc),&pc);
    CmdDraw(cb,3,1,0,0);
}

void BlurBarsVulkan::blur(VkCommandBuffer cb,uint32_t slot,Rect rect,float radius) {
    auto& f=frames.at(slot);
    // Radius is the outer kernel support in output pixels at 1080p.
    float r=std::clamp(radius,0.f,120.f)*float(height)/1080.f/4.f;
    for(int i=0;i<2;++i) {
        VkClearValue clear{};
        VkRenderPassBeginInfo ri{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO}; ri.renderPass=pass; ri.framebuffer=f.targets[i+1].fb; ri.renderArea={{0,0},{smallW,smallH}}; ri.clearValueCount=1; ri.pClearValues=&clear;
        CmdBeginRenderPass(cb,&ri,VK_SUBPASS_CONTENTS_INLINE);
        Params pc{rect,i==0?r/float(width):0.f,i==1?r/float(height):0.f,1.f,i,0.12f};
        draw(cb,f.sets[i],smallW,smallH,pc,pipeline);
        CmdEndRenderPass(cb);
    }
}

void BlurBarsVulkan::composite(VkCommandBuffer cb,uint32_t slot,Rect rect,float edgeWidth,float brightness,VkRenderPass finalPass) {
    Params pc{rect,0,0,std::clamp(brightness,0.f,1.f),2,std::clamp(edgeWidth,0.01f,0.5f)};
    auto& finalPipeline=finalPipelines[finalPass];
    if(!finalPipeline) createPipeline(finalPass,finalPipeline);
    draw(cb,frames.at(slot).sets[2],width,height,pc,finalPipeline);
}

void BlurBarsVulkan::destroyTargets() {
    if(pool) { DestroyDescriptorPool(device,pool,nullptr); pool=VK_NULL_HANDLE; }
    for(auto& f:frames) for(auto& t:f.targets) {
        if(t.fb) DestroyFramebuffer(device,t.fb,nullptr);
        if(t.view) DestroyImageView(device,t.view,nullptr);
        if(t.image) DestroyImage(device,t.image,nullptr);
        if(t.memory) FreeMemory(device,t.memory,nullptr);
        t={};
    }
    width=height=0;
}

BlurBarsVulkan::~BlurBarsVulkan() {
    destroyTargets();
    for(auto& entry:finalPipelines) if(entry.second) DestroyPipeline(device,entry.second,nullptr);
    DestroyPipeline(device,pipeline,nullptr); DestroyPipelineLayout(device,layout,nullptr);
    DestroyDescriptorSetLayout(device,setLayout,nullptr); DestroySampler(device,sampler,nullptr); DestroyRenderPass(device,pass,nullptr);
}
