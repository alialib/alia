#include <alia/platforms/glfw/vk_present.h>

#include <alia/abi/prelude.h>

#include "vk_gl_ext_win32.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#ifndef VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME
#define VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME                           \
    "VK_KHR_external_memory_win32"
#endif
#ifndef VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
#define VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME                        \
    "VK_KHR_external_semaphore_win32"
#endif

#ifndef VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR
#define VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR                    \
    VkStructureType(1000073002)
#define VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR                 \
    VkStructureType(1000073004)
typedef struct VkMemoryGetWin32HandleInfoKHR
{
    VkStructureType sType;
    void const* pNext;
    VkDeviceMemory memory;
    VkExternalMemoryHandleTypeFlagBits handleType;
} VkMemoryGetWin32HandleInfoKHR;
typedef struct VkSemaphoreGetWin32HandleInfoKHR
{
    VkStructureType sType;
    void const* pNext;
    VkSemaphore semaphore;
    VkExternalSemaphoreHandleTypeFlagBits handleType;
} VkSemaphoreGetWin32HandleInfoKHR;
typedef VkResult(VKAPI_PTR* PFN_vkGetMemoryWin32HandleKHR)(
    VkDevice, VkMemoryGetWin32HandleInfoKHR const*, HANDLE*);
typedef VkResult(VKAPI_PTR* PFN_vkGetSemaphoreWin32HandleKHR)(
    VkDevice, VkSemaphoreGetWin32HandleInfoKHR const*, HANDLE*);
#endif

struct alia_glfw_vk_present
{
    GLFWwindow* window = nullptr;
    bool vsync = true;
    bool modal_interaction = false;
    bool gpu_interop = false;
    bool gpu_interop_semaphore = false;
    bool vulkan_interop_available = false;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;
    uint32_t graphics_family = 0;
    uint32_t present_family = 0;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D swapchain_extent{};

    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;

    VkSemaphore image_available = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    VkFence in_flight_fence = VK_NULL_HANDLE;

    // Tier A fallback.
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    void* staging_mapped = nullptr;
    VkDeviceSize staging_size = 0;

    // Tier B GL/Vulkan interop.
    alia_vk_gl_ext_win32 gl_ext{};
    PFN_vkGetMemoryWin32HandleKHR get_memory_win32_handle = nullptr;
    PFN_vkGetSemaphoreWin32HandleKHR get_semaphore_win32_handle = nullptr;

    VkImage interop_image = VK_NULL_HANDLE;
    VkDeviceMemory interop_memory = VK_NULL_HANDLE;
    VkImageView interop_view = VK_NULL_HANDLE;
    VkSemaphore interop_vk_sem = VK_NULL_HANDLE;

    GLuint interop_gl_memory = 0;
    GLuint interop_gl_texture = 0;
    GLuint interop_gl_fbo = 0;
    GLuint interop_gl_sem = 0;

    int width = 0;
    int height = 0;
};

namespace {

bool
vk_create_interop(alia_glfw_vk_present* p, int width, int height);

bool
vk_check(VkResult result, char const* what)
{
    if (result == VK_SUCCESS)
        return true;
    std::fprintf(
        stderr, "[alia vk_present] %s failed (%d)\n", what, int(result));
    return false;
}

bool
vk_has_device_extension(VkPhysicalDevice device, char const* name)
{
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> props(count);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &count, props.data());
    for (VkExtensionProperties const& prop : props)
    {
        if (std::strcmp(prop.extensionName, name) == 0)
            return true;
    }
    return false;
}

bool
vk_find_queue_families(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    uint32_t* out_graphics,
    uint32_t* out_present)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    uint32_t graphics = UINT32_MAX;
    uint32_t present = UINT32_MAX;
    for (uint32_t i = 0; i < count; ++i)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            graphics = i;

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            device, i, surface, &present_support);
        if (present_support == VK_TRUE)
            present = i;
    }

    if (graphics == UINT32_MAX || present == UINT32_MAX)
        return false;
    *out_graphics = graphics;
    *out_present = present;
    return true;
}

