#include "XcbHelper.h"

#include <cassert>
#include <cstdlib>

static xcb_image_format_t effective_format(xcb_image_format_t format, uint8_t bpp)
{
    if (format == XCB_IMAGE_FORMAT_Z_PIXMAP && bpp != 1)
        return format;
    return XCB_IMAGE_FORMAT_XY_PIXMAP;
}

static int format_valid (uint8_t depth, uint8_t bpp, uint8_t unit, xcb_image_format_t format, uint8_t xpad)
{
    xcb_image_format_t  ef = effective_format(format, bpp);
    if (depth > bpp)
        return 0;
    switch(ef) {
        case XCB_IMAGE_FORMAT_XY_PIXMAP:
            switch(unit) {
                case 8:
                case 16:
                case 32:
                    break;
                default:
                    return 0;
            }
            if (xpad < bpp)
                return 0;
            switch (xpad) {
                case 8:
                case 16:
                case 32:
                    break;
                default:
                    return 0;
            }
            break;
        case XCB_IMAGE_FORMAT_Z_PIXMAP:
            switch (bpp) {
                case 4:
                    if (unit != 8)
                        return 0;
                    break;
                case 8:
                case 16:
                case 24:
                case 32:
                    if (unit != bpp)
                        return 0;
                    break;
                default:
                    return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

static uint32_t xcb_roundup (uint32_t base, uint32_t pad)
{
    uint32_t b = base + pad - 1;
    // faster if pad is a power of two
    if (((pad - 1) & pad) == 0)
        return b & -pad;
    return b - b % pad;
}

static uint32_t xcb_mask(uint32_t n)
{
    return n == 32 ? ~0 : (1 << n) - 1;
}

void xcb_image_annotate(xcb_image_t* image)
{
    xcb_image_format_t ef = effective_format(image->format, image->bpp);
    switch (ef) {
        case XCB_IMAGE_FORMAT_XY_PIXMAP:
            image->stride = xcb_roundup(image->width, image->scanline_pad) >> 3;
            image->size = image->height * image->stride * image->depth;
            break;
        case XCB_IMAGE_FORMAT_Z_PIXMAP:
            image->stride = xcb_roundup((uint32_t)image->width *
                                        (uint32_t)image->bpp,
                                        image->scanline_pad) >> 3;
            image->size = image->height * image->stride;
            break;
        default:
            assert(0);
    }
}

xcb_image_t*
xcb_image_create (uint16_t           width,
                  uint16_t           height,
                  xcb_image_format_t format,
                  uint8_t            xpad,
                  uint8_t            depth,
                  uint8_t            bpp,
                  uint8_t            unit,
                  xcb_image_order_t  byte_order,
                  xcb_image_order_t  bit_order,
                  void *             base,
                  uint32_t           bytes,
                  uint8_t *          data)
{
    xcb_image_t*  image;

    if (unit == 0) {
        switch (format) {
            case XCB_IMAGE_FORMAT_XY_BITMAP:
            case XCB_IMAGE_FORMAT_XY_PIXMAP:
                unit = 32;
                break;
            case XCB_IMAGE_FORMAT_Z_PIXMAP:
                if (bpp == 1) {
                    unit = 32;
                    break;
                }
                if (bpp < 8) {
                    unit = 8;
                    break;
                }
                unit = bpp;
                break;
        }
    }
    if (!format_valid(depth, bpp, unit, format, xpad))
        return nullptr;
    image = (xcb_image_t*)malloc(sizeof(*image));
    if (image == nullptr)
        return nullptr;
    image->width = width;
    image->height = height;
    image->format = format;
    image->scanline_pad = xpad;
    image->depth = depth;
    image->bpp = bpp;
    image->unit = unit;
    image->plane_mask = xcb_mask(depth);
    image->byte_order = byte_order;
    image->bit_order = bit_order;
    xcb_image_annotate(image);

    /*
     * Ways this function can be called:
     *   * with data: we fail if bytes isn't
     *     large enough, else leave well enough alone.
     *   * with base and !data: if bytes is zero, we
     *     default; otherwise we fail if bytes isn't
     *     large enough, else fill in data
     *   * with !base and !data: we malloc storage
     *     for the data, save that address as the base,
     *     and fail if malloc does.
     *
     * When successful, we establish the invariant that data
     * points at sufficient storage that may have been
     * supplied, and base is set iff it should be
     * auto-freed when the image is destroyed.
     *
     * Except as a special case when base = 0 && data == 0 &&
     * bytes == ~0 we just return the image structure and let
     * the caller deal with getting the allocation right.
     */
    if (!base && !data && bytes == ~0) {
        image->base = nullptr;
        image->data = nullptr;
        return image;
    }
    if (!base && data && bytes == 0)
        bytes = image->size;
    image->base = base;
    image->data = data;
    if (!image->data) {
        if (image->base) {
            image->data = (uint8_t*)image->base;
        } else {
            bytes = image->size;
            image->base = malloc(bytes);
            image->data = (uint8_t*)image->base;
        }
    }
    if (!image->data || bytes < image->size) {
        free(image);
        return nullptr;
    }
    return image;
}


void xcb_image_destroy(xcb_image_t* image)
{
    if (image->base)
        free (image->base);
    free (image);
}


xcb_void_cookie_t
xcb_image_put (xcb_connection_t *  conn,
               xcb_drawable_t      draw,
               xcb_gcontext_t      gc,
               xcb_image_t *       image,
               int16_t             x,
               int16_t             y,
               uint8_t             left_pad)
{
    return xcb_put_image(conn, image->format, draw, gc,
                         image->width, image->height,
                         x, y, left_pad,
                         image->depth,
                         image->size,
                         image->data);
}