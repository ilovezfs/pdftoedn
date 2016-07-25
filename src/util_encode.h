#pragma once

#include <string>
#include <ostream>

namespace pdftoedn
{
    namespace util
    {
        namespace encode {

            bool encode_image(std::ostream& output, ImageStream* img_str,
                              uint32_t width, uint32_t height, uint32_t num_pix_comps, uint8_t bpp,
                              GfxImageColorMap *colorMap);
            bool encode_rgba_image(std::ostream& output,
                                   ImageStream* img_str, uint32_t width, uint32_t height,
                                   uint32_t num_pix_comps, uint8_t bpp, GfxImageColorMap *color_map,
                                   ImageStream* mask_str, uint32_t mask_width, uint32_t mask_height,
                                   uint32_t mask_num_pix_comps, uint8_t mask_bpp, GfxImageColorMap *mask_color_map,
                                   bool mask_invert);
            bool encode_mask(std::ostream& output, ImageStream* img_str, uint32_t width, uint32_t height,
                             const StreamProps& properties);
#if 0
            bool encode_grey_image(std::ostream& output, ImageStream* img_str,
                                   uint32_t width, uint32_t height, uint32_t num_pix_comps, uint8_t bpp,
                                   GfxImageColorMap *colorMap);
#endif
        }
    }
}
