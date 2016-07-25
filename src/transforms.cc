#include <list>

#ifdef EDSEL_RUBY_GEM
#include <rice/Array.hpp>
#include <rice/Hash.hpp>
#endif

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
    //
#ifdef EDSEL_RUBY_GEM
    Rice::Object Transform::list_to_ruby(const std::list<Transform*>& l)
    {
        Rice::Array transform_a;
        std::for_each( l.begin(), l.end(),
                       [&](const Transform* t) { transform_a.push(t->to_ruby()); }
                       );
        return transform_a;
    }
#endif

    util::edn::Vector& Transform::list_to_edn(const std::list<Transform*>& l, util::edn::Vector& transform_a)
    {
        std::for_each( l.begin(), l.end(),
                       [&](const Transform* t) { transform_a.push(t); }
                       );
        return transform_a;
    }

#ifdef EDSEL_RUBY_GEM
    //
    //
    Rice::Object Rotate::to_ruby() const
    {
        Rice::Hash rot_h;

        rot_h[ Transform::SYMBOL ]       = SYMBOL;
        rot_h[ PdfText::SYMBOL_ORIGIN ]  = origin.to_ruby();
        rot_h[ SYMBOL_ANGLE ]            = angle;
        return rot_h;
    }
#endif
    std::ostream& Rotate::to_edn(std::ostream& o) const
    {
        util::edn::Hash rot_h(3);

        rot_h.push( Transform::SYMBOL,      SYMBOL );
        rot_h.push( PdfText::SYMBOL_ORIGIN, origin );
        rot_h.push( SYMBOL_ANGLE,           angle );
        o << rot_h;
        return o;
    }

#ifdef EDSEL_RUBY_GEM
    //
    //
    Rice::Object Translate::to_ruby() const
    {
        Rice::Hash translate_h;

        translate_h[ Transform::SYMBOL ] = SYMBOL;
        translate_h[ SYMBOL_DELTA ]      = delta.to_ruby();
        return translate_h;
    }
#endif
    std::ostream& Translate::to_edn(std::ostream& o) const
    {
        util::edn::Hash translate_h(2);

        translate_h.push( Transform::SYMBOL, SYMBOL );
        translate_h.push( SYMBOL_DELTA,      delta );
        o << translate_h;
        return o;
    }

} // namespace
