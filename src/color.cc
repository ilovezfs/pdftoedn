#include <ostream>
#include <iomanip>
#include <sstream>

#include "color.h"

namespace pdftoedn
{
    // =============================================
    // RGB colors

    //
    // output color components into an HTML-style color string (e.g.: #00ff00)
    std::ostream& RGBColor::to_edn(std::ostream& o) const
    {
        o << '"'
          << "#" << std::setfill('0')
          << std::setw(2) << std::hex << red()
          << std::setw(2) << std::hex << green()
          << std::setw(2) << std::hex << blue()
          << '"';
        return o;
    }
} // namespace
