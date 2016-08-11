#ifdef EDSEL_RUBY_GEM
#include <rice/Hash.hpp>
#endif

#include "pdf_links.h"
#include "util.h"
#include "util_edn.h"

namespace pdftoedn
{
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_TOP             = "top";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_BOTTOM          = "bottom";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_LEFT            = "left";
    const pdftoedn::Symbol PdfLink::SYMBOL_POS_RIGHT           = "right";
    const pdftoedn::Symbol PdfLink::SYMBOL_ZOOM                = "zoom";

    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_TYPE           = "type";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_EFFECT         = "effect";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_EFFECTS[]      = { "none", "invert", "outline", "push" };
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_ACTION_TYPE    = "action";
    const pdftoedn::Symbol PdfAnnotLink::SYMBOL_ACTION_TYPES[] = { "goto", "gotor", "uri", "launch" };

    const pdftoedn::Symbol PdfAnnotLinkDest::SYMBOL_DEST       = "dest";
    const pdftoedn::Symbol PdfAnnotLinkGoto::SYMBOL_PAGE       = "page";


    // =============================================
    // links found in outlines
    //

    //
    // TL mode
    void PdfLink::set_top_left(double t, double l)
    {
        pos.y = t;
        pos.x = l;
        orientation = TOP_LEFT;
    }

    //
    // BR mode
    void PdfLink::set_bottom_right(double b, double r)
    {
        pos.x = r;
        pos.y = b;
        orientation = BOTTOM_RIGHT;
    }

#ifdef EDSEL_RUBY_GEM
    //
    // rubify
    Rice::Object PdfLink::to_ruby() const
    {
        Rice::Hash link_h;

        //
        // only indicate any values set
        if (orientation == TOP_LEFT) {
            if (pos.y > 0) {
                link_h[ SYMBOL_POS_TOP ]    = pos.y;
            }

            if (pos.x > 0) {
                link_h[ SYMBOL_POS_LEFT ]   = pos.x;
            }
        } else {
            if (pos.y > 0) {
                link_h[ SYMBOL_POS_BOTTOM ] = pos.y;
            }
            if (pos.x > 0) {
                link_h[ SYMBOL_POS_RIGHT ]  = pos.x;
            }
        }

        // and zoom
        if (zoom != -1) {
            link_h[ SYMBOL_ZOOM ]           = zoom;
        }
        return link_h;
    }

#else

    util::edn::Hash& PdfLink::to_edn_hash(util::edn::Hash& link_h) const
    {
        //
        // only indicate any values set
        if (orientation == TOP_LEFT) {
            if (pos.y > 0) {
                link_h.push( SYMBOL_POS_TOP, pos.y );
            }

            if (pos.x > 0) {
                link_h.push( SYMBOL_POS_LEFT, pos.x );
            }
        } else {
            if (pos.y > 0) {
                link_h.push( SYMBOL_POS_BOTTOM, pos.y );
            }
            if (pos.x > 0) {
                link_h.push( SYMBOL_POS_RIGHT, pos.x );
            }
        }

        // and zoom
        if (zoom != -1) {
            link_h.push( SYMBOL_ZOOM, zoom );
        }
        return link_h;
    }

    std::ostream& PdfLink::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(3);
        o << to_edn_hash(link_h);
        return o;
    }
#endif

    // =============================================
    // annotation links
    //

    //
    // rubify
#ifdef EDSEL_RUBY_GEM
    Rice::Object PdfAnnotLink::to_ruby() const
    {
        Rice::Hash link_h = PdfLink::to_ruby();

        link_h[ SYMBOL_TYPE ]         = SYMBOL_ACTION_TYPES[ type ];
        if (effect != EFFECT_NONE) {
            link_h[ SYMBOL_EFFECT ]   = SYMBOL_EFFECTS[ effect ];
        }
        link_h[ BoundingBox::SYMBOL ] = bbox.to_ruby();

        return link_h;
    }

#else

    util::edn::Hash& PdfAnnotLink::to_edn_hash(util::edn::Hash& link_h) const
    {
        PdfLink::to_edn_hash(link_h);

        link_h.push( SYMBOL_TYPE, SYMBOL_ACTION_TYPES[ type ] );
        if (effect != EFFECT_NONE) {
            link_h.push( SYMBOL_EFFECT, SYMBOL_EFFECTS[ effect ] );
        }
        link_h.push( BoundingBox::SYMBOL, bbox );
        return link_h;
    }

    std::ostream& PdfAnnotLink::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(6);
        o << to_edn_hash(link_h);
        return o;
    }
#endif

    //
    // rubify link dest
#ifdef EDSEL_RUBY_GEM
    Rice::Object PdfAnnotLinkDest::to_ruby() const
    {
        Rice::Hash link_h = PdfAnnotLink::to_ruby();
        if (dest.length() > 0) {
            link_h[ SYMBOL_DEST ]     = util::wstring_to_ruby(util::string_to_iso8859(dest.c_str()));
        }
        return link_h;
    }

#else

    std::ostream& PdfAnnotLinkDest::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(7);
        o << to_edn_hash(link_h);
        return o;
    }

    util::edn::Hash& PdfAnnotLinkDest::to_edn_hash(util::edn::Hash& link_h) const
    {
        PdfAnnotLink::to_edn_hash(link_h);
        if (dest.length() > 0) {
            link_h.push( SYMBOL_DEST, util::wstring_to_utfstring(util::string_to_iso8859(dest.c_str())) );
        }
        return link_h;
    }
#endif

    //
    // link goto
#ifdef EDSEL_RUBY_GEM
    Rice::Object PdfAnnotLinkGoto::to_ruby() const
    {
        Rice::Hash link_h = PdfAnnotLinkDest::to_ruby();
        if (page != -1) {
            link_h[ SYMBOL_PAGE ]     = page;
        }
        return link_h;
    }

#else

    std::ostream& PdfAnnotLinkGoto::to_edn(std::ostream& o) const
    {
        util::edn::Hash link_h(7);
        PdfAnnotLinkDest::to_edn_hash(link_h);

        if (page != -1) {
            link_h.push( SYMBOL_PAGE, page );
        }
        o << link_h;
        return o;
    }
#endif

} // namespace
