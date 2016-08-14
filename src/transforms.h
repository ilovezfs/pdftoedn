#pragma once

#include <list>
#include "base_types.h"

namespace pdftoedn
{
    // -------------------------------------------------------
    // abstract base xform type
    //
    struct Transform : public gemable
    {
        virtual std::ostream& to_edn(std::ostream& o) const = 0;
        static util::edn::Vector& list_to_edn(const std::list<Transform*>& l, util::edn::Vector& transform_a);

        static const pdftoedn::Symbol SYMBOL;
    };


    // -------------------------------------------------------
    // rotation transforms
    //
    class Rotate : public Transform
    {
    public:
        Rotate(double a, double x, double y) :
            angle(a), origin(x, y)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL;

    private:
        double angle;
        Coord origin;
    };


    // -------------------------------------------------------
    // translate transforms
    //
    class Translate : public Transform
    {
    public:
        Translate(double x, double y) :
            delta(x, y)
        { }

        virtual std::ostream& to_edn(std::ostream& o) const;

        static const pdftoedn::Symbol SYMBOL;

    private:
        Coord delta;
    };

} // namespace