VkPresentModeKHR
vk_choose_present_mode(
    VkPhysicalDevice device, VkSurfaceKHR surface, bool vsync)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &count, modes.data());

    if (!vsync)
    {
        for (VkPresentModeKHR mode : modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }
        for (VkPresentModeKHR mode : modes)
        {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
                return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

bool
vk_effective_vsync(alia_glfw_vk_present const* p)
{
    return p && p->vsync && !p->modal_interaction;
}

void
vk_destroy_swapchain(alia_glfw_vk_present* p)
{
    if (!p || !p->device)
        return;
    for (VkImageView view : p->swapchain_image_views)
        vkDestroyImageView(p->device, view, nullptr);
    p->swapchain_image_views.clear();
    p->swapchain_images.clear();
    if (p->swapchain)
        vkDestroySwapchainKHR(p->device, p->swapchain, nullptr);
    p->swapchain = VK_NULL_HANDLE;
}

void
vk_destroy_interop(alia_glfw_vk_present* p)
{
    if (!p)
        return;

    if (p->interop_gl_sem && p->gl_ext.DeleteSemaphoresEXT)
        p->gl_ext.DeleteSemaphoresEXT(1, &p->interop_gl_sem);
    if (p->interop_gl_fbo)
        glDeleteFramebuffers(1, &p->interop_gl_fbo);
    if (p->interop_gl_texture)
        glDeleteTextures(1, &p->interop_gl_texture);
    if (p->interop_gl_memory && p->gl_ext.DeleteMemoryObjectsEXT)
        p->gl_ext.DeleteMemoryObjectsEXT(1, &p->interop_gl_memory);

    p->interop_gl_sem = 0;
    p->interop_gl_fbo = 0;
    p->interop_gl_texture = 0;
    p->interop_gl_memory = 0;

    if (p->device)
    {
        if (p->interop_view)
            vkDestroyImageView(p->device, p->interop_view, nullptr);
        if (p->interop_image)
            vkDestroyImage(p->device, p->interop_image, nullptr);
        if (p->interop_memory)
            vkFreeMemory(p->device, p->interop_memory, nullptr);
        if (p->interop_vk_sem)
            vkDestroySemaphore(p->device, p->interop_vk_sem, nullptr);
    }

    p->interop_view = VK_NULL_HANDLE;
    p->interop_image = VK_NULL_HANDLE;
    p->interop_memory = VK_NULL_HANDLE;
    p->interop_vk_sem = VK_NULL_HANDLE;
}

bool
vk_create_swapchain(alia_glfw_vk_present* p, int width, int height)
{
    vk_destroy_swapchain(p);

    VkSurfaceCapabilitiesKHR caps{};
    if (!vk_check(
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                p->physical_device, p->surface, &caps),
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
        return false;

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        p->physical_device, p->surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        p->physical_device, p->surface, &format_count, formats.data());

    VkSurfaceFormatKHR chosen
        = formats.empty()
            ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
            : formats[0];
    for (VkSurfaceFormatKHR const& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB
            && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosen = f;
            break;
        }
    }

    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX)
    {
        extent.width = uint32_t(width);
        extent.height = uint32_t(height);
        extent.width = (std::max) (caps.minImageExtent.width, extent.width);
        extent.width = (std::min) (caps.maxImageExtent.width, extent.width);
        extent.height = (std::max) (caps.minImageExtent.height, extent.height);
        extent.height = (std::min) (caps.maxImageExtent.height, extent.height);
    }

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
        image_count = caps.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = p->surface,
        .minImageCount = image_count,
        .imageFormat = chosen.format,
        .imageColorSpace = chosen.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vk_choose_present_mode(
            p->physical_device, p->surface, vk_effective_vsync(p)),
        .clipped = VK_TRUE,
    };

    if (!vk_check(
            vkCreateSwapchainKHR(
                p->device, &create_info, nullptr, &p->swapchain),
            "vkCreateSwapchainKHR"))
        return false;

    uint32_t sc_image_count = 0;
    vkGetSwapchainImagesKHR(p->device, p->swapchain, &sc_image_count, nullptr);
    p->swapchain_images.resize(sc_image_count);
    vkGetSwapchainImagesKHR(
        p->device, p->swapchain, &sc_image_count, p->swapchain_images.data());

    p->swapchain_format = chosen.format;
    p->swapchain_extent = extent;
    p->width = int(extent.width);
    p->height = int(extent.height);

    p->swapchain_image_views.resize(sc_image_count);
    for (uint32_t i = 0; i < sc_image_count; ++i)
    {
        VkImageViewCreateInfo view_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = p->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = p->swapchain_format,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        };
        if (!vk_check(
                vkCreateImageView(
                    p->device,
                    &view_info,
                    nullptr,
                    &p->swapchain_image_views[i]),
                "vkCreateImageView"))
            return false;
    }
    return true;
}

