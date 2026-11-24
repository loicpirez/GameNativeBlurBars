// Runs the production Vulkan shaders/compositor, without a window or input.
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "BlurBarsVulkan.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#define FUNCS(X) X(CreateInstance) X(DestroyInstance) X(EnumeratePhysicalDevices) X(GetPhysicalDeviceProperties) X(GetPhysicalDeviceMemoryProperties) X(GetPhysicalDeviceQueueFamilyProperties) X(CreateDevice) X(DestroyDevice) X(GetDeviceQueue) X(GetDeviceProcAddr) X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) X(ResetCommandBuffer) X(BeginCommandBuffer) X(EndCommandBuffer) X(QueueSubmit) X(QueueWaitIdle) X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(AllocateMemory) X(FreeMemory) X(BindImageMemory) X(CreateImageView) X(DestroyImageView) X(CreateFramebuffer) X(DestroyFramebuffer) X(CreateRenderPass) X(DestroyRenderPass) X(CmdBeginRenderPass) X(CmdEndRenderPass) X(CmdClearAttachments) X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(CmdCopyImageToBuffer)
#define DECLARE(n) PFN_vk##n n;
FUNCS(DECLARE)
void check(VkResult r) { if(r!=VK_SUCCESS) throw std::runtime_error("Vulkan error "+std::to_string(r)); }
void require(bool b,const char* msg) { if(!b) throw std::runtime_error(msg); }
struct Pixel { uint8_t r,g,b,a; bool operator==(const Pixel& p) const { return r==p.r&&g==p.g&&b==p.b&&a==p.a; } };
VkDevice device; VkPhysicalDeviceMemoryProperties mem;
int validationErrors=0;
VKAPI_ATTR VkBool32 VKAPI_CALL validation(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT,const VkDebugUtilsMessengerCallbackDataEXT* message,void*) {
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) { ++validationErrors; std::cerr<<"Validation ERROR: "<<message->pMessage<<"\n"; }
    return VK_FALSE;
}
uint32_t memoryType(uint32_t bits,VkMemoryPropertyFlags flags) { for(uint32_t i=0;i<mem.memoryTypeCount;++i) if((bits&(1<<i))&&(mem.memoryTypes[i].propertyFlags&flags)==flags) return i; throw std::runtime_error("No memory type"); }

int main() try {
#ifdef _WIN32
    auto lib=LoadLibraryA("vulkan-1.dll");
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(lib,"vkGetInstanceProcAddr"));
#else
    auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(dlopen("libvulkan.so.1",RTLD_NOW),"vkGetInstanceProcAddr"));
