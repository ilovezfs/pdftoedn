#pragma once

#include <string>
#include <ostream>

namespace pdftoedn
{
    namespace util
    {
        namespace encode {

            bool encode_image(std::ostream& output, ImageStream* img_str, const StreamProps& properties,
                              GfxImageColorMap *colorMap);
            bool encode_rgba_image(std::ostream& output, ImageStream* img_str, ImageStream* mask_str,
                                   const StreamProps& properties,
                                   GfxImageColorMap *color_map, GfxImageColorMap *mask_color_map,
                                   bool mask_invert);
            bool encode_mask(std::ostream& output, ImageStream* img_str, const StreamProps& properties);
#if 0
            bool encode_grey_image(std::ostream& output, ImageStream* img_str, const StreamProps& properties,
                                   GfxImageColorMap *colorMap);
#endif
        }
    }
}
