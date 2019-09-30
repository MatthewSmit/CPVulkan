#pragma once

#include <xcb/xcb.h>

/**
 * @struct xcb_image_t
 * A structure that describes an xcb_image_t.
 */
struct xcb_image_t
{
    uint16_t           width;   /**< Width in pixels, excluding pads etc. */
    uint16_t           height;   /**< Height in pixels. */
    xcb_image_format_t format;   /**< Format. */
    uint8_t            scanline_pad;   /**< Right pad in bits.  Valid pads
				      *   are 8, 16, 32.
				      */
    uint8_t            depth;   /**< Depth in bits. Valid depths
			       *   are 1, 4, 8, 16, 24 for z format,
			       *   1 for xy-bitmap-format, anything
			       *   for xy-pixmap-format.
			       */
    uint8_t            bpp;   /**< Storage per pixel in bits.
			     *   Must be >= depth. Valid bpp
			     *   are 1, 4, 8, 16, 24, 32 for z
			     *   format, 1 for xy-bitmap format,
			     *   anything for xy-pixmap-format.
			     */
    uint8_t	     unit;  /**< Scanline unit in bits for
			     *   xy formats and for bpp == 1,
			     *   in which case valid scanline
			     *   units are 8, 16, 32.  Otherwise,
			     *   will be max(8, bpp).  Must be >= bpp.
			     */
    uint32_t           plane_mask;   /**< When format is
				    *   xy-pixmap and depth >
				    *   1, this says which
				    *   planes are "valid" in
				    *   some vague sense.
				    *   Currently used only
				    *   by xcb_image_get/put_pixel(),
				    *   and set only by
				    *   xcb_image_get().
				    */
    xcb_image_order_t  byte_order;   /**< Component byte order
				    *   for z-pixmap, byte
				    *   order of scanline unit
				    *   for xy-bitmap and
				    *   xy-pixmap.  Nybble
				    *   order for z-pixmap
				    *   when bpp == 4.
				    */
    xcb_image_order_t  bit_order;    /**< Bit order of
				    *   scanline unit for
				    *   xy-bitmap and
				    *   xy-pixmap.
				    */
    uint32_t           stride;   /**< Bytes per image row.
				*   Computable from other
				*   data, but cached for
				*   convenience/performance.
				*/
    uint32_t           size;   /**< Size of image data in bytes.
			      *   Computable from other
			      *   data, but cached for
			      *   convenience/performance.
			      */
    void *             base;   /**< Malloced block of storage that
			      *   will be freed by
			      *   @ref xcb_image_destroy() if non-null.
			      */
    uint8_t *          data;   /**< The actual image. */
};

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
                  uint8_t *          data);


/**
 * Put an image onto the X server.
 * @param conn The connection to the X server.
 * @param draw The draw you get the image from.
 * @param gc The graphic context.
 * @param image The image you want to combine with the rectangle.
 * @param x The x coordinate, which is relative to the origin of the
 * drawable and defines the x coordinate of the upper-left corner of the
 * rectangle.
 * @param y The y coordinate, which is relative to the origin of the
 * drawable and defines the x coordinate of the upper-left corner of
 * the rectangle.
 * @param left_pad Notionally shift an xy-bitmap or xy-pixmap image
 * to the right some small amount, for some reason.  XXX Not clear
 * this is currently supported correctly.
 * @return The cookie returned by xcb_put_image().
 *
 * This function combines an image with a rectangle of the
 * specified drawable @p draw. The image must be in native
 * format for the connection.  The image is drawn at the
 * specified location in the drawable. For the xy-bitmap
 * format, the foreground pixel in @p gc defines the source
 * for the one bits in the image, and the background pixel
 * defines the source for the zero bits. For xy-pixmap and
 * z-pixmap formats, the depth of the image must match the
 * depth of the drawable; the gc is ignored.
 *
 * @ingroup xcb__image_t
 */
xcb_void_cookie_t
xcb_image_put (xcb_connection_t *  conn,
               xcb_drawable_t      draw,
               xcb_gcontext_t      gc,
               xcb_image_t *       image,
               int16_t             x,
               int16_t             y,
               uint8_t             left_pad);


/**
 * Destroy an image.
 * @param image The image to be destroyed.
 *
 * This function frees the memory associated with the @p image
 * parameter.  If its base pointer is non-null, it frees
 * that also.
 * @ingroup xcb__image_t
 */
void
xcb_image_destroy (xcb_image_t *image);