#endif
    require(get!=nullptr,"Vulkan loader missing");
    CreateInstance=reinterpret_cast<PFN_vkCreateInstance>(get(nullptr,"vkCreateInstance"));
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo ii{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ii.pApplicationInfo=&app;
    const char* layer="VK_LAYER_KHRONOS_validation";
    const char* extensions[]={VK_EXT_DEBUG_UTILS_EXTENSION_NAME,VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    ii.enabledLayerCount=1; ii.ppEnabledLayerNames=&layer; ii.enabledExtensionCount=2; ii.ppEnabledExtensionNames=extensions;
    VkValidationFeatureEnableEXT sync=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    VkValidationFeaturesEXT vf{VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT}; vf.enabledValidationFeatureCount=1; vf.pEnabledValidationFeatures=&sync; ii.pNext=&vf;
    VkInstance instance; check(CreateInstance(&ii,nullptr,&instance));
    auto createMessenger=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(get(instance,"vkCreateDebugUtilsMessengerEXT"));
    auto destroyMessenger=reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(get(instance,"vkDestroyDebugUtilsMessengerEXT"));
    VkDebugUtilsMessengerCreateInfoEXT debug{VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT}; debug.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; debug.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; debug.pfnUserCallback=validation;
    VkDebugUtilsMessengerEXT messenger; check(createMessenger(instance,&debug,nullptr,&messenger));
#define LOAD(n) n=reinterpret_cast<PFN_vk##n>(get(instance,"vk" #n));
    FUNCS(LOAD)
    uint32_t count=0; check(EnumeratePhysicalDevices(instance,&count,nullptr)); require(count>0,"No Vulkan physical device");
    std::vector<VkPhysicalDevice> physical(count); check(EnumeratePhysicalDevices(instance,&count,physical.data()));
    VkPhysicalDevice gpu=physical[0]; VkPhysicalDeviceProperties props{}; GetPhysicalDeviceProperties(gpu,&props);
    std::cout<<"Vulkan device: "<<props.deviceName<<"\n";
    GetPhysicalDeviceMemoryProperties(gpu,&mem);
    GetPhysicalDeviceQueueFamilyProperties(gpu,&count,nullptr); std::vector<VkQueueFamilyProperties> families(count); GetPhysicalDeviceQueueFamilyProperties(gpu,&count,families.data());
    uint32_t family=0; while(family<count && !(families[family].queueFlags&VK_QUEUE_GRAPHICS_BIT)) ++family; require(family<count,"No graphics queue");
    float priority=0.1f; VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; qi.queueFamilyIndex=family; qi.queueCount=1; qi.pQueuePriorities=&priority;
    VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; di.queueCreateInfoCount=1; di.pQueueCreateInfos=&qi; check(CreateDevice(gpu,&di,nullptr,&device));
    VkQueue queue; GetDeviceQueue(device,family,0,&queue);
    VkCommandPoolCreateInfo pi{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; pi.queueFamilyIndex=family; pi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool pool; check(CreateCommandPool(device,&pi,nullptr,&pool));
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; cai.commandPool=pool; cai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount=1;
    VkCommandBuffer cb; check(AllocateCommandBuffers(device,&cai,&cb));
    {
        BlurBarsVulkan bars(device,GetDeviceProcAddr,mem,VK_FORMAT_R8G8B8A8_UNORM);
        auto normal=BlurBarsVulkan::sceneRect(100,0,.75f,1,800,600,800,600);
        auto virtualDesktop=BlurBarsVulkan::sceneRect(160,0,1.2f,1.6f,1280,960,800,600);
        require(normal.left==.125f && normal.right==.875f && normal.top==0 && normal.bottom==1,"4:3 geometry");
        require(virtualDesktop.left==normal.left && virtualDesktop.right==normal.right && virtualDesktop.bottom==normal.bottom,"Effective desktop geometry regression");
        std::cout<<"PASS effective desktop smaller than X server: same useful rectangle\n";
        struct Case { int w,h,x,y,cw,ch; float radius,width,brightness; };
        std::vector<Case> cases={
            {160,90,20,0,120,90,48,.12f,.75f}, {160,90,20,0,120,90,0,.12f,.75f},
            {160,90,20,0,120,90,120,.12f,.75f}, {160,90,20,0,120,90,48,.5f,.75f},
            {160,90,20,0,120,90,48,.01f,.75f}, {160,90,20,0,120,90,48,.12f,0},
            {160,90,20,0,120,90,48,.12f,1}, {120,120,0,15,120,90,48,.12f,.75f},
            {160,100,7,13,120,80,48,.12f,.75f}, {161,91,20,0,121,91,48,.12f,.75f},
            {120,90,0,0,120,90,48,.12f,.75f}, {122,90,1,0,120,90,48,.12f,.75f},
            {1920,1080,240,0,1440,1080,48,.12f,.75f},
            {3440,1440,760,0,1920,1440,48,.12f,.75f}};
        std::vector<std::vector<Pixel>> outputs;
        int slot=0;
        for(const auto& c:cases) {
            bars.resize(c.w,c.h);
            VkImage image; VkDeviceMemory memory; VkImageView view; VkFramebuffer fb;
            VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; ci.imageType=VK_IMAGE_TYPE_2D; ci.format=VK_FORMAT_R8G8B8A8_UNORM; ci.extent={uint32_t(c.w),uint32_t(c.h),1}; ci.mipLevels=ci.arrayLayers=1; ci.samples=VK_SAMPLE_COUNT_1_BIT; ci.tiling=VK_IMAGE_TILING_OPTIMAL; ci.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            check(CreateImage(device,&ci,nullptr,&image)); VkMemoryRequirements req; GetImageMemoryRequirements(device,image,&req);
            VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; ma.allocationSize=req.size; ma.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); check(AllocateMemory(device,&ma,nullptr,&memory)); check(BindImageMemory(device,image,memory,0));
            VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO}; vi.image=image; vi.viewType=VK_IMAGE_VIEW_TYPE_2D; vi.format=ci.format; vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}; check(CreateImageView(device,&vi,nullptr,&view));
            VkAttachmentDescription ad{}; ad.format=ci.format; ad.samples=VK_SAMPLE_COUNT_1_BIT; ad.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; ad.storeOp=VK_ATTACHMENT_STORE_OP_STORE; ad.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; ad.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE; ad.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            VkAttachmentReference ar{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; VkSubpassDescription sp{}; sp.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; sp.colorAttachmentCount=1; sp.pColorAttachments=&ar;
            VkSubpassDependency dep{}; dep.srcSubpass=0; dep.dstSubpass=VK_SUBPASS_EXTERNAL; dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT; dep.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            VkRenderPassCreateInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO}; rp.attachmentCount=1; rp.pAttachments=&ad; rp.subpassCount=1; rp.pSubpasses=&sp; rp.dependencyCount=1; rp.pDependencies=&dep; VkRenderPass pass; check(CreateRenderPass(device,&rp,nullptr,&pass));
            VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO}; fi.renderPass=pass; fi.attachmentCount=1; fi.pAttachments=&view; fi.width=c.w; fi.height=c.h; fi.layers=1; check(CreateFramebuffer(device,&fi,nullptr,&fb));
            VkBuffer buffer; VkDeviceMemory bufferMemory;
            VkBufferCreateInfo bi{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; bi.size=c.w*c.h*4; bi.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT; check(CreateBuffer(device,&bi,nullptr,&buffer)); GetBufferMemoryRequirements(device,buffer,&req);
            ma.allocationSize=req.size; ma.memoryTypeIndex=memoryType(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); check(AllocateMemory(device,&ma,nullptr,&bufferMemory)); check(BindBufferMemory(device,buffer,bufferMemory,0));
            check(ResetCommandBuffer(cb,0)); VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; check(BeginCommandBuffer(cb,&begin));
            VkClearValue black{{{0,0,0,1}}}; VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO}; rbi.renderPass=bars.renderPass(); rbi.framebuffer=bars.sceneFramebuffer(slot); rbi.renderArea={{0,0},{uint32_t(c.w),uint32_t(c.h)}}; rbi.clearValueCount=1; rbi.pClearValues=&black;
            CmdBeginRenderPass(cb,&rbi,VK_SUBPASS_CONTENTS_INLINE);
            std::vector<Pixel> reference(c.w*c.h,{0,0,0,255});
            for(int x=0;x<c.cw;++x) {
                Pixel pixel{uint8_t(20+(x/4*23)%210),uint8_t(40+(x/7*19)%170),uint8_t(220-(x/5*17)%190),255};
                VkClearAttachment ca{}; ca.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
                ca.clearValue.color={{pixel.r/255.f,pixel.g/255.f,pixel.b/255.f,1}};
                VkClearRect cr{{{c.x+x,c.y},{1,uint32_t(c.ch)}},0,1}; CmdClearAttachments(cb,1,&ca,1,&cr);
                for(int y=c.y;y<c.y+c.ch;++y) reference[y*c.w+c.x+x]=pixel;
            }
            CmdEndRenderPass(cb);
            BlurBarsVulkan::Rect rect{float(c.x)/c.w,float(c.y)/c.h,float(c.x+c.cw)/c.w,float(c.y+c.ch)/c.h};
            bars.blur(cb,slot,rect,c.radius);
            rbi.renderPass=pass; rbi.framebuffer=fb; CmdBeginRenderPass(cb,&rbi,VK_SUBPASS_CONTENTS_INLINE);
            bars.composite(cb,slot,rect,c.width,c.brightness,pass); CmdEndRenderPass(cb);
            VkBufferImageCopy copy{}; copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}; copy.imageExtent={uint32_t(c.w),uint32_t(c.h),1}; CmdCopyImageToBuffer(cb,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
            check(EndCommandBuffer(cb)); VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&cb; check(QueueSubmit(queue,1,&submit,VK_NULL_HANDLE)); check(QueueWaitIdle(queue));
            void* data; check(MapMemory(device,bufferMemory,0,VK_WHOLE_SIZE,0,&data));
            std::vector<Pixel> result((Pixel*)data,(Pixel*)data+c.w*c.h); UnmapMemory(device,bufferMemory);
            int margins=0;
            for(int y=0;y<c.h;++y) for(int x=0;x<c.w;++x) {
                auto p=result[y*c.w+x]; require(p.a==255,"Alpha regression");
                if(x>=c.x&&x<c.x+c.cw&&y>=c.y&&y<c.y+c.ch) require(p==reference[y*c.w+x],"Centre pixel changed");
                else { ++margins; if(c.brightness==0) require(p.r+p.g+p.b==0,"Brightness zero failed"); else require(p.r+p.g+p.b>0,"Black bleed in margin"); }
            }
            if(outputs.empty()) {
                std::ofstream out("blur-bars-result.ppm",std::ios::binary); out<<"P6\n"<<c.w<<" "<<c.h<<"\n255\n"; for(auto p:result) { out.put(p.r);out.put(p.g);out.put(p.b); }
            }
            outputs.push_back(result);
            DestroyBuffer(device,buffer,nullptr); FreeMemory(device,bufferMemory,nullptr);
            DestroyFramebuffer(device,fb,nullptr); DestroyRenderPass(device,pass,nullptr); DestroyImageView(device,view,nullptr); DestroyImage(device,image,nullptr); FreeMemory(device,memory,nullptr);
            std::cout<<"PASS case "<<outputs.size()<<": exact centre, "<<margins<<" margin pixels\n";
            slot=1-slot;
        }
        require(outputs[0]!=outputs[1]&&outputs[0]!=outputs[2],"Radius has no visible effect");
        require(outputs[0]!=outputs[3]&&outputs[3]!=outputs[4],"Edge width has no visible effect");
        require(outputs[0]!=outputs[5]&&outputs[0]!=outputs[6],"Brightness has no visible effect");
        std::cout<<"PASS independent radius, width, brightness and resize/frame-slot regression\n";
    }
    DestroyCommandPool(device,pool,nullptr); DestroyDevice(device,nullptr);
    destroyMessenger(instance,messenger,nullptr); DestroyInstance(instance,nullptr);
    require(validationErrors==0,"Vulkan validation/synchronization errors");
    std::cout<<"PASS Khronos Vulkan validation and synchronization: zero errors\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL "<<e.what()<<"\n"; return 1; }
