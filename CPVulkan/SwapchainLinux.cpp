#include <vulkan/vk_icd.h>
#include "Swapchain.h"

#include "Instance.h"
#include "PhysicalDevice.h"

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#include <X11/Xlib.h>
#endif

VkResult Swapchain::Present(uint32_t pImageIndex)
{
    FATAL_ERROR();
}

void Instance::DestroySurface(VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    if (reinterpret_cast<VkIcdSurfaceBase*>(surface)->platform == VK_ICD_WSI_PLATFORM_XCB)
    {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        Free(reinterpret_cast<VkIcdSurfaceXcb*>(surface), pAllocator);
#else
        FATAL_ERROR();
#endif
    }
    else
    {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        Free(reinterpret_cast<VkIcdSurfaceXlib*>(surface), pAllocator);
#else
        FATAL_ERROR();
#endif
    }
}

#if defined(VK_USE_PLATFORM_XCB_KHR)
VkResult Instance::CreateXcbSurface(const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR);
    assert(pCreateInfo->pNext == nullptr);
    assert(pCreateInfo->flags == 0);

    auto surface = Allocate<VkIcdSurfaceXcb>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    surface->base.platform = VK_ICD_WSI_PLATFORM_XCB;
    surface->connection = pCreateInfo->connection;
    surface->window = pCreateInfo->window;
    *pSurface = reinterpret_cast<VkSurfaceKHR>(surface);
    return VK_SUCCESS;
}
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
VkResult Instance::CreateXlibSurface(const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface)
{
    assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
    assert(pCreateInfo->pNext == nullptr);
    assert(pCreateInfo->flags == 0);

    auto surface = Allocate<VkIcdSurfaceXlib>(pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    surface->base.platform = VK_ICD_WSI_PLATFORM_XLIB;
    surface->dpy = pCreateInfo->dpy;
    surface->window = pCreateInfo->window;
    *pSurface = reinterpret_cast<VkSurfaceKHR>(surface);
    return VK_SUCCESS;
}
#endif

VkResult PhysicalDevice::GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    uint32_t width = 0;
    uint32_t height = 0;
    if (reinterpret_cast<VkIcdSurfaceBase*>(surface)->platform == VK_ICD_WSI_PLATFORM_XCB)
    {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        const auto xcbSurface = reinterpret_cast<VkIcdSurfaceXcb*>(surface);
        auto geometry = xcb_get_geometry_reply(xcbSurface->connection, xcb_get_geometry(xcbSurface->connection, xcbSurface->window), nullptr);
        width = geometry->width;
        height = geometry->height;
        free(geometry);
#else
        FATAL_ERROR();
#endif
    }
    else
    {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        const auto xlibSurface = reinterpret_cast<VkIcdSurfaceXlib*>(surface);
        Window rootWindow;
        int x;
        int y;
        unsigned int border;
        unsigned int depth;
        XGetGeometry(xlibSurface->dpy, xlibSurface->window, &rootWindow, &x, &y, &width, &height, &border, &depth);
#else
        FATAL_ERROR();
#endif
    }

    pSurfaceCapabilities->minImageCount = 1;
    pSurfaceCapabilities->maxImageCount = 0;
    pSurfaceCapabilities->currentExtent.width = width;
    pSurfaceCapabilities->currentExtent.height = height;
    pSurfaceCapabilities->minImageExtent.width = 1;
    pSurfaceCapabilities->minImageExtent.height = 1;
    pSurfaceCapabilities->maxImageExtent.width = 8192;
    pSurfaceCapabilities->maxImageExtent.height = 8192;
    pSurfaceCapabilities->maxImageArrayLayers = 1;
    pSurfaceCapabilities->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    pSurfaceCapabilities->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    pSurfaceCapabilities->supportedCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    pSurfaceCapabilities->supportedUsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                VK_IMAGE_USAGE_SAMPLED_BIT |
                                                VK_IMAGE_USAGE_STORAGE_BIT |
                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                                                VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV |
                                                VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
    return VK_SUCCESS;
}
