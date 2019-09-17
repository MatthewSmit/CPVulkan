#include <vulkan/vk_icd.h>
#include "Swapchain.h"

#include "Image.h"
#include "Instance.h"
#include "PhysicalDevice.h"

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#include <X11/Xlib.h>
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
static xcb_format_t* FindFormat(const xcb_setup_t* setup, uint8_t depth, uint8_t bpp)
{
    auto format = xcb_setup_pixmap_formats(setup);
    auto formatEnd = format + xcb_setup_pixmap_formats_length(setup);

    for (; format != formatEnd; ++format)
    {
        if (format->depth == depth && format->bits_per_pixel == bpp && format->scanline_pad == bpp)
        {
            return format;
        }
    }

    return nullptr;
}
#endif

VkResult Swapchain::Present(uint32_t pImageIndex)
{
	const auto surfaceBase = UnwrapVulkan<VkIcdSurfaceBase>(surface);
    const auto image = images[pImageIndex];

    if (surfaceBase->platform == VK_ICD_WSI_PLATFORM_XCB)
    {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        const auto xcbSurface = reinterpret_cast<VkIcdSurfaceXcb*>(surfaceBase);
        auto geometry = xcb_get_geometry_reply(xcbSurface->connection, xcb_get_geometry(xcbSurface->connection, xcbSurface->window), nullptr);
        const xcb_setup_t* setup = xcb_get_setup(xcbSurface->connection);
        auto* image32 = (uint8_t*)malloc(image->getWidth() * image->getHeight() * 4);
        xcb_format_t* format = FindFormat(setup, geometry->depth, 32);
        auto xcbImage = xcb_image_create(image->getWidth(),
                                         image->getHeight(),
                                         XCB_IMAGE_FORMAT_Z_PIXMAP,
                                         format->scanline_pad,
                                         format->depth,
                                         format->bits_per_pixel,
                                         0,
                                         (xcb_image_order_t)setup->image_byte_order,
                                         XCB_IMAGE_ORDER_LSB_FIRST,
                                         image32,
                                         image->getWidth() * image->getHeight() * 4,
                                         image32);
        memcpy(xcbImage->data, image->getData(), image->getWidth() * image->getHeight() * 4);
        auto gc = xcb_generate_id(xcbSurface->connection);
        auto mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
        uint32_t values[]
        {
            0,
            0
        };
        xcb_create_gc(xcbSurface->connection, gc, xcbSurface->window, mask, values);
        xcb_image_put(xcbSurface->connection, xcbSurface->window, gc, xcbImage, 0, 0, 0);
        xcb_image_destroy(xcbImage);
        xcb_flush(xcbSurface->connection);

        auto width = geometry->width;
        auto height = geometry->height;
        free(geometry);
        if (width != image->getWidth() || height != image->getHeight())
        {
            return VK_ERROR_OUT_OF_DATE_KHR;
        }
#else
        FATAL_ERROR();
#endif
    }
    else
    {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        const auto xlibSurface = reinterpret_cast<VkIcdSurfaceXlib*>(surfaceBase);
        FATAL_ERROR();
#else
        FATAL_ERROR();
#endif
    }

    return VK_SUCCESS;
}

void Instance::DestroySurface(VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
	const auto surfaceBase = UnwrapVulkan<VkIcdSurfaceBase>(surface);
    if (surfaceBase->platform == VK_ICD_WSI_PLATFORM_XCB)
    {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        Free(reinterpret_cast<VkIcdSurfaceXcb*>(surfaceBase), pAllocator);
#else
        FATAL_ERROR();
#endif
    }
    else
    {
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        Free(reinterpret_cast<VkIcdSurfaceXlib*>(surfaceBase), pAllocator);
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

	WrapVulkan(surface, pSurface);
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

	WrapVulkan(surface, pSurface);
    return VK_SUCCESS;
}
#endif

VkResult PhysicalDevice::GetSurfaceCapabilities(VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
	const auto surfaceBase = UnwrapVulkan<VkIcdSurfaceBase>(surface);
    uint32_t width = 0;
    uint32_t height = 0;
    if (surfaceBase->platform == VK_ICD_WSI_PLATFORM_XCB)
    {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        const auto xcbSurface = reinterpret_cast<VkIcdSurfaceXcb*>(surfaceBase);
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
        const auto xlibSurface = reinterpret_cast<VkIcdSurfaceXlib*>(surfaceBase);
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