bool
vk_create_gl_target(alia_glfw_vk_present* p, int width, int height)
{
    vk_destroy_interop(p);
    if (width <= 0 || height <= 0)
        return false;

    if (p->gpu_interop)
        return vk_create_interop(p, width, height);

    glGenTextures(1, &p->interop_gl_texture);
    glBindTexture(GL_TEXTURE_2D, p->interop_gl_texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_SRGB8_ALPHA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &p->interop_gl_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, p->interop_gl_fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        p->interop_gl_texture,
        0);
    bool const complete
        = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete)
        return false;

    p->width = width;
    p->height = height;
    return true;
}

bool
vk_create_interop(alia_glfw_vk_present* p, int width, int height)
{
    if (!p->gpu_interop)
        return vk_create_gl_target(p, width, height);

    VkExternalMemoryImageCreateInfo external_image{
        .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
    };
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = &external_image,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = {uint32_t(width), uint32_t(height), 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
               | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!vk_check(
            vkCreateImage(p->device, &image_info, nullptr, &p->interop_image),
            "vkCreateImage interop"))
        return false;

    VkMemoryRequirements mem_req{};
    vkGetImageMemoryRequirements(p->device, p->interop_image, &mem_req);

    VkExportMemoryAllocateInfo export_alloc{
        .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
    };
    VkMemoryDedicatedAllocateInfo dedicated_alloc{
        .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
        .image = p->interop_image,
    };
    dedicated_alloc.pNext = &export_alloc;

    uint32_t mem_type = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties mem_props{};
    vkGetPhysicalDeviceMemoryProperties(p->physical_device, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        if ((mem_req.memoryTypeBits & (1u << i))
            && (mem_props.memoryTypes[i].propertyFlags
                & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            mem_type = i;
            break;
        }
    }
    if (mem_type == UINT32_MAX)
    {
        for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
        {
            if (mem_req.memoryTypeBits & (1u << i))
            {
                mem_type = i;
                break;
            }
        }
    }
    if (mem_type == UINT32_MAX)
        return false;

    VkMemoryAllocateInfo alloc{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &dedicated_alloc,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = mem_type,
    };
    if (!vk_check(
            vkAllocateMemory(p->device, &alloc, nullptr, &p->interop_memory),
            "vkAllocateMemory interop"))
        return false;
    if (!vk_check(
            vkBindImageMemory(
                p->device, p->interop_image, p->interop_memory, 0),
            "vkBindImageMemory interop"))
        return false;

    VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = p->interop_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    if (!vk_check(
            vkCreateImageView(
                p->device, &view_info, nullptr, &p->interop_view),
            "vkCreateImageView interop"))
        return false;

    if (p->gpu_interop_semaphore)
    {
        VkExportSemaphoreCreateInfo export_sem{
            .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR,
            .handleTypes
            = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
        };
        VkSemaphoreCreateInfo sem_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &export_sem,
        };
        if (!vk_check(
                vkCreateSemaphore(
                    p->device, &sem_info, nullptr, &p->interop_vk_sem),
                "vkCreateSemaphore interop"))
            return false;
    }

    VkMemoryGetWin32HandleInfoKHR memory_handle_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .memory = p->interop_memory,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
    };
    HANDLE memory_handle = nullptr;
    if (!vk_check(
            p->get_memory_win32_handle(
                p->device, &memory_handle_info, &memory_handle),
            "vkGetMemoryWin32HandleKHR"))
        return false;

    HANDLE sem_handle = nullptr;
    if (p->gpu_interop_semaphore)
    {
        VkSemaphoreGetWin32HandleInfoKHR sem_handle_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
            .semaphore = p->interop_vk_sem,
            .handleType
            = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
        };
        if (!vk_check(
                p->get_semaphore_win32_handle(
                    p->device, &sem_handle_info, &sem_handle),
                "vkGetSemaphoreWin32HandleKHR"))
        {
            CloseHandle(memory_handle);
            return false;
        }
    }

    alia_vk_gl_ext_win32& gl = p->gl_ext;
    gl.CreateMemoryObjectsEXT(1, &p->interop_gl_memory);
    gl.ImportMemoryWin32HandleEXT(
        p->interop_gl_memory,
        mem_req.size,
        GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
        memory_handle);
    CloseHandle(memory_handle);

    auto const create_textures = reinterpret_cast<PFNGLCREATETEXTURESPROC>(
        glfwGetProcAddress("glCreateTextures"));
    if (create_textures)
        create_textures(GL_TEXTURE_2D, 1, &p->interop_gl_texture);
    else
        glGenTextures(1, &p->interop_gl_texture);
    gl.TextureStorageMem2DEXT(
        p->interop_gl_texture,
        1,
        GL_RGBA8,
        width,
        height,
        p->interop_gl_memory,
        0);
    glBindTexture(GL_TEXTURE_2D, p->interop_gl_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &p->interop_gl_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, p->interop_gl_fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        p->interop_gl_texture,
        0);

    GLenum const fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::fprintf(
            stderr,
            "[alia vk_present] interop framebuffer incomplete (%u)\n",
            unsigned(fbo_status));
        return false;
    }

    if (p->gpu_interop_semaphore)
    {
        gl.CreateSemaphoresEXT(1, &p->interop_gl_sem);
        gl.ImportSemaphoreWin32HandleEXT(
            p->interop_gl_sem, 0, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, sem_handle);
        CloseHandle(sem_handle);

        GLuint64 const sem_type = GL_SEMAPHORE_TYPE_BINARY_EXT;
        gl.SemaphoreParameterui64vEXT(
            p->interop_gl_sem, GL_SEMAPHORE_TYPE_EXT, &sem_type);
    }

    p->width = width;
    p->height = height;
    return true;
}

