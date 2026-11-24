#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <map>

// Independent of Android surfaces: the same compositor is exercised by the
// desktop headless Vulkan regression harness. Call only on the render thread,
// after waiting for the frame slot's fence. Destruction requires a device idle.
class BlurBarsVulkan {
public:
    struct Rect { float left, top, right, bottom; };
    static Rect sceneRect(float ox,float oy,float sx,float sy,float containerW,float containerH,float contentW,float contentH);
    BlurBarsVulkan(VkDevice device, PFN_vkGetDeviceProcAddr getProc,
                   const VkPhysicalDeviceMemoryProperties& memory, VkFormat format);
    ~BlurBarsVulkan();
    BlurBarsVulkan(const BlurBarsVulkan&) = delete;
    BlurBarsVulkan& operator=(const BlurBarsVulkan&) = delete;
    void resize(uint32_t width, uint32_t height);
    VkRenderPass renderPass() const { return pass; }
    VkFramebuffer sceneFramebuffer(uint32_t slot) const { return frames.at(slot).targets[0].fb; }
    void blur(VkCommandBuffer cb, uint32_t slot, Rect rect, float radius);
    // Called inside the final target's compatible render pass, before cursor.
    void composite(VkCommandBuffer cb, uint32_t slot, Rect rect, float width, float brightness, VkRenderPass finalPass);
private:
#define BB_FUNCTIONS(X) \
    X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(AllocateMemory) X(FreeMemory) \
    X(BindImageMemory) X(CreateImageView) X(DestroyImageView) X(CreateFramebuffer) X(DestroyFramebuffer) \
    X(CreateRenderPass) X(DestroyRenderPass) X(CreateSampler) X(DestroySampler) \
    X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) X(CreateDescriptorPool) X(DestroyDescriptorPool) \
    X(AllocateDescriptorSets) X(UpdateDescriptorSets) X(CreatePipelineLayout) X(DestroyPipelineLayout) \
    X(CreateShaderModule) X(DestroyShaderModule) X(CreateGraphicsPipelines) X(DestroyPipeline) \
    X(CmdBeginRenderPass) X(CmdEndRenderPass) X(CmdSetViewport) X(CmdSetScissor) \
    X(CmdBindPipeline) X(CmdBindDescriptorSets) X(CmdPushConstants) X(CmdDraw) X(DeviceWaitIdle)
#define BB_DECLARE(name) PFN_vk##name name = nullptr;
    BB_FUNCTIONS(BB_DECLARE)
#undef BB_DECLARE
    struct Target { VkImage image{}; VkDeviceMemory memory{}; VkImageView view{}; VkFramebuffer fb{}; };
    struct Frame { std::array<Target,3> targets{}; std::array<VkDescriptorSet,3> sets{}; };
    struct Params { Rect rect; float stepX, stepY, brightness; int mode; float edgeWidth; };
    VkDevice device;
    VkPhysicalDeviceMemoryProperties memory;
    VkFormat format;
    VkRenderPass pass{};
    VkSampler sampler{};
    VkDescriptorSetLayout setLayout{};
    VkDescriptorPool pool{};
    VkPipelineLayout layout{};
    VkPipeline pipeline{};
    std::map<VkRenderPass,VkPipeline> finalPipelines;
    uint32_t width{}, height{}, smallW{}, smallH{};
    std::array<Frame,2> frames{};
    void create();
    void createPipeline(VkRenderPass renderPass, VkPipeline& result);
    void destroyTargets();
    void target(Target& t, uint32_t w, uint32_t h);
    void draw(VkCommandBuffer cb, VkDescriptorSet set, uint32_t w, uint32_t h, const Params& pc, VkPipeline selectedPipeline);
};
