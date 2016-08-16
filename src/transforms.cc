#include <list>

#include "transforms.h"
#include "text.h"
#include "util_edn.h"

namespace pdftoedn
{
    const pdftoedn::Symbol Transform::SYMBOL            = "transform";
    const pdftoedn::Symbol Rotate::SYMBOL               = "rotate";
    const pdftoedn::Symbol Translate::SYMBOL            = "translate";

    static const pdftoedn::Symbol SYMBOL_ANGLE          = "angle";
    static const pdftoedn::Symbol SYMBOL_DELTA          = "delta";

    //
    // output a list of transforms in EDN format
    util::edn::Vector& Transform::list_to_edn(const std::list<Transform*>& transform_list,
                                              util::edn::Vector& transform_a)
    {
        for (const Transform* t : transform_list) {
            transform_a.push( t );
        }
        return transform_a;
    }


    //
    // EDN rotate transforms
    std::ostream& Rotate::to_edn(std::ostream& o) const
    {
        util::edn::Hash rot_h(3);

        rot_h.push( Transform::SYMBOL,      SYMBOL );
        rot_h.push( PdfText::SYMBOL_ORIGIN, origin );
        rot_h.push( SYMBOL_ANGLE,           angle );
        o << rot_h;
        return o;
    }

    //
    // EDN translate transforms
    std::ostream& Translate::to_edn(std::ostream& o) const
    {
        util::edn::Hash translate_h(2);

        translate_h.push( Transform::SYMBOL, SYMBOL );
        translate_h.push( SYMBOL_DELTA,      delta );
        o << translate_h;
        return o;
    }

} // namespace