bool
vk_ensure_staging(alia_glfw_vk_present* p, VkDeviceSize size)
{
    if (p->staging_buffer && p->staging_size >= size)
        return true;

    if (p->staging_buffer)
    {
        if (p->staging_mapped)
            vkUnmapMemory(p->device, p->staging_memory);
        vkDestroyBuffer(p->device, p->staging_buffer, nullptr);
        vkFreeMemory(p->device, p->staging_memory, nullptr);
        p->staging_buffer = VK_NULL_HANDLE;
        p->staging_memory = VK_NULL_HANDLE;
        p->staging_mapped = nullptr;
        p->staging_size = 0;
    }

    VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    if (!vk_check(
            vkCreateBuffer(
                p->device, &buffer_info, nullptr, &p->staging_buffer),
            "vkCreateBuffer"))
        return false;

    VkMemoryRequirements mem_req{};
    vkGetBufferMemoryRequirements(p->device, p->staging_buffer, &mem_req);

    uint32_t mem_type = UINT32_MAX;
    VkPhysicalDeviceMemoryProperties mem_props{};
    vkGetPhysicalDeviceMemoryProperties(p->physical_device, &mem_props);
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        if ((mem_req.memoryTypeBits & (1u << i))
            && (mem_props.memoryTypes[i].propertyFlags
                & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            && (mem_props.memoryTypes[i].propertyFlags
                & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            mem_type = i;
            break;
        }
    }
    if (mem_type == UINT32_MAX)
        return false;

    VkMemoryAllocateInfo alloc{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = mem_type,
    };
    if (!vk_check(
            vkAllocateMemory(p->device, &alloc, nullptr, &p->staging_memory),
            "vkAllocateMemory"))
        return false;
    if (!vk_check(
            vkBindBufferMemory(
                p->device, p->staging_buffer, p->staging_memory, 0),
            "vkBindBufferMemory"))
        return false;
    if (!vk_check(
            vkMapMemory(
                p->device, p->staging_memory, 0, size, 0, &p->staging_mapped),
            "vkMapMemory"))
        return false;

    p->staging_size = size;
    return true;
}

void
vk_rgba_to_bgra(void* dst, void const* src, size_t byte_count)
{
    auto* d = static_cast<uint8_t*>(dst);
    auto const* s = static_cast<uint8_t const*>(src);
    for (size_t i = 0; i + 3 < byte_count; i += 4)
    {
        d[i + 0] = s[i + 2];
        d[i + 1] = s[i + 1];
        d[i + 2] = s[i + 0];
        d[i + 3] = s[i + 3];
    }
}

bool
vk_present_record_copy(
    alia_glfw_vk_present* present,
    uint32_t image_index,
    VkImage src_image,
    VkImageLayout src_layout)
{
    VkImageMemoryBarrier to_transfer{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = src_layout,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = src_image,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(
        present->command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &to_transfer);

    VkImageMemoryBarrier dst_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = present->swapchain_images[image_index],
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(
        present->command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &dst_barrier);

    VkImageBlit blit{
        .srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        // OpenGL's origin is bottom-left; Vulkan images are top-left.
        .srcOffsets = {{0, present->height, 0}, {present->width, 0, 1}},
        .dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .dstOffsets = {{0, 0, 0}, {present->width, present->height, 1}},
    };
    vkCmdBlitImage(
        present->command_buffer,
        src_image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        present->swapchain_images[image_index],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_LINEAR);

    VkImageMemoryBarrier to_present{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = present->swapchain_images[image_index],
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(
        present->command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &to_present);
    return true;
}

bool
vk_present_submit_and_queue(
    alia_glfw_vk_present* present,
    uint32_t image_index,
    VkSemaphore const* wait_semaphore,
    VkPipelineStageFlags wait_stage)
{
    if (!vk_check(
            vkEndCommandBuffer(present->command_buffer), "vkEndCommandBuffer"))
        return false;

    VkSemaphore wait_sems[2];
    uint32_t wait_count = 0;
    VkPipelineStageFlags wait_stages[2];
    if (wait_semaphore)
    {
        wait_sems[wait_count] = *wait_semaphore;
        wait_stages[wait_count] = wait_stage;
        ++wait_count;
    }
    wait_sems[wait_count] = present->image_available;
    wait_stages[wait_count] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ++wait_count;

    VkSubmitInfo submit{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = wait_count,
        .pWaitSemaphores = wait_sems,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &present->command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &present->render_finished,
    };
    if (!vk_check(
            vkQueueSubmit(
                present->graphics_queue, 1, &submit, present->in_flight_fence),
            "vkQueueSubmit"))
        return false;

    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &present->render_finished,
        .swapchainCount = 1,
        .pSwapchains = &present->swapchain,
        .pImageIndices = &image_index,
    };
    VkResult pr = vkQueuePresentKHR(present->present_queue, &present_info);
    if (pr == VK_ERROR_OUT_OF_DATE_KHR || pr == VK_SUBOPTIMAL_KHR)
        return true;
    return vk_check(pr, "vkQueuePresentKHR");
}

bool
vk_present_begin_frame(alia_glfw_vk_present* present, uint32_t* image_index)
{
    if (!vk_check(
            vkWaitForFences(
                present->device,
                1,
                &present->in_flight_fence,
                VK_TRUE,
                UINT64_MAX),
            "vkWaitForFences"))
        return false;
    if (!vk_check(
            vkResetFences(present->device, 1, &present->in_flight_fence),
            "vkResetFences"))
        return false;

    VkResult acquire = vkAcquireNextImageKHR(
        present->device,
        present->swapchain,
        UINT64_MAX,
        present->image_available,
        VK_NULL_HANDLE,
        image_index);
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
        return false;
    if (!vk_check(acquire, "vkAcquireNextImageKHR"))
        return false;

    if (!vk_check(
            vkResetCommandBuffer(present->command_buffer, 0),
            "vkResetCommandBuffer"))
        return false;

    VkCommandBufferBeginInfo begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    return vk_check(
        vkBeginCommandBuffer(present->command_buffer, &begin),
        "vkBeginCommandBuffer");
}

bool
vk_present_from_gl_interop(alia_glfw_vk_present* present)
{
    if (present->gpu_interop_semaphore)
    {
        present->gl_ext.SignalSemaphoreEXT(
            present->interop_gl_sem,
            0,
            nullptr,
            1,
            &present->interop_gl_texture);
    }
    else
    {
        glFinish();
    }

    uint32_t image_index = 0;
    if (!vk_present_begin_frame(present, &image_index))
        return false;

    vk_present_record_copy(
        present, image_index, present->interop_image, VK_IMAGE_LAYOUT_GENERAL);

    if (present->gpu_interop_semaphore)
    {
        return vk_present_submit_and_queue(
            present,
            image_index,
            &present->interop_vk_sem,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }

    return vk_present_submit_and_queue(
        present, image_index, nullptr, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

bool
vk_present_from_readback(alia_glfw_vk_present* present)
{
    size_t const row_bytes = size_t(present->width) * 4;
    size_t const total = row_bytes * size_t(present->height);
    std::vector<uint8_t> raw(total);
    std::vector<uint8_t> pixels(total);

    glBindFramebuffer(GL_FRAMEBUFFER, present->interop_gl_fbo);
    glReadPixels(
        0,
        0,
        present->width,
        present->height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        raw.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (int y = 0; y < present->height; ++y)
    {
        std::memcpy(
            pixels.data() + y * row_bytes,
            raw.data() + size_t(present->height - 1 - y) * row_bytes,
            row_bytes);
    }

    VkDeviceSize const image_bytes = VkDeviceSize(total);
    if (!vk_ensure_staging(present, image_bytes))
        return false;

    if (present->swapchain_format == VK_FORMAT_B8G8R8A8_SRGB
        || present->swapchain_format == VK_FORMAT_B8G8R8A8_UNORM)
        vk_rgba_to_bgra(present->staging_mapped, pixels.data(), total);
    else
        std::memcpy(present->staging_mapped, pixels.data(), total);

    uint32_t image_index = 0;
    if (!vk_present_begin_frame(present, &image_index))
        return false;

    VkImageMemoryBarrier to_transfer{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = present->swapchain_images[image_index],
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(
        present->command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &to_transfer);

    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent
        = {uint32_t(present->width), uint32_t(present->height), 1},
    };
    vkCmdCopyBufferToImage(
        present->command_buffer,
        present->staging_buffer,
        present->swapchain_images[image_index],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    VkImageMemoryBarrier to_present{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = present->swapchain_images[image_index],
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(
        present->command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &to_present);

    return vk_present_submit_and_queue(
        present, image_index, nullptr, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

} // namespace

extern "C" {

bool
alia_glfw_vk_present_begin(
    alia_glfw_vk_present** out_present, GLFWwindow* window, bool vsync)
{
    ALIA_ASSERT(out_present);
    ALIA_ASSERT(window);

    *out_present = nullptr;
    auto* p = new (std::nothrow) alia_glfw_vk_present{};
    if (!p)
        return false;
    p->window = window;
    p->vsync = vsync;

    uint32_t ext_count = 0;
    char const** extensions = glfwGetRequiredInstanceExtensions(&ext_count);
    if (!extensions || ext_count == 0)
    {
        std::fprintf(
            stderr, "[alia vk_present] GLFW Vulkan extensions unavailable\n");
        delete p;
        return false;
    }

    VkApplicationInfo app_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Alia",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName = "Alia",
        .engineVersion = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo instance_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = ext_count,
        .ppEnabledExtensionNames = extensions,
    };

    if (!vk_check(
            vkCreateInstance(&instance_info, nullptr, &p->instance),
            "vkCreateInstance"))
    {
        delete p;
        return false;
    }

    if (!vk_check(
            glfwCreateWindowSurface(p->instance, window, nullptr, &p->surface),
            "glfwCreateWindowSurface"))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(p->instance, &device_count, nullptr);
    if (device_count == 0)
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(p->instance, &device_count, devices.data());

    p->physical_device = devices[0];
    for (VkPhysicalDevice device : devices)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            p->physical_device = device;
            break;
        }
    }

    if (!vk_find_queue_families(
            p->physical_device,
            p->surface,
            &p->graphics_family,
            &p->present_family))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    p->vulkan_interop_available
        = vk_has_device_extension(
              p->physical_device, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME)
       && vk_has_device_extension(
              p->physical_device, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
       && vk_has_device_extension(
              p->physical_device, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME)
       && vk_has_device_extension(
              p->physical_device,
              VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);

    std::vector<char const*> device_extensions;
    device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (p->vulkan_interop_available)
    {
        device_extensions.push_back(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
        device_extensions.push_back(
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
        device_extensions.push_back(
            VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    }

    float queue_priority = 1.f;
    VkDeviceQueueCreateInfo queue_infos[2]{};
    uint32_t queue_info_count = 0;
    queue_infos[queue_info_count++] = VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = p->graphics_family,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    if (p->present_family != p->graphics_family)
    {
        queue_infos[queue_info_count++] = VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = p->present_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        };
    }

    VkDeviceCreateInfo device_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = queue_info_count,
        .pQueueCreateInfos = queue_infos,
        .enabledExtensionCount = uint32_t(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    };

    if (!vk_check(
            vkCreateDevice(
                p->physical_device, &device_info, nullptr, &p->device),
            "vkCreateDevice"))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    vkGetDeviceQueue(p->device, p->graphics_family, 0, &p->graphics_queue);
    vkGetDeviceQueue(p->device, p->present_family, 0, &p->present_queue);

    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = p->graphics_family,
    };
    if (!vk_check(
            vkCreateCommandPool(
                p->device, &pool_info, nullptr, &p->command_pool),
            "vkCreateCommandPool"))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    VkCommandBufferAllocateInfo cmd_alloc{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = p->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    if (!vk_check(
            vkAllocateCommandBuffers(
                p->device, &cmd_alloc, &p->command_buffer),
            "vkAllocateCommandBuffers"))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    VkSemaphoreCreateInfo sem_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    if (!vk_check(
            vkCreateSemaphore(
                p->device, &sem_info, nullptr, &p->image_available),
            "vkCreateSemaphore image_available")
        || !vk_check(
            vkCreateSemaphore(
                p->device, &sem_info, nullptr, &p->render_finished),
            "vkCreateSemaphore render_finished")
        || !vk_check(
            vkCreateFence(
                p->device, &fence_info, nullptr, &p->in_flight_fence),
            "vkCreateFence"))
    {
        alia_glfw_vk_present_destroy(p);
        return false;
    }

    *out_present = p;
    return true;
}

bool
alia_glfw_vk_present_complete(alia_glfw_vk_present* p)
{
    ALIA_ASSERT(p);
    ALIA_ASSERT(p->window);

    if (p->vulkan_interop_available
        && alia_vk_gl_ext_win32_load_memory(&p->gl_ext))
    {
        p->gpu_interop = true;
        p->gpu_interop_semaphore
            = alia_vk_gl_ext_win32_load_semaphore(&p->gl_ext);
        p->get_memory_win32_handle
            = reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(
                vkGetDeviceProcAddr(p->device, "vkGetMemoryWin32HandleKHR"));
        if (p->gpu_interop_semaphore)
        {
            p->get_semaphore_win32_handle
                = reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(
                    vkGetDeviceProcAddr(
                        p->device, "vkGetSemaphoreWin32HandleKHR"));
            if (!p->get_semaphore_win32_handle)
                p->gpu_interop_semaphore = false;
        }
        if (!p->get_memory_win32_handle)
        {
            std::fprintf(
                stderr,
                "[alia vk_present] Win32 memory interop unavailable; using "
                "readback\n");
            p->gpu_interop = false;
            p->gpu_interop_semaphore = false;
        }
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(p->window, &width, &height);
    if (!vk_create_swapchain(p, width, height))
        return false;
    if (!vk_create_gl_target(p, width, height))
    {
        if (p->gpu_interop)
        {
            std::fprintf(
                stderr,
                "[alia vk_present] GPU interop target setup failed; using "
                "readback\n");
            p->gpu_interop = false;
            p->gpu_interop_semaphore = false;
            if (!vk_create_gl_target(p, width, height))
                return false;
        }
        else
        {
            return false;
        }
    }

    char const* path = "readback";
    if (p->gpu_interop)
        path = p->gpu_interop_semaphore ? "gpu_interop"
                                        : "gpu_interop_glfinish";

    std::fprintf(
        stderr,
        "[alia vk_present] initialized %dx%d (vsync=%s, path=%s)\n",
        p->width,
        p->height,
        p->vsync ? "on" : "off",
        path);
    return true;
}

bool
alia_glfw_vk_present_create(
    alia_glfw_vk_present** out_present, GLFWwindow* window, bool vsync)
{
    alia_glfw_vk_present* present = nullptr;
    if (!alia_glfw_vk_present_begin(&present, window, vsync))
        return false;
    if (!alia_glfw_vk_present_complete(present))
    {
        alia_glfw_vk_present_destroy(present);
        return false;
    }
    *out_present = present;
    return true;
}

void
alia_glfw_vk_present_destroy(alia_glfw_vk_present* p)
{
    if (!p)
        return;

    if (p->device)
        vkDeviceWaitIdle(p->device);

    vk_destroy_interop(p);

    if (p->staging_buffer)
    {
        if (p->staging_mapped)
            vkUnmapMemory(p->device, p->staging_memory);
        vkDestroyBuffer(p->device, p->staging_buffer, nullptr);
        vkFreeMemory(p->device, p->staging_memory, nullptr);
    }

    if (p->in_flight_fence)
        vkDestroyFence(p->device, p->in_flight_fence, nullptr);
    if (p->render_finished)
        vkDestroySemaphore(p->device, p->render_finished, nullptr);
    if (p->image_available)
        vkDestroySemaphore(p->device, p->image_available, nullptr);

    if (p->command_pool)
        vkDestroyCommandPool(p->device, p->command_pool, nullptr);

    vk_destroy_swapchain(p);

    if (p->device)
        vkDestroyDevice(p->device, nullptr);
    if (p->surface)
        vkDestroySurfaceKHR(p->instance, p->surface, nullptr);
    if (p->instance)
        vkDestroyInstance(p->instance, nullptr);

    delete p;
}

bool
alia_glfw_vk_present_resize(alia_glfw_vk_present* p, int width, int height)
{
    if (!p || width <= 0 || height <= 0)
        return false;
    if (p->width == width && p->height == height && p->swapchain
        && (!p->gpu_interop || p->interop_gl_fbo))
        return true;

    vkDeviceWaitIdle(p->device);
    if (!vk_create_swapchain(p, width, height))
        return false;
    return vk_create_gl_target(p, width, height);
}

bool
alia_glfw_vk_present_uses_gpu_interop(alia_glfw_vk_present* present)
{
    return present && present->gpu_interop;
}

bool
alia_glfw_vk_present_get_gl_target(
    alia_glfw_vk_present* present,
    int width,
    int height,
    alia_glfw_vk_gl_target* out_target)
{
    ALIA_ASSERT(present);
    ALIA_ASSERT(out_target);
    if (!alia_glfw_vk_present_resize(present, width, height))
        return false;
    if (!present->interop_gl_fbo)
        return false;
    out_target->framebuffer = present->interop_gl_fbo;
    out_target->width = present->width;
    out_target->height = present->height;
    return true;
}

bool
alia_glfw_vk_present_from_gl_target(alia_glfw_vk_present* present)
{
    ALIA_ASSERT(present);
    if (present->gpu_interop)
        return vk_present_from_gl_interop(present);
    return vk_present_from_readback(present);
}

bool
alia_glfw_vk_present_set_modal_interaction(alia_glfw_vk_present* p, bool modal)
{
    ALIA_ASSERT(p);
    if (!p->vsync || p->modal_interaction == modal)
        return true;
    if (!p->device || !p->swapchain)
        return false;

    p->modal_interaction = modal;
    vkDeviceWaitIdle(p->device);
    // Do not reset in_flight_fence here. vkDeviceWaitIdle already ensures the
    // last submission finished (fence signaled). Resetting would leave the
    // fence unsignaled with no pending submit, so the next vkWaitForFences in
    // vk_present_begin_frame would block forever.

    int width = p->width;
    int height = p->height;
    if (p->window)
        glfwGetFramebufferSize(p->window, &width, &height);
    if (width <= 0 || height <= 0)
        return false;

    if (!vk_create_swapchain(p, width, height))
    {
        p->modal_interaction = !modal;
        return false;
    }
    return true;
}

} // extern "C"
