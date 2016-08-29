#pragma once

#include <ostream>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#include <poppler/GfxState.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif


#include "base_types.h"

namespace pdftoedn
{
    typedef uint32_t color_comp_t;

    // ----------------------------------
    //
    struct Color : gemable {
        virtual std::ostream& to_edn(std::ostream& o) const = 0;
    };

    // ----------------------------------
    // rubifies color commands
    //
    class RGBColor : public Color {
    public:
        RGBColor() : r(0), g(0), b(0) { }
        RGBColor(const GfxRGB& color) :
            r(color.r), g(color.g), b(color.b)
        { }
        RGBColor(color_comp_t red, color_comp_t green, color_comp_t blue) :
            r(red), g(green), b(blue)
        { }

        color_comp_t red() const { return colToByte(r); }
        color_comp_t green() const { return colToByte(g); }
        color_comp_t blue() const { return colToByte(b); }

        bool equals(color_comp_t red, color_comp_t green, color_comp_t blue) const {
            return (r == red && g == green && b == blue);
        }

        virtual std::ostream& to_edn(std::ostream& o) const;

    private:
        color_comp_t r, g, b;
    };

} // namespace
